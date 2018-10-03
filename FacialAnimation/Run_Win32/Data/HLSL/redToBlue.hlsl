struct vertex_in_t
{
   float3 position : POSITION;
   float2 uv : UV;
   float4 color : COLOR;
};

struct vertex_to_fragment_t
{
   float4 position : SV_Position;
   float2 uv : UV;
   float4 color : COLOR; 
};


Texture2D <float4> tTexture : register(t0);
SamplerState sSampler : register(s0);

cbuffer matrix_cb : register(b0)
{
   float4x4 MODEL;
   float4x4 VIEW;
   float4x4 PROJECTION;
};

cbuffer time_cb : register(b1)
{
   float GAME_TIME;
   float SYSTEM_TIME;
   float GAME_FRAME_TIME;
   float SYSTEM_FRAME_TIME;
};

// NOP VERTEX FUNCTION
vertex_to_fragment_t VertexFunction( vertex_in_t vertex ) 
{
   vertex_to_fragment_t out_data = (vertex_to_fragment_t)0;

   float4 transformedPosition = mul( float4(vertex.position, 1.f), PROJECTION);

   out_data.position = transformedPosition;
   out_data.uv = vertex.uv;
   out_data.color = vertex.color;
   return out_data;
}

// COLOR (PIXEL/FRAGMENT) FUNCTION
float4 FragmentFunction( vertex_to_fragment_t data ) : SV_Target0
{
   float4 sampledColor = tTexture.Sample( sSampler, data.uv );
   float3 reverse = float3(sampledColor.y, sampledColor.z, sampledColor.x);
   float4 final_color = float4(reverse, 1.f);
   final_color = data.color * final_color;
   return final_color;
}
  