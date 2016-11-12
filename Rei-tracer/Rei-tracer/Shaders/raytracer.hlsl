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

struct Sphere
{
	float3 position;
	float radius;
};
struct Ray
{
	float3 o;
	float3 d;
};

float RayVSSphere(Sphere s, Ray r)
{
	float3 l = s.position - r.o;
	float tca = -dot(l, r.d);
	if (tca < 0.0f)
		return -1.0f;
	float d2 = dot(l, l) - tca*tca;
	float radius2 = s.radius * s.radius;
	if (d2 > radius2)
		return -1.0f;
	float thc = sqrt(radius2 - d2);
	return tca - thc;
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

	Sphere testsphere;
	testsphere.position = float3(-0.0f, 5.0f, -25.0f);
	testsphere.radius = 4.0f;

	float3 rayOrigin = camera.position;
	float3 rayDirection = camera.direction;
	float2 pixel;
	pixel.x = threadID.x;
	pixel.y = threadID.y;
	float3 farAway = camera.position + camera.direction * camera.fardist;
	float3 rayPos = farAway;
	rayPos.x += ((pixel.x - 200.0f) / 400.0f) * (camera.fardist / tan(camera.fov / 2.0f));
	rayPos.y += ((pixel.y - 200.0f) / 400.0f) * (camera.fardist / camera.aspectratio);
	
	rayDirection = normalize(rayPos - camera.position);

	Ray r;
	r.o = rayPos;
	r.d = rayDirection;
	float dist = RayVSSphere(testsphere, r);
	if (dist > 0.0f)
	{
		float3 intersection = rayDirection * dist;
		float3 normal = normalize(intersection - testsphere.position);
		output[threadID.xy] = float4(normal, 1.0f);
	}
	else
		output[threadID.xy] = float4(rayDirection.xyz, 1.0f);
		
	//output[threadID.xy] = float4(float3(1,0,1) * groupThreadID.x / 32.0f, 1);
}