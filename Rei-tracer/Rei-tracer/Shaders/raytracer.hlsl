

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
	int gTextureCount;
	int gSpotLightCount;
	int gMeshIndexCount;
	int gMeshPartitionCount;
};

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

struct SpotLight
{
	float3 position;
	float intensity;
	float3 color;
	float range;
	float3 dir;
	float cone;
};

struct TriangleTexture
{
	uint lowerIndex;
	uint upperIndex;
	int diffuseIndex;
	int normalIndex;
};

struct MeshPartition
{
	float3 position;
	int lowerIndex;
	float3 halflengths;
	int upperIndex;
};

struct MeshIndices
{
	int lowerIndex;
	int upperIndex;
	int rootPartition;
	int partitionCount;
};

struct Box
{
	float3 min;
	float3 max;
};


StructuredBuffer<Sphere> gSpheres : register(t0);
StructuredBuffer<Triangle> gTriangles : register(t1);
StructuredBuffer<PointLight> gPointLights : register(t2);
StructuredBuffer<TriangleTexture> gTriangleTextureIndices : register(t3);
Texture2DArray gMeshTextures : register(t4);
StructuredBuffer<SpotLight> gSpotLights : register(t5);
StructuredBuffer<MeshIndices> gMeshIndices : register(t6);
StructuredBuffer<MeshPartition> gMeshPartitions : register(t7);


SamplerState gSampleLinear : register(s0);

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



void RayVSTriangle(Triangle t, Ray r, inout float dist, inout float u, inout float v, inout float3 normal, out float4 tangent)
{

	float3 e1 = t.v2.position - t.v1.position;
	float3 e2 = t.v3.position - t.v1.position;
	float3 q = cross(r.d, e2);
	float a = dot(e1, q); //The determinant of the matrix (-direction e1 e2)
	if (a < 0.0001f)
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
	if (ttt > 0.0f && (ttt < dist || dist < 0))
	{
		dist = ttt;
		u = bu * t.v2.u + bv * t.v3.u + (1.0f - bv - bu) * t.v1.u;
		v = bu * t.v2.v + bv * t.v3.v + (1.0f - bv - bu) * t.v1.v;
		normal = normalize(bu * t.v2.normal + bv * t.v3.normal + (1.0f - bv - bu) * t.v1.normal);
		tangent = normalize(bu * t.v2.tangent + bv * t.v3.tangent + (1.0f - bv - bu) * t.v1.tangent);
	}
}

//Used for checking occlusion of lights
void RayVSTriangleDistance(Triangle t, Ray r, out float dist)
{
	dist = -1.0f;
	float3 e1 = t.v2.position - t.v1.position;
	float3 e2 = t.v3.position - t.v1.position;
	float3 q = cross(r.d, e2);
	float a = dot(e1, q); //The determinant of the matrix (-direction e1 e2)
	if (a < 0.0001f)
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
	dist = f * dot(e2, rr);
}

bool RayVSBox(Ray r, float3 rcpDir, Box b)
{
	float tx1 = (b.min.x - r.o.x)*rcpDir.x;
	float tx2 = (b.max.x - r.o.x)*rcpDir.x;
	float tmin = min(tx1, tx2);
	float tmax = max(tx1, tx2);

	float ty1 = (b.min.y - r.o.y)*rcpDir.y;
	float ty2 = (b.max.y - r.o.y)*rcpDir.y;
	tmin = min(ty1, ty2);
	tmax = max(ty1, ty2);

	float tz1 = (b.min.z - r.o.z)*rcpDir.z;
	float tz2 = (b.max.z - r.o.z)*rcpDir.z;
	tmin = min(tz1, tz2);
	tmax = max(tz1, tz2);

	return tmax >= max(tmin, 0.0f);
}

