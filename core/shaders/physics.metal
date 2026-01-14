/**
 * ARFitKit Cloth Physics Compute Shader (Metal)
 * Position Based Dynamics (PBD) solver for cloth simulation
 */

#include <metal_stdlib>
using namespace metal;

// Particle data structure
struct Particle {
    float3 position;
    float3 velocity;
    float3 prevPosition;
    float invMass;  // 0 = pinned particle
};

// Constraint data
struct DistanceConstraint {
    uint indexA;
    uint indexB;
    float restLength;
    float stiffness;
};

// Simulation parameters
struct SimParams {
    float dt;
    float gravity;
    float damping;
    uint numParticles;
    uint numConstraints;
    uint substeps;
};

// Collision sphere (body part)
struct CollisionSphere {
    float3 center;
    float radius;
};

// ========== Kernel 1: Apply External Forces ==========
kernel void applyForces(
    device Particle* particles [[buffer(0)]],
    constant SimParams& params [[buffer(1)]],
    uint id [[thread_position_in_grid]]
) {
    if (id >= params.numParticles) return;
    
    Particle p = particles[id];
    
    // Skip pinned particles
    if (p.invMass == 0.0) return;
    
    // Save current position
    p.prevPosition = p.position;
    
    // Apply gravity
    p.velocity.y -= params.gravity * params.dt;
    
    // Apply damping
    p.velocity *= (1.0 - params.damping);
    
    // Predict position
    p.position += p.velocity * params.dt;
    
    particles[id] = p;
}

// ========== Kernel 2: Solve Distance Constraints ==========
kernel void solveConstraints(
    device Particle* particles [[buffer(0)]],
    constant DistanceConstraint* constraints [[buffer(1)]],
    constant SimParams& params [[buffer(2)]],
    uint id [[thread_position_in_grid]]
) {
    if (id >= params.numConstraints) return;
    
    DistanceConstraint c = constraints[id];
    
    Particle pA = particles[c.indexA];
    Particle pB = particles[c.indexB];
    
    float3 delta = pB.position - pA.position;
    float currentLength = length(delta);
    
    if (currentLength < 0.0001) return;
    
    float error = (currentLength - c.restLength) / currentLength;
    float3 correction = delta * error * 0.5 * c.stiffness;
    
    float totalMass = pA.invMass + pB.invMass;
    if (totalMass == 0.0) return;
    
    float wA = pA.invMass / totalMass;
    float wB = pB.invMass / totalMass;
    
    // Apply corrections
    if (pA.invMass > 0.0) {
        particles[c.indexA].position += correction * wA;
    }
    if (pB.invMass > 0.0) {
        particles[c.indexB].position -= correction * wB;
    }
}

// ========== Kernel 3: Collision Detection & Response ==========
kernel void solveCollisions(
    device Particle* particles [[buffer(0)]],
    constant CollisionSphere* spheres [[buffer(1)]],
    constant uint& numSpheres [[buffer(2)]],
    constant SimParams& params [[buffer(3)]],
    uint id [[thread_position_in_grid]]
) {
    if (id >= params.numParticles) return;
    
    Particle p = particles[id];
    if (p.invMass == 0.0) return;
    
    for (uint i = 0; i < numSpheres; i++) {
        CollisionSphere sphere = spheres[i];
        
        float3 diff = p.position - sphere.center;
        float dist = length(diff);
        
        // Sphere collision with offset for cloth thickness
        float clothThickness = 0.01;
        float penetration = (sphere.radius + clothThickness) - dist;
        
        if (penetration > 0.0 && dist > 0.0001) {
            float3 normal = diff / dist;
            p.position += normal * penetration;
        }
    }
    
    particles[id] = p;
}

// ========== Kernel 4: Update Velocities ==========
kernel void updateVelocities(
    device Particle* particles [[buffer(0)]],
    constant SimParams& params [[buffer(1)]],
    uint id [[thread_position_in_grid]]
) {
    if (id >= params.numParticles) return;
    
    Particle p = particles[id];
    
    if (p.invMass > 0.0) {
        p.velocity = (p.position - p.prevPosition) / params.dt;
    }
    
    particles[id] = p;
}
