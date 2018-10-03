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

   float4 modelPosition = float4(vertex.position, 1.f);
   float4 worldPosition = mul(modelPosition, MODEL);
   float4 viewPosition = mul(worldPosition, VIEW);
   float4 clipPosition = mul(viewPosition, PROJECTION);

   out_data.position = clipPosition;
   out_data.uv = vertex.uv;
   out_data.color = vertex.color;
   return out_data;
}

// COLOR (PIXEL/FRAGMENT) FUNCTION
float4 FragmentFunction( vertex_to_fragment_t data ) : SV_Target0
{
   float4 final_color = tTexture.Sample( sSampler, data.uv );
   //float4 sample = float4(1.f, 1.f, 1.f, sampledColor.x);
   //float4 final_color = mul(data.color, sample);
   //vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
   //color = vec4(textColor, 1.0) * sampled;
    

   final_color = data.color * final_color;
   return final_color;
}