void TraverseOctTree(Ray r, inout float dist, inout float u, inout float v, inout int triangleIndex, inout float3 normal, out float4 tangent, float3 rcpDir)
{
	//for (int i = 0; i < gTriangleCount; i++)
	//{
	//	float previous = dist;
	//	RayVSTriangle(gTriangles[i], r, dist, u, v, normal, tangent);
	//	if (dist < previous)
	//	{
	//		triangleIndex = i;
	//	}
	//}
	//For each MeshIndices traverse octtree if any, else traverse triangles
	float previous = dist;
	for (int i = 0; i < gMeshIndexCount; i++)
	{
		if (gMeshIndices[i].rootPartition >= 0)
		{
			//We have an octtree to traverse
			//No recursion in hlsl, we'll have to use a stack
			int stack[73];
			int stackPtr = 0;
			int nodeIndex = gMeshIndices[i].rootPartition;
			

			stack[stackPtr] = nodeIndex;
			stackPtr++;

			while (stackPtr)
			{
				nodeIndex = stack[stackPtr - 1];
				stackPtr--;

				Box b;
				b.min = gMeshPartitions[nodeIndex].position - gMeshPartitions[nodeIndex].halflengths;
				b.max = gMeshPartitions[nodeIndex].position + gMeshPartitions[nodeIndex].halflengths;

				if (RayVSBox(r, rcpDir, b))
				{
					if (gMeshIndices[i].partitionCount >= (nodeIndex - gMeshIndices[i].rootPartition) * 8 + 8 + 1)
					{
						[unroll]
						for (int c = 1; c < 9; c++)
						{
							stack[stackPtr++] = ((nodeIndex - gMeshIndices[i].rootPartition) * 8) + c;
						}
					}
					for (int c = gMeshPartitions[nodeIndex].lowerIndex; c < gMeshPartitions[nodeIndex].upperIndex; c++)
					{
						previous = dist;
						RayVSTriangle(gTriangles[c], r, dist, u, v, normal, tangent);
						if (dist < previous)
						{
							triangleIndex = c;
						}
					}
				}
			}
		}
		else
		{
			//We check the triangles in this range of triangles
			for (int j = gMeshIndices[i].lowerIndex; j < gMeshIndices[i].upperIndex; j++)
			{
				previous = dist;
				RayVSTriangle(gTriangles[j], r, dist, u, v, normal, tangent);
				if (dist < previous)
				{
					triangleIndex = j;
				}
			}
		}
	}
}

bool TraverseOctTreeForShadows(Ray r, float dist, float3 rcpDir)
{
	//float t0;
	//for (int i = 0; i < gTriangleCount; i++)
	//{
	//	RayVSTriangleDistance(gTriangles[i], r, t0);
	//	if (t0 > 0.0f && t0 < dist)
	//		return true;
	//}
	//return false;
	//For each MeshIndices traverse octtree if any, else traverse triangles
	
	float comp = -1.0f;
	for (int i = 0; i < gMeshIndexCount; i++)
	{
		if (gMeshIndices[i].rootPartition >= 0)
		{
			//We have an octtree to traverse
			//No recursion in hlsl, we'll have to use a stack
			int stack[73];
			int stackPtr = 0;
			int nodeIndex = gMeshIndices[i].rootPartition;
			

			stack[stackPtr] = nodeIndex;
			stackPtr++;

			while (stackPtr)
			{
				nodeIndex = stack[stackPtr - 1];
				stackPtr--;

				Box b;
				b.min = gMeshPartitions[nodeIndex].position - gMeshPartitions[nodeIndex].halflengths;
				b.max = gMeshPartitions[nodeIndex].position + gMeshPartitions[nodeIndex].halflengths;

				if (RayVSBox(r, rcpDir, b))
				{
					if (gMeshIndices[i].partitionCount >= (nodeIndex - gMeshIndices[i].rootPartition) * 8 + 8 + 1)
					{
						[unroll]
						for (int c = 1; c < 9; c++)
						{
							stack[stackPtr++] = ((nodeIndex - gMeshIndices[i].rootPartition) * 8) + c;
						}
					}
					for (int c = gMeshPartitions[nodeIndex].lowerIndex; c < gMeshPartitions[nodeIndex].upperIndex; c++)
					{
						RayVSTriangleDistance(gTriangles[c], r, comp);
						if (comp < dist && comp > 0.0f)
						{
							return true;
						}
					}
				}
			}
		}
		else
		{
			//We check the triangles that arent partitioned into an octtree
			for (int j = gMeshIndices[i].lowerIndex; j < gMeshIndices[i].upperIndex; j++)
			{
				RayVSTriangleDistance(gTriangles[j], r, comp);
				if (comp < dist && comp > 0.0f)
				{
					return true;
				}
			}
		}
	}
	return false;
}

