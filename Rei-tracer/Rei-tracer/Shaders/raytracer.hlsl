struct CameraInfo
{
	float3 position;
	float3 direction;
	float3 up;
	float3 right;
	float fardist;
	float neardist;
	float aspectratio;
	float fov;
	int width;
	int height;
	float pad;
	float pad2;

};

cbuffer CameraBuffer : register(b0)
{
	float3 gCamPos : packoffset(c0);
	float gCamFar : packoffset(c0.w);
	float3 gCamDir : packoffset(c1);
	float gCamNear : packoffset(c1.w);
	float3 gCamUp : packoffset(c2);
	float gAspectRatio : packoffset(c2.w);
	float3 gCamRight : packoffset(c3);
	float gFOV : packoffset(c3.w);
	int gWidth : packoffset(c4.x);
	int gHeight : packoffset(c4.y);
	float pad : packoffset(c4.z);
	float pad2 : packoffset(c4.w);
}

cbuffer Counts : register(b1)
{
	int gSphereCount;
	int gTriangleCount;
	int gPointLightCount;
	int gBounceCount;
	int gPlaneCount;
	int gOBBCount;
	int pad1;
	int pad3;
};

struct Sphere
{
	float3 position;
	float radius;
};

struct Plane
{
	float3 normal;
	float d;
};
struct Ray
{
	float3 o;
	float3 d;
};

struct Vertex
{
	float3 position;
	float u;		//adhere to how vectors are packaged {pos, pos, pos, u} {normal, normal, normal, v} {tangent, tangent, tangent, tangent}
	float3 normal;
	float v;
	float4 tangent; //w-component is handedness
};

struct Triangle
{
	Vertex v1;
	Vertex v2;
	Vertex v3;
};

struct PointLight
{
	float3 position;
	float intensity;
	float3 color;
	float range;
};

StructuredBuffer<Sphere> gSpheres : register(t0);
StructuredBuffer<Triangle> gTriangles : register(t1);
StructuredBuffer<Plane> gPlanes : register(t2);
StructuredBuffer<PointLight> gPointLights : register(t3);

void RayVSSphere(Sphere s, Ray r, out float t0, out float t1)
{
	t0 = -1.0f;
	t1 = -1.0f;
	float3 l = s.position - r.o;
	float tca = dot(l, r.d);
	if (tca < 0.0f)
		return;
	float d2 = dot(l, l) - tca*tca;
	float radius2 = s.radius * s.radius;
	if (d2 > radius2)
		return;
	float thc = sqrt(radius2 - d2);
	t0 = tca - thc;
	t1 = tca + thc;
}

void RayVSPlane(Plane p, Ray r, out float distance)
{
	distance = (p.d - dot(r.o, p.normal)) / dot(r.d, p.normal);
}

void RayVSTriangle(Triangle t, Ray r, out float distance, out float u, out float v, out float3 normal)
{
	distance = gCamFar;
	u = 0.0f;
	v = 0.0f;
	float3 e1 = t.v2.position - t.v1.position;
	float3 e2 = t.v3.position - t.v1.position;
	float3 q = cross(r.d, e2);
	float a = dot(e1, q); //The determinant of the matrix (-direction e1 e2)
	if (a < 0.0001f && a > -0.0001f)
		return; //avoid determinants close to zero since we will divide by this
	float f = 1.0f / a;
	float3 s = r.o - t.v1.position;
	float bu = f * dot(s, q); //barycentric u coordinate
	if (bu < 0.0f)
		return;
	float3 rr = cross(s, e1);
	float bv = f*dot(r.d, rr); //barycentric v coordinate
	if (bv < 0.0f || bu + bv > 1.0f)
		return;
	float ttt = f * dot(e2, rr);
	if (ttt > 0.0f)
	{
		distance = ttt;
		u = bu * t.v2.u + bv * t.v3.u + (1.0f - bv - bu) * t.v1.u;
		v = bu * t.v2.v + bv * t.v3.v + (1.0f - bv - bu) * t.v1.v;
		normal = bu * t.v2.normal + bv * t.v3.normal + (1.0f - bv - bu) * t.v1.normal;
	}
}

