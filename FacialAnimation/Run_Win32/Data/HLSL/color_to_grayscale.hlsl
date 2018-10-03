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

// MAX SIZE: 4096 Elements (float4), 16 KB
// ALIGNMENT:  must be aligned to 16B,
cbuffer time_cb : register(b1) 
{
   float TIME;
   
   float3 PADDING;
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
   // ?  Sample Color at Texture Coordinate data.uv
   float4 color = tTexture.Sample( sSampler, data.uv );

   // ?
   float t = frac(TIME); // returns decimal part of number

   // ? Gets luminosity (grascale)
   float3 desaturate = float3(0.3, 0.59, 0.11);
   float g = dot( desaturate, color.xyz );
   float4 grayscale = float4( g, g, g, 1 );

   // ? Interpolates based on time
   float4 final_color = lerp( color, grayscale, sin(TIME) );
   
   return final_color;
}




