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

cbuffer zoom_cb : register(b2) 
{
	float ZOOM;

	float3 PADDING;
};

// NOP VERTEX FUNCTION
vertex_to_fragment_t VertexFunction( vertex_in_t vertex ) 
{
	vertex_to_fragment_t out_data = (vertex_to_fragment_t)0;

	float4 final_pos = float4( vertex.position, 1.0f );
	out_data.position = final_pos;
	out_data.uv = vertex.uv;
	return out_data;
}

// COLOR (PIXEL/FRAGMENT) FUNCTION
float4 FragmentFunction( vertex_to_fragment_t data ) : SV_Target0
{
	float4 color = tTexture.Sample(sSampler, data.uv);

	float screenRatio = 1280.f / 720.f;
	float dx = (data.uv.x - 0.5f) * screenRatio;
	float dy = data.uv.y - 0.5f;

	if( dx * dx + dy * dy < 0.1f)
	{
		float4 color = tTexture.Sample(sSampler, (data.uv * ZOOM) + ((1.f - ZOOM)) * 0.5f);
		return color;
	}
	else if ( dx * dx + dy * dy < 0.102f)
	{
		return float4(1.f, 0.f, 0.f, 1.f);
	}
	else
	{
		color.g = color.g * sin(data.uv.y * 1000 - GAME_TIME) * 2;
		color.b = color.b * sin(data.uv.y * 1000 - GAME_TIME) * 2;
		color.r = color.r * sin(data.uv.y * 1000 - GAME_TIME) * 2;
		return color;
	}

}