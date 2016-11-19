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
	int gPlaneCount;
	int gPointLightCount;
	int gOBBCount;
	int pad0;
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

StructuredBuffer<Sphere> gSpheres : register(t0);
StructuredBuffer<Triangle> gTriangles : register(t1);
StructuredBuffer<Plane> gPlanes : register(t2);

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
	distance = -1.0f;
	u = 0.0f;
	v = 0.0f;
	float3 e1 = t.v2.position - t.v1.position;
	float3 e2 = t.v3.position - t.v1.position;
	float3 q = cross(r.d, e2);
	float a = dot(e1, q); //The determinant of the matrix (-direction e1 e2)
	if (a > -0.0001f && a < 0.0001f)
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
	u = bu * t.v2.u + bv * t.v3.u + (1.0f - bv - bu) * t.v1.u;
	v = bu * t.v2.v + bv * t.v3.v + (1.0f - bv - bu) * t.v1.v;
	normal = bu * t.v2.normal + bv * t.v3.normal + (1.0f - bv - bu) * t.v1.normal;
}

RWTexture2D<float4> output : register(u0);

[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID )
{
	CameraInfo gCamera;
	//gCamera.position = float3(10.0f, 0.0f, 40.0f);
	gCamera.position = gCamPos;
	//gCamera.direction = float3(0.0f, 0.0f, -1.0f);
	gCamera.direction = gCamDir;
	gCamera.fov = gFOV;
	//gCamera.fov = ggCamera.fov;
	gCamera.aspectratio = 1.0f;
	gCamera.fardist = 150.0f;
	gCamera.neardist = 1.0f;
	gCamera.width = 400.0f;
	gCamera.height = 400.0f;
	//output[threadID.xy] = float4(gCamera.position.xyz, 1.0f);

	float3 rayPos = gCamera.position + gCamera.direction * gCamera.fardist;
	rayPos += gCamRight * ((threadID.x - gCamera.width / 2.0f) / gCamera. width) * (gCamera.fardist / tan(gCamera.fov / 2.0f));
	rayPos += gCamUp * ((threadID.y - gCamera.height / 2.0f) / gCamera.height) * (gCamera.fardist / gCamera.aspectratio);
	float3 rayDirection = normalize(rayPos - gCamera.position);
//	output[threadID.xy] = float4(rayDirection.xyz, 1.0f);

	Ray r;
	r.o = gCamera.position;
	r.d = rayDirection;
	float t0, t1;
	float spheredistance = gCamera.fardist + 1.0f;
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

	float planedistance = gCamera.fardist + 1.0f;
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

	float triangledist = -1.0f;
	Triangle t;
	t.v1.position = float3(0.0f, 0.0f, 0.0f);
	t.v2.position = float3(10.0f, 0.0f, 0.0f);
	t.v3.position = float3(10.0f, 0.0f, -10.0f);
	t.v1.normal = float3(0.0f, 1.0f, 0.0f);
	t.v2.normal = float3(0.0f, 0.0f, 1.0f);
	t.v3.normal = float3(1.0f, 0.0f, 0.0f);
	t.v1.u = 0.0f;
	t.v1.v = 0.0f;
	t.v2.u = 1.0f;
	t.v2.v = 0.0f;
	t.v3.u = 1.0f;
	t.v3.v = 1.0f;

	float dduu = 0.0f;
	float ddvv = 0.0f;
	float3 narmal = float3(0.0f, 0.0f, 0.0f);

	RayVSTriangle(t, r, triangledist, dduu, ddvv, narmal);


	if (spheredistance < planedistance && sphereindex >= 0)
	{
		float3 intersection = gCamera.position + rayDirection * spheredistance;
		float3 normal = normalize(intersection - gSpheres[sphereindex].position);
		output[threadID.xy] = float4(normal.xyz, 1.0f);
	}
	else if(planedistance < spheredistance && planeindex >= 0)
		output[threadID.xy] = float4(gPlanes[planeindex].normal, 1.0f);
	else if(triangledist > 0.0f)
		output[threadID.xy] = float4(dduu,ddvv,0.0f, 1.0f);
	else
		output[threadID.xy] = float4(rayDirection.xyz, 1.0f);

}