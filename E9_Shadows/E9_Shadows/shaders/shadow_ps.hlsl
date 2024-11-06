
Texture2D shaderTexture : register(t0);
Texture2D depthMapTexture[2] : register(t1);

SamplerState diffuseSampler : register(s0);
SamplerState shadowSampler[2] : register(s1);

cbuffer LightBuffer : register(b0)
{
    float4 ambient[2];
    float4 diffuse[2];
    float4 direction[2];
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float4 lightViewPos[2] : TEXCOORD1;
};

// Calculate lighting intensity based on direction and normal. Combine with light colour.
float4 calculateLighting(float3 lightDirection, float3 normal, float4 diffuse)
{
    float intensity = saturate(dot(normal, lightDirection));
    float4 colour = saturate(diffuse * intensity);
    return colour;
}

// Is the gemoetry in our shadow map
bool hasDepthData(float2 uv)
{
    if (uv.x < 0.f || uv.x > 1.f || uv.y < 0.f || uv.y > 1.f)
    {
        return false;
    }
    return true;
}

bool isInShadow(Texture2D sMap, float2 uv, float4 lightViewPosition, float bias, int i)
{
    // Sample the shadow map (get depth of geometry)
    float depthValue = sMap.Sample(shadowSampler[i], uv).r;
	// Calculate the depth from the light.
    float lightDepthValue = lightViewPosition.z / lightViewPosition.w;
    lightDepthValue -= bias;

	// Compare the depth of the shadow map value and the depth of the light to determine whether to shadow or to light this pixel.
    if (lightDepthValue < depthValue)
    {
        return false;
    }
    return true;
}

float2 getProjectiveCoords(float4 lightViewPosition)
{
    // Calculate the projected texture coordinates.
    float2 projTex = lightViewPosition.xy / lightViewPosition.w;
    projTex *= float2(0.5, -0.5);
    projTex += float2(0.5f, 0.5f);
    return projTex;
}

float4 main(InputType input) : SV_TARGET
{
    float shadowMapBias = 0.005f;
    float4 colour = float4(0.f, 0.f, 0.f, 1.f);
    float4 textureColour = shaderTexture.Sample(diffuseSampler, input.tex);

    for (int i = 0; i < 2; ++i)
    {
        // Calculate the projected texture coordinates.
        float2 pTexCoord = getProjectiveCoords(input.lightViewPos[i]);
	
        // Shadow test. Is or isn't in shadow
        if (hasDepthData(pTexCoord))
        {
            // Has depth map data
            if (!isInShadow(depthMapTexture[i], pTexCoord, input.lightViewPos[i], shadowMapBias, i))
            {
                // is NOT in shadow, therefore light
                colour += calculateLighting(-direction[i].xyz, input.normal, diffuse[i]);
            }
        }
    
        colour += ambient[i];
    }
    return saturate(colour) * textureColour;
}