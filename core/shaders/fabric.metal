/**
 * ARFitKit Fabric/Cloth Material Shader (Metal)
 * Realistic fabric rendering with subsurface scattering and anisotropic reflection
 */

#include <metal_stdlib>
using namespace metal;

// Fabric types with different optical properties
enum FabricType : uint {
    COTTON = 0,
    SILK = 1,
    DENIM = 2,
    LEATHER = 3,
    VELVET = 4,
    WOOL = 5
};

struct FabricProperties {
    float roughness;
    float anisotropy;       // Directional reflection (thread direction)
    float sheenIntensity;   // Edge highlight
    float subsurface;       // Light penetration
    float3 sheenColor;
    float3 subsurfaceColor;
};

constant FabricProperties fabricPresets[] = {
    // COTTON
    { 0.8, 0.1, 0.2, 0.1, float3(1.0), float3(1.0, 0.95, 0.9) },
    // SILK
    { 0.3, 0.8, 0.8, 0.2, float3(1.0, 0.98, 0.95), float3(1.0) },
    // DENIM
    { 0.9, 0.3, 0.1, 0.05, float3(0.3, 0.4, 0.6), float3(0.2, 0.3, 0.5) },
    // LEATHER
    { 0.6, 0.0, 0.4, 0.0, float3(0.9, 0.85, 0.8), float3(0.0) },
    // VELVET
    { 0.95, 0.0, 0.9, 0.3, float3(1.0), float3(1.0, 0.9, 0.9) },
    // WOOL
    { 0.95, 0.2, 0.3, 0.15, float3(1.0), float3(0.95, 0.9, 0.85) }
};

// Anisotropic GGX distribution
float D_GGX_Anisotropic(
    float NdotH, float HdotT, float HdotB,
    float roughnessT, float roughnessB
) {
    float a2 = roughnessT * roughnessB;
    float3 v = float3(roughnessB * HdotT, roughnessT * HdotB, a2 * NdotH);
    float v2 = dot(v, v);
    float w2 = a2 / v2;
    return a2 * w2 * w2 * (1.0 / 3.14159);
}

// Sheen BRDF (for fabric edge highlight)
float3 sheenBRDF(float3 N, float3 V, float3 L, float3 sheenColor, float roughness) {
    float3 H = normalize(V + L);
    float NdotH = max(dot(N, H), 0.0);
    float LdotH = max(dot(L, H), 0.0);
    
    // Charlie sheen distribution
    float invRoughness = 1.0 / roughness;
    float cos2 = NdotH * NdotH;
    float sin2 = max(1.0 - cos2, 0.0078125);
    float sheen = (2.0 + invRoughness) * pow(sin2, invRoughness * 0.5) / (2.0 * 3.14159);
    
    // Fresnel approximation for sheen
    float VdotN = max(dot(V, N), 0.0);
    float fresnel = pow(1.0 - VdotN, 5.0);
    
    return sheenColor * sheen * fresnel;
}

// Subsurface scattering approximation
float3 subsurfaceScattering(
    float3 N, float3 L, float3 V,
    float3 albedo, float3 subsurfaceColor, float subsurface
) {
    // Wrap lighting for subsurface effect
    float wrapDiffuse = max(0.0, (dot(N, L) + subsurface) / (1.0 + subsurface));
    
    // Back-lighting
    float3 backL = -L + N * 0.5;
    float backLight = saturate(dot(V, normalize(backL)));
    backLight = pow(backLight, 3.0) * subsurface;
    
    return albedo * wrapDiffuse + subsurfaceColor * backLight;
}

// Main fabric fragment shader
fragment float4 fabricFragment(
    VertexOut in [[stage_in]],
    texture2d<float> albedoMap [[texture(0)]],
    texture2d<float> normalMap [[texture(1)]],
    texture2d<float> threadDirectionMap [[texture(2)]],  // Tangent direction for anisotropy
    constant Uniforms& uniforms [[buffer(0)]],
    constant uint& fabricType [[buffer(1)]],
    sampler textureSampler [[sampler(0)]]
) {
    // Get fabric preset
    FabricProperties props = fabricPresets[fabricType];
    
    // Sample textures
    float4 albedo = albedoMap.sample(textureSampler, in.texCoord);
    float3 normalSample = normalMap.sample(textureSampler, in.texCoord).rgb * 2.0 - 1.0;
    float2 threadDir = threadDirectionMap.sample(textureSampler, in.texCoord).rg * 2.0 - 1.0;
    
    // Construct vectors
    float3 N = normalize(in.normal);
    float3 V = normalize(uniforms.cameraPosition - in.worldPosition);
    float3 L = normalize(-uniforms.lightDirection);
    float3 H = normalize(V + L);
    
    // Tangent and bitangent for anisotropy
    float3 T = normalize(cross(N, float3(0, 1, 0)));
    if (length(cross(N, float3(0, 1, 0))) < 0.001) {
        T = normalize(cross(N, float3(1, 0, 0)));
    }
    float3 B = cross(N, T);
    
    // Rotate tangent by thread direction
    T = T * threadDir.x + B * threadDir.y;
    B = cross(N, T);
    
    // Apply normal map
    float3 finalN = normalize(T * normalSample.x + B * normalSample.y + N * normalSample.z);
    
    // Calculate dot products
    float NdotL = max(dot(finalN, L), 0.0);
    float NdotV = max(dot(finalN, V), 0.0);
    float NdotH = max(dot(finalN, H), 0.0);
    float HdotT = dot(H, T);
    float HdotB = dot(H, B);
    
    // Anisotropic roughness
    float aspect = sqrt(1.0 - props.anisotropy * 0.9);
    float roughnessT = props.roughness / aspect;
    float roughnessB = props.roughness * aspect;
    
    // Diffuse (with subsurface)
    float3 diffuse = subsurfaceScattering(finalN, L, V, albedo.rgb, props.subsurfaceColor, props.subsurface);
    
    // Specular (anisotropic GGX)
    float D = D_GGX_Anisotropic(NdotH, HdotT, HdotB, roughnessT, roughnessB);
    float3 specular = float3(D * 0.25);
    
    // Sheen
    float3 sheen = sheenBRDF(finalN, V, L, props.sheenColor, props.roughness) * props.sheenIntensity;
    
    // Combine
    float3 color = (diffuse + specular + sheen) * uniforms.lightColor * NdotL;
    color += albedo.rgb * uniforms.ambientIntensity;
    
    // Tone mapping
    color = color / (color + float3(1.0));
    color = pow(color, float3(1.0/2.2));
    
    return float4(color, albedo.a);
}