void PointLightContribution(float3 origin, float3 normal, PointLight pointlight, out float3 specular, out float3 diffuse)
{
	specular = float3(0.0f, 0.0f, 0.0f);
	diffuse = float3(0.0f, 0.0f, 0.0f);
	float3 toLight = pointlight.position - origin;
	float dist = length(toLight);
	//if (dist > pointlight.range)
	//	return;
	toLight = toLight / dist;
	float NdL = dot(toLight, normal);
	if (NdL < 0.0f)
		return; //No contribution at all, return
	//Check for occlusion (shadows)
	float t0, t1;
	Ray r;
	r.o = origin;
	r.d = toLight;
	for (int i = 0; i < gSphereCount; i++)
	{
		RayVSSphere(gSpheres[i], r, t0, t1);
		if (t0 > 0.0f && t0 < dist)
			return;
	}
	float divby = (dist / pointlight.range) + 1.0f;
	float attenuation = pointlight.intensity / (divby * divby);
	diffuse = saturate(NdL * pointlight.color * attenuation);
	float3 halfVector = normalize(toLight + normalize(gCamPos - origin));
	float NdH = saturate(dot(normal, halfVector));
	specular = saturate(pointlight.color * pow(NdH, 96.0f) * attenuation);
	
}

RWTexture2D<float4> output : register(u0);

[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID )
{

	float3 rayPos = gCamPos + gCamDir * gCamFar;
	rayPos += gCamRight * ((threadID.x - gWidth / 2.0f) / gWidth) * (gCamFar / tan(gFOV / 2.0f));
	rayPos += -gCamUp * ((threadID.y - gHeight / 2.0f) / gHeight) * (gCamFar / gAspectRatio);

	Ray r;
	r.o = gCamPos;
	r.d = normalize(rayPos - gCamPos);
	float t0, t1;
	float spheredistance = gCamFar;
	int sphereindex = -1;
	for (int i = 0; i < gSphereCount; i++)
	{
		RayVSSphere(gSpheres[i], r, t0, t1);
		if (t0 > 0.0f && t0 < spheredistance)
		{
			spheredistance = t0;
			sphereindex = i;
		}
	}

	float planedistance = gCamFar;
	int planeindex = -1;
	for (int i = 0; i < gPlaneCount; i++)
	{
		RayVSPlane(gPlanes[i], r, t0);
		if (t0 > 0.0f && t0 < planedistance)
		{
			planedistance = t0;
			planeindex = i;
		}
	}


	float dduu = 0.0f;
	float ddvv = 0.0f;
	float3 narmal = float3(0.0f, 0.0f, 0.0f);
	float triangledist = gCamFar;
	int triangleIndex = -1;
	for (int i = 0; i < gTriangleCount; i++)
	{
		RayVSTriangle(gTriangles[i], r, t0, dduu, ddvv, narmal);
		if (t0 > 0.0f && t0 < triangledist)
		{
			triangledist = t0;
			triangleIndex = i;
		}
	}
	

	float3 intersectionNormal = r.d;
	float3 intersectionPoint = r.o;

	if (spheredistance < min(gCamFar, min(planedistance, max(triangledist, 0))) && sphereindex >= 0)
	{
		intersectionPoint += r.d * spheredistance;
		intersectionNormal = normalize(intersectionPoint - gSpheres[sphereindex].position);
		//output[threadID.xy] = float4(normal.xyz, 1.0f);
	}
	else if (planedistance < spheredistance && planeindex >= 0)
	{
		intersectionPoint += r.d * planedistance;
		intersectionNormal = gPlanes[planeindex].normal;
	}
	else if (triangledist > 0.0f && dduu > 0.0f)
	{
		intersectionPoint += r.d * triangledist;
		intersectionNormal = narmal;
	}
	PointLight pp;
	pp.position = float3(20.5f, 5.0f, -20.5f);
	pp.range = 30.0f;
	pp.intensity = 1.0f;
	pp.color = float3(1.0f, 1.0f, 1.0f);
	float3 ldiffuse = float3(0.0f, 0.0f, 0.0f);
	float3 lspec = float3(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < gPointLightCount; i++)
	{
		float3 tempdiff = float3(0.0f, 0.0f, 0.0f);
		float3 tempspec = float3(0.0f, 0.0f, 0.0f);
		PointLightContribution(intersectionPoint, intersectionNormal, gPointLights[i], tempspec, tempdiff);
		ldiffuse += tempdiff;
		lspec += tempspec;
	}
//	PointLightContribution(intersectionPoint, intersectionNormal, pp, lspec, ldiffuse);

//	output[threadID.xy] = float4(length(ldiffuse).xxx, 1.0f);
//	output[threadID.xy] = float4(intersectionPoint.xyz, 1.0f);
	output[threadID.xy] = saturate(float4(intersectionNormal * (ldiffuse + lspec) , 1.0f));
}