struct VertexInput
{
   float3 position : POSITION;
   float2 uv0 : UV0;
};

struct VertexToFragment
{
   float4 position : SV_Position;
   float2 uv : UV;
};

Texture2D <float4> tTexture : register(t0);
SamplerState sSampler : register(s0);

VertexToFragment VertexFunction(VertexInput input)
{
    VertexToFragment v2f = (VertexToFragment)0;
    float4 final_pos = float4( input.position, 1.0f );
    v2f.position = final_pos;
    v2f.uv = input.uv0;
    return v2f;
}

float4 FragmentFunction(VertexToFragment input) : SV_Target0
{
   float4 color = tTexture.Sample( sSampler, input.uv );
   
   float reverse_r = color.z;
   float reverse_g = color.x;
   float reverse_b = color.y;
   float3 reverse = float3( 
      reverse_r, reverse_g, reverse_b
   );

   float4 final_color = float4( reverse, 1.0f );
   return final_color;
}