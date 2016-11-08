RWTexture2D<float4> output : register(u0);

[numthreads(32, 32, 1)]
void main( uint3 threadID : SV_DispatchThreadID, uint3 groupThreadID : SV_GroupThreadID )
{
	output[threadID.xy] = float4(float3(1,0,1) * groupThreadID.x / 32.0f, 1);
}