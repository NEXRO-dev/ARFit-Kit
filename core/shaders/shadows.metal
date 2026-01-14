/**
 * ARFitKit Shadow Rendering Shader (Metal)
 * Soft shadows between garment and body for realistic grounding
 */

#include <metal_stdlib>
using namespace metal;

struct ShadowUniforms {
    float4x4 lightSpaceMatrix;
    float4x4 modelMatrix;
    float shadowBias;
    float shadowIntensity;
    float softness;
    float2 shadowMapSize;
};

// ========== Shadow Map Generation Pass ==========
struct ShadowVertexOut {
    float4 position [[position]];
    float depth;
};

vertex ShadowVertexOut shadowMapVertex(
    uint vertexId [[vertex_id]],
    constant float3* positions [[buffer(0)]],
    constant ShadowUniforms& uniforms [[buffer(1)]]
) {
    ShadowVertexOut out;
    
    float4 worldPos = uniforms.modelMatrix * float4(positions[vertexId], 1.0);
    out.position = uniforms.lightSpaceMatrix * worldPos;
    out.depth = out.position.z / out.position.w;
    
    return out;
}

fragment float shadowMapFragment(ShadowVertexOut in [[stage_in]]) {
    return in.depth;
}

// ========== Contact Shadow Computation ==========
// Screen-space contact shadows for garment-body interaction
kernel void computeContactShadows(
    texture2d<float, access::read> depthBuffer [[texture(0)]],
    texture2d<float, access::read> normalBuffer [[texture(1)]],
    texture2d<float, access::write> shadowBuffer [[texture(2)]],
    constant ShadowUniforms& uniforms [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    float depth = depthBuffer.read(gid).r;
    float3 normal = normalBuffer.read(gid).rgb * 2.0 - 1.0;
    
    // Ray march in screen space toward light
    float2 lightDir2D = float2(0.3, -0.7); // Simplified light direction
    float shadow = 0.0;
    
    int steps = 16;
    float stepSize = 2.0;
    
    for (int i = 1; i <= steps; i++) {
        float2 samplePos = float2(gid) + lightDir2D * float(i) * stepSize;
        
        if (samplePos.x < 0 || samplePos.y < 0 ||
            samplePos.x >= uniforms.shadowMapSize.x ||
            samplePos.y >= uniforms.shadowMapSize.y) {
            break;
        }
        
        float sampleDepth = depthBuffer.read(uint2(samplePos)).r;
        
        // If sample is closer to camera, we're in shadow
        if (sampleDepth < depth - uniforms.shadowBias) {
            float falloff = 1.0 - float(i) / float(steps);
            shadow += falloff * uniforms.shadowIntensity / float(steps);
        }
    }
    
    shadow = min(shadow, 1.0);
    shadowBuffer.write(float4(1.0 - shadow), gid);
}

// ========== Soft Shadow Sampling (PCF) ==========
float sampleShadowPCF(
    texture2d<float> shadowMap,
    sampler shadowSampler,
    float2 uv,
    float compareDepth,
    float2 texelSize,
    float softness
) {
    float shadow = 0.0;
    int samples = 9;
    
    // 3x3 PCF kernel
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float2 offset = float2(x, y) * texelSize * softness;
            float shadowDepth = shadowMap.sample(shadowSampler, uv + offset).r;
            shadow += (shadowDepth < compareDepth) ? 1.0 : 0.0;
        }
    }
    
    return shadow / float(samples);
}

// ========== Ambient Occlusion (SSAO) ==========
kernel void computeSSAO(
    texture2d<float, access::read> depthBuffer [[texture(0)]],
    texture2d<float, access::read> normalBuffer [[texture(1)]],
    texture2d<float, access::write> aoBuffer [[texture(2)]],
    constant float3* sampleKernel [[buffer(0)]],
    constant float2* noiseTexture [[buffer(1)]],
    constant float4x4& projection [[buffer(2)]],
    uint2 gid [[thread_position_in_grid]]
) {
    float depth = depthBuffer.read(gid).r;
    float3 normal = normalBuffer.read(gid).rgb * 2.0 - 1.0;
    
    // Sample random vector from noise
    uint2 noiseCoord = gid % 4;
    float2 noise = noiseTexture[noiseCoord.x + noiseCoord.y * 4];
    
    float ao = 0.0;
    int kernelSize = 16;
    float radius = 0.5;
    
    for (int i = 0; i < kernelSize; i++) {
        // Create sample position
        float3 samplePos = sampleKernel[i];
        
        // Reflect around random tangent
        float3 tangent = normalize(cross(normal, float3(noise, 0.0)));
        float3 bitangent = cross(normal, tangent);
        float3x3 TBN = float3x3(tangent, bitangent, normal);
        
        samplePos = TBN * samplePos;
        
        // Compare depths
        // ... (simplified for demo)
        
        ao += 0.0625; // Placeholder
    }
    
    ao = 1.0 - ao;
    aoBuffer.write(float4(ao), gid);
}

// ========== Ground Shadow (for floor contact) ==========
fragment float4 groundShadowFragment(
    float4 position [[position]],
    float2 uv,
    texture2d<float> shadowMap [[texture(0)]],
    sampler shadowSampler [[sampler(0)]],
    constant ShadowUniforms& uniforms [[buffer(0)]]
) {
    // Sample shadow with soft edges
    float shadow = sampleShadowPCF(
        shadowMap, shadowSampler,
        uv, position.z,
        1.0 / uniforms.shadowMapSize,
        uniforms.softness
    );
    
    // Soft falloff at edges
    float2 center = abs(uv - 0.5) * 2.0;
    float edgeFade = 1.0 - max(center.x, center.y);
    edgeFade = smoothstep(0.0, 0.3, edgeFade);
    
    shadow *= edgeFade * uniforms.shadowIntensity;
    
    return float4(0, 0, 0, shadow * 0.5);
}