void SpotLightContribution(float3 rayOrigin, float3 origin, float3 normal, SpotLight spotlight, inout float3 specular, inout float3 diffuse, float3 rcpDir)
{
	float3 toLight = spotlight.position - origin;
	float dist = length(toLight);
	toLight /= dist;
	float NdL = dot(toLight, normal);
	if (NdL < 0.0f)
		return;

	//Check if light source is occluded
	Ray r;
	r.o = origin;
	r.d = toLight;
	r.o += 0.0001f * r.d;
	float t0;
	for (int i = 0; i < gSphereCount; i++)
	{
		RayVSSphereDistance(gSpheres[i], r, t0);
		if (t0 > 0.0f && t0 < dist)
			return;
	}
	t0 = -1.0f;
	for (i = 0; i < gTriangleCount; i++)
	{
		RayVSTriangleDistance(gTriangles[i], r, t0);
		if (t0 > 0.0f && t0 < dist)
			return;
	}

	float divby = (dist / spotlight.range) + 1.0f;
	float attenuation = pow(max(dot(-toLight, spotlight.dir), 0.0f),spotlight.cone) * spotlight.intensity / (divby * divby);
	if (attenuation > 0.0f)
	{
		diffuse += NdL * spotlight.color * attenuation;
		float3 halfVector = normalize(toLight + normalize(rayOrigin - origin));
		float NdH = dot(normal, halfVector);
		if (NdH > 0.0f)
			specular += spotlight.color * pow(NdH, 6.0f) * attenuation;
		diffuse += float3(attenuation.xxx);
	}
}

void PointLightContribution(float3 rayOrigin, float3 origin, float3 normal, PointLight pointlight, inout float3 specular, inout float3 diffuse, float3 rcpDir)
{
	float3 toLight = pointlight.position - origin;
	float dist = length(toLight);
	toLight = toLight / dist;
	float NdL = dot(toLight, normal);
	if (NdL < 0.0f)
		return; //No contribution at all, return
	//Check for occlusion (shadows)

	Ray r;
	r.o = origin;
	r.d = toLight;
	r.o += 0.0001f * r.d;

	float t0;
	for (int i = 0; i < gSphereCount; i++)
	{
		RayVSSphereDistance(gSpheres[i], r, t0);
		if (t0 > 0.0f && t0 < dist)
			return;
	}
	t0 = -1.0f;
	/*for (i = 0; i < gTriangleCount; i++)
	{
		RayVSTriangleDistance(gTriangles[i], r, t0);
		if (t0 > 0.0f && t0 < dist)
			return;
	}*/

	if (TraverseOctTreeForShadows(r, dist, rcpDir))
		return;


	float divby = (dist / pointlight.range) + 1.0f;
	float attenuation = pointlight.intensity / (divby * divby);
	diffuse += NdL * pointlight.color * attenuation;
	float3 halfVector = normalize(toLight + normalize(rayOrigin - origin));
	float NdH = dot(normal, halfVector);
	if(NdH > 0.0f)
		specular += pointlight.color * pow(NdH, 6.0f) * attenuation;
	
}



RWTexture2D<float4> output : register(u0);

