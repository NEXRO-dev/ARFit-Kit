/**
 * ARFitKit Environment Lighting Shader (Metal)
 * Estimates lighting from camera feed and applies to garments
 */

#include <metal_stdlib>
using namespace metal;

// Spherical Harmonics coefficients for environment lighting
struct SHCoefficients {
    float3 L00;   // Ambient
    float3 L1m1;  // Y direction
    float3 L10;   // Z direction
    float3 L11;   // X direction
    float3 L2m2;  // Higher order
    float3 L2m1;
    float3 L20;
    float3 L21;
    float3 L22;
};

// PBR Material properties
struct Material {
    float3 albedo;
    float metallic;
    float roughness;
    float ao;
};

// ========== Compute SH Coefficients from Environment ==========
kernel void computeEnvironmentSH(
    texture2d<float, access::read> cameraFrame [[texture(0)]],
    device SHCoefficients& shCoeffs [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    // Sample multiple points from camera to estimate lighting
    // This is a simplified version - production would use more samples
    
    uint2 dims = uint2(cameraFrame.get_width(), cameraFrame.get_height());
    
    // Only process on first thread
    if (gid.x != 0 || gid.y != 0) return;
    
    float3 totalLight = float3(0);
    int sampleCount = 16;
    
    for (int i = 0; i < sampleCount; i++) {
        for (int j = 0; j < sampleCount; j++) {
            uint2 samplePos = uint2(
                (dims.x / sampleCount) * i + dims.x / (2 * sampleCount),
                (dims.y / sampleCount) * j + dims.y / (2 * sampleCount)
            );
            float4 color = cameraFrame.read(samplePos);
            totalLight += color.rgb;
        }
    }
    
    totalLight /= float(sampleCount * sampleCount);
    
    // Simple SH estimation (L0 ambient only for this demo)
    shCoeffs.L00 = totalLight * 0.886227; // Y_00 coefficient
    shCoeffs.L1m1 = float3(0);
    shCoeffs.L10 = float3(0, 0.1, 0); // Assume slight top-down light
    shCoeffs.L11 = float3(0);
}

// ========== Apply Environment Lighting to Garment ==========
float3 evaluateSH(float3 normal, constant SHCoefficients& sh) {
    // Evaluate spherical harmonics for the given normal
    float3 result = sh.L00;
    
    // First order (directional)
    result += sh.L1m1 * normal.y;
    result += sh.L10 * normal.z;
    result += sh.L11 * normal.x;
    
    // Second order (more detail)
    result += sh.L2m2 * (normal.x * normal.y);
    result += sh.L2m1 * (normal.y * normal.z);
    result += sh.L20 * (3.0 * normal.z * normal.z - 1.0);
    result += sh.L21 * (normal.x * normal.z);
    result += sh.L22 * (normal.x * normal.x - normal.y * normal.y);
    
    return max(result, float3(0));
}

// PBR lighting calculation
float3 calculatePBR(
    float3 N, float3 V, float3 L,
    Material mat,
    float3 lightColor,
    float3 envLight
) {
    float3 H = normalize(V + L);
    
    // Fresnel-Schlick
    float3 F0 = mix(float3(0.04), mat.albedo, mat.metallic);
    float cosTheta = max(dot(H, V), 0.0);
    float3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
    
    // Distribution (GGX)
    float a = mat.roughness * mat.roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    float D = a2 / (3.14159 * denom * denom);
    
    // Geometry (Smith)
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float k = (mat.roughness + 1.0) * (mat.roughness + 1.0) / 8.0;
    float G = (NdotV / (NdotV * (1.0 - k) + k)) * (NdotL / (NdotL * (1.0 - k) + k));
    
    // Specular
    float3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);
    
    // Diffuse
    float3 kD = (1.0 - F) * (1.0 - mat.metallic);
    float3 diffuse = kD * mat.albedo / 3.14159;
    
    // Combine
    float3 directLight = (diffuse + specular) * lightColor * NdotL;
    float3 ambientLight = mat.albedo * envLight * mat.ao;
    
    return directLight + ambientLight;
}

// Fragment shader for PBR garment rendering
fragment float4 pbrGarmentFragment(
    VertexOut in [[stage_in]],
    texture2d<float> albedoMap [[texture(0)]],
    texture2d<float> normalMap [[texture(1)]],
    texture2d<float> roughnessMap [[texture(2)]],
    constant SHCoefficients& shCoeffs [[buffer(0)]],
    constant Uniforms& uniforms [[buffer(1)]],
    sampler textureSampler [[sampler(0)]]
) {
    // Sample textures
    float4 albedo = albedoMap.sample(textureSampler, in.texCoord);
    float3 normalSample = normalMap.sample(textureSampler, in.texCoord).rgb * 2.0 - 1.0;
    float roughness = roughnessMap.sample(textureSampler, in.texCoord).r;
    
    // Construct TBN matrix for normal mapping
    float3 N = normalize(in.normal);
    // Simplified - in production, calculate tangent/bitangent
    float3 finalNormal = normalize(N + normalSample * 0.5);
    
    float3 V = normalize(uniforms.cameraPosition - in.worldPosition);
    float3 L = normalize(-uniforms.lightDirection);
    
    Material mat;
    mat.albedo = albedo.rgb;
    mat.metallic = 0.0; // Cloth is usually non-metallic
    mat.roughness = max(roughness, 0.1);
    mat.ao = 1.0;
    
    // Environment lighting from SH
    float3 envLight = evaluateSH(finalNormal, shCoeffs);
    
    // Calculate PBR lighting
    float3 color = calculatePBR(finalNormal, V, L, mat, uniforms.lightColor, envLight);
    
    // Tone mapping (ACES approximation)
    color = color / (color + float3(1.0));
    color = pow(color, float3(1.0/2.2)); // Gamma correction
    
    return float4(color, albedo.a);
}
