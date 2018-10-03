struct vertex_in_t
{
    float3 position     : POSITION;
    float3 normal       : NORMAL;
    float2 uv           : UV;
    float4 color        : COLOR;
    float3 tangent      : TANGENT;
    uint4 bone_indices  : BONE_INDICES;
    float4 bone_weights : BONE_WEIGHTS;
    uint index           : INDEX;
};

struct vertex_to_fragment_t
{
    float4 position     : SV_Position;
    float3 normal       : NORMAL;
    float2 uv           : UV;
    float4 color        : COLOR;
    float3 tangent      : TANGENT;
    uint4 bone_indices  : BONE_INDICES;
    float4 bone_weights : BONE_WEIGHTS;
    uint index           : INDEX;

    float3 worldPosition : WORLD;
};


Texture2D<float4> tTexture : register(t0);
Texture2D<float4> tNormal : register(t1);
StructuredBuffer<uint> tIndices : register(t2);
StructuredBuffer<float> tWeights : register(t3);
StructuredBuffer<float3> tMorphTargets : register(t4);
StructuredBuffer<float4x4> tSkinMatrices : register(t5);
SamplerState sSampler : register(s0);

cbuffer matrix_cb : register(b0)
{
    float4x4 MODEL;
    float4x4 VIEW;
    float4x4 PROJECTION;
    float4 EYE_POSITION;
};

cbuffer time_cb : register(b1)
{
    float GAME_TIME;
    float SYSTEM_TIME;
    float GAME_FRAME_TIME;
    float SYSTEM_FRAME_TIME;
};

struct PointLight
{
    float4 LIGHT_COLOR;
    float4 LIGHT_POSITION;
    float4 ATTENUATION;
    float4 SPEC_ATTENUATION;
};

cbuffer light_cb : register(b2)
{
    PointLight pointLights[8];

    float4 AMBIENT;
    float SPEC_FACTOR;
    float SPEC_POWER;
    float2 LIGHT_PADDING;
};

cbuffer blendShape_cb : register(b3)
{
    uint NUM_BLEND_SHAPES;
    uint PADDING1;
    uint PADDING2;
    uint PADDING3;
};


// NOP VERTEX FUNCTION
vertex_to_fragment_t VertexFunction(vertex_in_t vertex)
{
    vertex_to_fragment_t out_data = (vertex_to_fragment_t) 0;

    float weightSum = 0.f;
    for (uint count = 0; count < NUM_BLEND_SHAPES; count++)
    {
        weightSum += tWeights[count];
    }

    float3 localPos = vertex.position;

    for (uint bsCount = 0; bsCount < NUM_BLEND_SHAPES; bsCount++)
    {
        uint index = tIndices[bsCount] + vertex.index;
        localPos += (tMorphTargets[index] * tWeights[bsCount]);
    }

    float4 modelPosition = float4(localPos, 1.f);
    float4 worldPosition = mul(modelPosition, MODEL);
    float4 viewPosition = mul(worldPosition, VIEW);
    float4 clipPosition = mul(viewPosition, PROJECTION);

    out_data.position = clipPosition;
    out_data.normal = vertex.normal;
    out_data.uv = vertex.uv;
    out_data.color = vertex.color;
    out_data.tangent = vertex.tangent;
    out_data.bone_indices = vertex.bone_indices;
    out_data.bone_weights = vertex.bone_weights;
    out_data.worldPosition = worldPosition.xyz;
    return out_data;
}

// COLOR (PIXEL/FRAGMENT) FUNCTION
float4 FragmentFunction(vertex_to_fragment_t data) : SV_Target0
{
    float4 color = tTexture.Sample(sSampler, data.uv);
    float4 diffuse = color * data.color;
   
    float3 normal_color = tNormal.Sample(sSampler, data.uv).xyz;
    float3 surface_normal = normal_color * float3(2, 2, -1) - float3(1, 1, 0);
  
    float3 normal = surface_normal;
    normal = normalize(normal);

    float3 tan = data.tangent;
    float3 bitangent = normalize(cross(tan, normal));
    tan = normalize(cross(normal, bitangent));

    float3x3 tbn =
    {
        tan,
        bitangent,
        normal
    };

    surface_normal = mul(surface_normal, tbn);
    surface_normal = normalize(surface_normal);

    float3 vector_to_eye = EYE_POSITION.xyz - data.worldPosition;
    float3 eye_vector = -normalize(vector_to_eye);

    float4 ambient_factor = float4(AMBIENT.xyz * AMBIENT.w, 1.0f);

    float4 light_diffuse_factor;
    float4 spec_color;

    for (int counter = 0; counter < 8; counter++)
    {
        float3 vector_to_light = pointLights[counter].LIGHT_POSITION.xyz - data.worldPosition;
        float distance_to_light = length(vector_to_light);
        float3 dir_to_light = vector_to_light / distance_to_light;

        float dot3 = saturate(dot(dir_to_light, surface_normal));
        float attenuation = pointLights[counter].LIGHT_COLOR.w / (pointLights[counter].ATTENUATION.x 
      + distance_to_light * pointLights[counter].ATTENUATION.y
      + distance_to_light * distance_to_light * pointLights[counter].ATTENUATION.z);

        attenuation = saturate(attenuation);

        float4 light_color = float4(pointLights[counter].LIGHT_COLOR.xyz, 1.0f);
        light_diffuse_factor += light_color * dot3 * attenuation;

        float spec_attenuation = pointLights[counter].LIGHT_COLOR.w / (pointLights[counter].SPEC_ATTENUATION.x 
      + distance_to_light * pointLights[counter].SPEC_ATTENUATION.y
      + distance_to_light * distance_to_light * pointLights[counter].SPEC_ATTENUATION.z);
   
        float3 ref_light_dir = reflect(dir_to_light, surface_normal);
        float spec_dot3 = saturate(dot(ref_light_dir, eye_vector));

        float spec_factor = spec_attenuation * SPEC_FACTOR * pow(spec_dot3, SPEC_POWER);
        spec_color += spec_factor * light_color;
    }

    float4 diffuse_factor = saturate(ambient_factor + light_diffuse_factor);

    float4 final_color = diffuse_factor * diffuse + spec_color;

    return final_color;
}




