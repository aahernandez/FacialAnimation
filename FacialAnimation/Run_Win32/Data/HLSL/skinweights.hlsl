struct vertex_in_t
{
   float3 position      : POSITION;
   float2 uv            : UV;
   float4 color         : COLOR;
   uint4 bone_indices   : BONE_INDICES;
   float4 bone_weights  : BONE_WEIGHTS;
};

struct vertex_to_fragment_t
{
   float4 position      : SV_Position;
   float2 uv            : UV;
   float4 color         : COLOR; 
   uint4 bone_indices   : BONE_INDICES;
   float4 bone_weights  : BONE_WEIGHTS;
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

   float4 modelPosition = float4(vertex.position, 1.f);
   float4 worldPosition = mul(modelPosition, MODEL);
   float4 viewPosition = mul(worldPosition, VIEW);
   float4 clipPosition = mul(viewPosition, PROJECTION);

   out_data.position = clipPosition;
   out_data.uv = vertex.uv;
   out_data.color = vertex.color;
   out_data.bone_indices = vertex.bone_indices;
   out_data.bone_weights = vertex.bone_weights;
   return out_data;
}

// COLOR (PIXEL/FRAGMENT) FUNCTION
float4 FragmentFunction( vertex_to_fragment_t data ) : SV_Target0
{
   return float4(data.bone_weights.xyz, 1.0f);
}




