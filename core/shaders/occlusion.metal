/**
 * ARFitKit Occlusion Shader (Metal)
 * Handles body-garment depth occlusion for realistic rendering
 */

#include <metal_stdlib>
using namespace metal;

// Vertex data
struct VertexIn {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float2 texCoord [[attribute(2)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 worldPosition;
    float3 normal;
    float2 texCoord;
    float depth;
};

// Uniforms
struct Uniforms {
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float3 cameraPosition;
    float3 lightDirection;
    float3 lightColor;
    float ambientIntensity;
};

// ========== Body Depth Pass ==========
// First pass: Render body mesh to depth buffer only
vertex VertexOut bodyDepthVertex(
    VertexIn in [[stage_in]],
    constant Uniforms& uniforms [[buffer(1)]]
) {
    VertexOut out;
    
    float4 worldPos = uniforms.modelMatrix * float4(in.position, 1.0);
    out.worldPosition = worldPos.xyz;
    out.position = uniforms.projectionMatrix * uniforms.viewMatrix * worldPos;
    out.depth = out.position.z / out.position.w;
    out.normal = (uniforms.modelMatrix * float4(in.normal, 0.0)).xyz;
    out.texCoord = in.texCoord;
    
    return out;
}

fragment float4 bodyDepthFragment(VertexOut in [[stage_in]]) {
    // Discard fragments - we only want depth written
    discard_fragment();
    return float4(0);
}

// ========== Garment Render Pass ==========
// Second pass: Render garment with depth test against body
vertex VertexOut garmentVertex(
    VertexIn in [[stage_in]],
    constant Uniforms& uniforms [[buffer(1)]]
) {
    VertexOut out;
    
    float4 worldPos = uniforms.modelMatrix * float4(in.position, 1.0);
    out.worldPosition = worldPos.xyz;
    out.position = uniforms.projectionMatrix * uniforms.viewMatrix * worldPos;
    out.depth = out.position.z / out.position.w;
    out.normal = normalize((uniforms.modelMatrix * float4(in.normal, 0.0)).xyz);
    out.texCoord = in.texCoord;
    
    return out;
}

fragment float4 garmentFragment(
    VertexOut in [[stage_in]],
    texture2d<float> garmentTexture [[texture(0)]],
    texture2d<float> bodyDepthTexture [[texture(1)]],
    sampler textureSampler [[sampler(0)]],
    constant Uniforms& uniforms [[buffer(1)]]
) {
    // Sample garment texture
    float4 garmentColor = garmentTexture.sample(textureSampler, in.texCoord);
    
    // Get screen-space coordinates for depth lookup
    float2 screenUV = in.position.xy / float2(1920, 1080); // TODO: Pass resolution
    
    // Sample body depth
    float bodyDepth = bodyDepthTexture.sample(textureSampler, screenUV).r;
    
    // Occlusion test: if body is in front of garment, discard
    float depthBias = 0.001; // Prevent z-fighting
    if (bodyDepth < in.depth - depthBias && bodyDepth > 0.0) {
        // Body part is in front - make garment transparent here
        garmentColor.a *= 0.0;
    }
    
    // Basic lighting
    float3 N = normalize(in.normal);
    float3 L = normalize(-uniforms.lightDirection);
    
    float NdotL = max(dot(N, L), 0.0);
    float3 diffuse = garmentColor.rgb * NdotL * uniforms.lightColor;
    float3 ambient = garmentColor.rgb * uniforms.ambientIntensity;
    
    float3 finalColor = ambient + diffuse;
    
    return float4(finalColor, garmentColor.a);
}

// ========== Arm Occlusion Mask ==========
// Special handling for arms crossing in front of torso
kernel void generateOcclusionMask(
    texture2d<float, access::read> bodySegmentation [[texture(0)]],
    texture2d<float, access::read> depthMap [[texture(1)]],
    texture2d<float, access::write> occlusionMask [[texture(2)]],
    uint2 gid [[thread_position_in_grid]]
) {
    float segmentation = bodySegmentation.read(gid).r;
    float depth = depthMap.read(gid).r;
    
    // Arm region detection (simplified - in reality use pose landmarks)
    // If it's a body part and close to camera, it occludes garment
    float occlusionValue = 0.0;
    
    if (segmentation > 0.5) {
        // Body detected
        if (depth < 0.3) {
            // Close to camera - likely arm in front
            occlusionValue = 1.0;
        }
    }
    
    occlusionMask.write(float4(occlusionValue), gid);
}
