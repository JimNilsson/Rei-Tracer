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

};

//cbuffer CameraBuffer : register(b0)
//{
//	CameraInfo camera;
//}

cbuffer Counts : register(b1)
{
	int gSphereCount;
	int gPlaneCount;
	int gPointLightCount;
	int gOBBCount;
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

StructuredBuffer<Sphere> gSpheres : register(t0);
StructuredBuffer<Plane> gPlanes : register(t1);

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

RWTexture2D<float4> output : register(u0);

[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID )
{
	CameraInfo camera;
	camera.position = float3(0.0f, 0.0f, 0.0f);
	camera.direction = float3(0.0f, 0.0f, -1.0f);
	camera.fov = 3.14 / 2.0f;
	camera.aspectratio = 1.0f;
	camera.fardist = 50.0f;

	Sphere sphere;
	sphere.position = float3(4.0f, 2.0f, -20.0f);
	sphere.radius = 4;

	float3 rayOrigin = camera.position;
	float2 pixel;
	float3 rayPos = camera.position + camera.direction * camera.fardist;
	rayPos.x += ((threadID.x - 200.0f) / 400.0f) * (camera.fardist / tan(camera.fov / 2.0f));
	rayPos.y += ((threadID.y - 200.0f) / 400.0f) * (camera.fardist / camera.aspectratio);
	float3 rayDirection = normalize(rayPos - camera.position);

	Ray r;
	r.o = camera.position;
	r.d = rayDirection;
	float t0, t1;
	float closestf = 9999999.0f;
	int closest = -1;
	for (int i = 0; i < gSphereCount; i++)
	{
		RayVSSphere(gSpheres[i], r, t0, t1);
		if (t0 > 0.0f && t0 < closestf)
		{
			closestf = t0;
			closest = i;
		}
	}

	if (closest >= 0)
	{
		float3 intersection = rayDirection * closestf;
		float3 normal = normalize(intersection - gSpheres[closest].position);
		output[threadID.xy] = float4(normal.xyz, 1.0f);
	}
	else
		output[threadID.xy] = float4(rayDirection.xyz, 1.0f);
		
	//output[threadID.xy] = float4(float3(1,0,1) * groupThreadID.x / 32.0f, 1);
}