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
	uint gWidth : packoffset(c4.x);
	uint gHeight : packoffset(c4.y);
	float pad : packoffset(c4.z);
	float pad2 : packoffset(c4.w);
}

cbuffer Counts : register(b1)
{
	int gSphereCount;
	int gTriangleCount;
	int gPointLightCount;
	int gBounceCount;
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

void RayVSSphere(Sphere s, Ray r, inout float t0, inout float3 normal)
{
	float3 l = s.position - r.o;
	float tca = dot(l, r.d);
	if (tca < 0.0f)
		return;
	float d2 = dot(l, l) - tca*tca;
	float radius2 = s.radius * s.radius;
	if (d2 > radius2)
		return;
	float thc = sqrt(radius2 - d2);
	float dist = tca - thc;
	if(dist < t0 || t0 < 0.0f)
	{
		t0 = tca - thc;
		normal = normalize((r.o + r.d * dist) - s.position);
	}
}

void RayVSSphereDistance(Sphere s, Ray r, out float t0)
{
	t0 = -1.0f;
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
}

void RayVSTriangle(Triangle t, Ray r, inout float distance, inout float u, inout float v, out float3 normal)
{

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
	if (ttt > 0.0f && (ttt < distance || distance < 0))
	{
		distance = ttt;
		u = bu * t.v2.u + bv * t.v3.u + (1.0f - bv - bu) * t.v1.u;
		v = bu * t.v2.v + bv * t.v3.v + (1.0f - bv - bu) * t.v1.v;
		normal = bu * t.v2.normal + bv * t.v3.normal + (1.0f - bv - bu) * t.v1.normal;
	}
}

//Used for checking occlusion of lights
void RayVSTriangleDistance(Triangle t, Ray r, out float distance)
{
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
	distance = f * dot(e2, rr);
}

void PointLightContribution(float3 origin, float3 normal, PointLight pointlight, inout float3 specular, inout float3 diffuse)
{
	//specular = float3(0.0f, 0.0f, 0.0f);
	//diffuse = float3(0.0f, 0.0f, 0.0f);
	float3 toLight = pointlight.position - origin;
	float dist = length(toLight);
	//if (dist > pointlight.range)
	//	return;
	toLight = toLight / dist;
	float NdL = dot(toLight, normal);
	if (NdL < 0.0f)
		return; //No contribution at all, return
	//Check for occlusion (shadows)
	float t0;
	Ray r;
	r.o = origin;
	r.d = toLight;
	r.o += 0.0001f * r.d;
	for (int i = 0; i < gSphereCount; i++)
	{
		RayVSSphereDistance(gSpheres[i], r, t0);
		if (t0 > 0.0f && t0 < dist)
			return;
	}
	//for (i = 0; i < gTriangleCount; i++)
	//{
	//	RayVSTriangleDistance(gTriangles[i], r, t0);
	//	if (t0 > 0.0f && t0 < dist)
	//		return;
	//}
	float divby = (dist / pointlight.range) + 1.0f;
	float attenuation = pointlight.intensity / (divby * divby);
	diffuse += saturate(NdL * pointlight.color * attenuation);
	float3 halfVector = normalize(toLight + normalize(gCamPos - origin));
	float NdH = saturate(dot(normal, halfVector));
	if(NdH > 0.0f)
		specular += saturate(pointlight.color * pow(NdH, 96.0f) * attenuation);
	
}

RWTexture2D<float4> output : register(u0);

[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID )
{
	if (threadID.x > gWidth || threadID.y > gHeight)
		return;

	float3 rayPos = gCamPos + gCamDir * gCamFar;
	rayPos += gCamRight * ((threadID.x - gWidth / 2.0f) / gWidth) * (gCamFar / tan(gFOV / 2.0f));
	rayPos += -gCamUp * ((threadID.y - gHeight / 2.0f) / gHeight) * (gCamFar / gAspectRatio);

	Ray r;
	r.o = gCamPos;
	r.d = normalize(rayPos - gCamPos);

	
	float3 accumulatedDiff = float3(0.0f, 0.0f, 0.0f);
	float3 accumulatedSpec = float3(0.0f, 0.0f, 0.0f);
	for (int bounces = 0; bounces < gBounceCount + 1; bounces++)
	{
		float3 intersectionNormal = r.d;
		float3 intersectionPoint = r.o;
		float intersectionDistance = -1.0f;
		for (int i = 0; i < gSphereCount; i++)
		{
			RayVSSphere(gSpheres[i], r, intersectionDistance, intersectionNormal);
		}

		float dduu = 0.0f;
		float ddvv = 0.0f;
		for (i = 0; i < gTriangleCount; i++)
		{
			RayVSTriangle(gTriangles[i], r, intersectionDistance, dduu, ddvv, intersectionNormal);
		}

		intersectionPoint += r.d * intersectionDistance;

		if(intersectionDistance < 0.0f)
			break;

		float3 ldiffuse = float3(0.0f, 0.0f, 0.0f);
		float3 lspec = float3(0.0f, 0.0f, 0.0f);
		for (int i = 0; i < gPointLightCount; i++)
		{
			PointLightContribution(intersectionPoint, intersectionNormal, gPointLights[i], lspec, ldiffuse);
		}

		accumulatedDiff += ldiffuse * (pow(0.8f, bounces + 1) / (bounces + 1)) / intersectionDistance;
		accumulatedSpec += lspec * (pow(0.8f, bounces + 1) / (bounces + 1)) / intersectionDistance;

		r.o = intersectionPoint;
		r.d = normalize(r.d - 2.0f * dot(r.d, intersectionNormal) * intersectionNormal);
		r.o += r.d * 0.0001f; //Get rid of pesky floating point rounding errors :^)
	}

	output[threadID.xy] = saturate(float4(float3(1.0f, 1.0f, 1.0f) * (accumulatedDiff + accumulatedSpec), 1.0f));
}