[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID )
{
	if (threadID.x > gWidth || threadID.y > gHeight)
		return;

	float3 rayPos = gCamPos + gCamDir * gCamFar;
	float nx = (threadID.x - gWidth / 2.0f) / gWidth;
	float ny = (threadID.y - gHeight / 2.0f) / gHeight;

	float dx = 1.0f / gWidth;
	float dy = 1.0f / gHeight;



	float3 fovCorrection = (gCamFar / tan(gFOV / 2.0f)) * gCamRight;
	float3 aspectCorrection = (gCamFar / gAspectRatio) * -gCamUp;
	float3 farplanePositions[9];

	farplanePositions[0] = rayPos + (nx - 0.5f * dx) * fovCorrection + (ny + 0.5f * dy) * aspectCorrection; //Upper left
	farplanePositions[1] = rayPos + (nx - 0.0f * dx) * fovCorrection + (ny + 0.5f * dy) * aspectCorrection; //Upper middle
	farplanePositions[2] = rayPos + (nx + 0.5f * dx) * fovCorrection + (ny + 0.5f * dy) * aspectCorrection; //Upper right
	farplanePositions[3] = rayPos + (nx - 0.5f * dx) * fovCorrection + (ny + 0.0f * dy) * aspectCorrection; //Middle left
	farplanePositions[4] = rayPos + (nx + 0.0f * dx) * fovCorrection + (ny + 0.0f * dy) * aspectCorrection; //Middle
	farplanePositions[5] = rayPos + (nx + 0.5f * dx) * fovCorrection + (ny + 0.0f * dy) * aspectCorrection; //Middle right
	farplanePositions[6] = rayPos + (nx - 0.5f * dx) * fovCorrection + (ny - 0.5f * dy) * aspectCorrection; //Lower left
	farplanePositions[7] = rayPos + (nx - 0.0f * dx) * fovCorrection + (ny - 0.5f * dy) * aspectCorrection; //Lower middle
	farplanePositions[8] = rayPos + (nx + 0.5f * dx) * fovCorrection + (ny - 0.5f * dy) * aspectCorrection; //Lower right
	
	float3 rayDirections[9];
	[unroll]
	for (int k = 0; k < 9; k++)
	{
		rayDirections[k] = normalize(farplanePositions[k] - gCamPos);
	}

	/*rayPos += nx * fovCorrection;
	rayPos += ny * aspectCorrection;*/

	Ray r;
	r.o = gCamPos;
	//r.d = normalize(rayPos - gCamPos);

	
	float3 accumulatedDiff = float3(0.0f, 0.0f, 0.0f);
	float3 accumulatedSpec = float3(0.0f, 0.0f, 0.0f);

	for (int samples = 0; samples < 1; samples++)
	{
		r.d = rayDirections[samples];
		r.o = gCamPos;
		for (int bounces = 0; bounces < gBounceCount + 1; bounces++)
		{
			float3 rcpDir = float3(1.0f, 1.0f, 1.0f) / r.d;
			float3 intersectionNormal = r.d;
			float3 intersectionPoint = r.o;
			float4 intersectionTangent;
			float intersectionDistance = 9999.0f;
			for (int i = 0; i < gSphereCount; i++)
			{
				RayVSSphere(gSpheres[i], r, intersectionDistance, intersectionNormal);
			}

			float dduu = 0.0f;
			float ddvv = 0.0f;
			int triangleIndex = -1;
			//for (i = 0; i < gTriangleCount; i++)
			//{
			//	float previous = intersectionDistance;
			//	RayVSTriangle(gTriangles[i], r, intersectionDistance, dduu, ddvv, intersectionNormal, intersectionTangent);
			//	if (intersectionDistance < previous)
			//	{
			//		triangleIndex = i;
			//	}
			//}
			TraverseOctTree(r, intersectionDistance, dduu, ddvv, triangleIndex, intersectionNormal, intersectionTangent, rcpDir);

			if (intersectionDistance < 0.0f)
				break;

			intersectionPoint += r.d * intersectionDistance;

			float3 texColor = float3(1.0f, 1.0f, 1.0f);
			if (triangleIndex >= 0)
			{
				for (i = 0; i < gTextureCount; i++)
				{
					if (triangleIndex >= gTriangleTextureIndices[i].lowerIndex && triangleIndex <= gTriangleTextureIndices[i].upperIndex)
					{
						texColor = gMeshTextures.SampleLevel(gSampleLinear, float3(dduu, ddvv, gTriangleTextureIndices[i].diffuseIndex), 0).xyz;
						if (gTriangleTextureIndices[i].normalIndex >= 0)
						{
							float3 sampledNormal = gMeshTextures.SampleLevel(gSampleLinear, float3(dduu, ddvv, gTriangleTextureIndices[i].normalIndex), 0).xyz;
							sampledNormal = sampledNormal * 2.0f - 1.0f;
							float3 bitan = intersectionTangent.w * cross(intersectionNormal, intersectionTangent.xyz);
							float3x3 tbn;
							tbn[2] = intersectionTangent.xyz;
							tbn[1] = bitan;
							tbn[0] = intersectionNormal;
							intersectionNormal = normalize(mul(sampledNormal, tbn));
						}
						break;
					}
				}
			}

			float3 ldiffuse = float3(0.0f, 0.0f, 0.0f);
			float3 lspec = float3(0.0f, 0.0f, 0.0f);
			for (i = 0; i < gPointLightCount; i++)
			{
				PointLightContribution(r.o, intersectionPoint, intersectionNormal, gPointLights[i], lspec, ldiffuse,rcpDir);
			}
			for (i = 0; i < gSpotLightCount; i++)
			{
				SpotLightContribution(r.o, intersectionPoint, intersectionNormal, gSpotLights[i], lspec, ldiffuse,rcpDir);
			}
			//CircularLightContribution(r.o, intersectionPoint, intersectionNormal, clight, lspec, ldiffuse);

			accumulatedDiff += ldiffuse * (pow(0.8f, bounces + 1) / (bounces + 1)) * texColor;
			accumulatedSpec += lspec * (pow(0.8f, bounces + 1) / (bounces + 1)) * texColor;

			r.o = intersectionPoint;
			r.d = normalize(r.d - 2.0f * dot(r.d, intersectionNormal) * intersectionNormal);
			r.o += r.d * 0.0001f; //Get rid of pesky floating point rounding errors :^)
		}
	}

	accumulatedDiff /= 1.0f;
	accumulatedSpec /= 1.0f;
	output[threadID.xy] = saturate(float4((accumulatedDiff + accumulatedSpec), 1.0f));
}