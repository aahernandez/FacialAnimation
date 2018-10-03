struct vertex_in_t
{
   float3 position : POSITION;
   float2 uv : UV;
};

struct vertex_to_fragment_t
{
   float4 position : SV_Position; 
   float2 uv : UV;
};

Texture2D <float4> tTexture : register(t0);
SamplerState sSampler : register(s0);

// NOP VERTEX FUNCTION
vertex_to_fragment_t VertexFunction( vertex_in_t vertex ) 
{
   vertex_to_fragment_t out_data = (vertex_to_fragment_t)0;
   out_data.position = float4( vertex.position, 1.0f );
   out_data.uv = vertex.uv;
   return out_data;
}

// COLOR (PIXEL/FRAGMENT) FUNCTION
float4 FragmentFunction( vertex_to_fragment_t data ) : SV_Target0
{
   float4 color = tTexture.Sample( sSampler, data.uv );

   float3 desaturate = float3(0.3, 0.6, 0.1);
   float g = dot( desaturate, color.xyz );
   float4 final_color = color;
   final_color.rgb = lerp(g, final_color.rgb, 2);
   
   return final_color;
}