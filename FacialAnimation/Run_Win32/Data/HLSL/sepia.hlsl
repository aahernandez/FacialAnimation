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
   float4 diffuse = tTexture.Sample( sSampler, input.uv );
   
   float3 sepia_r = float3( 0.393f, 0.769f, 0.189f );
   float3 sepia_g = float3( 0.349f, 0.686f, 0.168f );
   float3 sepia_b = float3( 0.272f, 0.534f, 0.131f );
   float3 sepia = float3( 
      dot( diffuse.xyz, sepia_r ),
      dot( diffuse.xyz, sepia_g ),
      dot( diffuse.xyz, sepia_b )
   );

   float4 final_color = float4( sepia, 1.0f );
   return final_color;
}