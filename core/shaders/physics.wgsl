/**
 * ARFitKit Cloth Physics Compute Shader (WebGPU/WGSL)
 * Position Based Dynamics (PBD) solver for cloth simulation
 */

// Particle structure
struct Particle {
    position: vec3<f32>,
    invMass: f32,
    velocity: vec3<f32>,
    _pad1: f32,
    prevPosition: vec3<f32>,
    _pad2: f32,
};

struct DistanceConstraint {
    indexA: u32,
    indexB: u32,
    restLength: f32,
    stiffness: f32,
};

struct CollisionSphere {
    center: vec3<f32>,
    radius: f32,
};

struct SimParams {
    dt: f32,
    gravity: f32,
    damping: f32,
    numParticles: u32,
    numConstraints: u32,
    numSpheres: u32,
    substeps: u32,
};

@group(0) @binding(0) var<uniform> params: SimParams;
@group(0) @binding(1) var<storage, read_write> particles: array<Particle>;
@group(0) @binding(2) var<storage, read> constraints: array<DistanceConstraint>;
@group(0) @binding(3) var<storage, read> spheres: array<CollisionSphere>;

// ========== Apply Forces ==========
@compute @workgroup_size(256)
fn applyForces(@builtin(global_invocation_id) id: vec3<u32>) {
    let idx = id.x;
    if (idx >= params.numParticles) { return; }
    
    var p = particles[idx];
    
    if (p.invMass == 0.0) { return; }
    
    p.prevPosition = p.position;
    p.velocity.y -= params.gravity * params.dt;
    p.velocity *= (1.0 - params.damping);
    p.position += p.velocity * params.dt;
    
    particles[idx] = p;
}

// ========== Solve Constraints ==========
@compute @workgroup_size(256)
fn solveConstraints(@builtin(global_invocation_id) id: vec3<u32>) {
    let idx = id.x;
    if (idx >= params.numConstraints) { return; }
    
    let c = constraints[idx];
    let pA = particles[c.indexA];
    let pB = particles[c.indexB];
    
    let delta = pB.position - pA.position;
    let currentLength = length(delta);
    
    if (currentLength < 0.0001) { return; }
    
    let error = (currentLength - c.restLength) / currentLength;
    let correction = delta * error * 0.5 * c.stiffness;
    
    let totalMass = pA.invMass + pB.invMass;
    if (totalMass == 0.0) { return; }
    
    let wA = pA.invMass / totalMass;
    let wB = pB.invMass / totalMass;
    
    if (pA.invMass > 0.0) {
        particles[c.indexA].position += correction * wA;
    }
    if (pB.invMass > 0.0) {
        particles[c.indexB].position -= correction * wB;
    }
}

// ========== Collision Detection ==========
@compute @workgroup_size(256)
fn solveCollisions(@builtin(global_invocation_id) id: vec3<u32>) {
    let idx = id.x;
    if (idx >= params.numParticles) { return; }
    
    var p = particles[idx];
    if (p.invMass == 0.0) { return; }
    
    let clothThickness = 0.01;
    
    for (var i: u32 = 0u; i < params.numSpheres; i++) {
        let sphere = spheres[i];
        let diff = p.position - sphere.center;
        let dist = length(diff);
        
        let penetration = (sphere.radius + clothThickness) - dist;
        
        if (penetration > 0.0 && dist > 0.0001) {
            let normal = diff / dist;
            p.position += normal * penetration;
        }
    }
    
    particles[idx] = p;
}

// ========== Update Velocities ==========
@compute @workgroup_size(256)
fn updateVelocities(@builtin(global_invocation_id) id: vec3<u32>) {
    let idx = id.x;
    if (idx >= params.numParticles) { return; }
    
    var p = particles[idx];
    
    if (p.invMass > 0.0) {
        p.velocity = (p.position - p.prevPosition) / params.dt;
    }
    
    particles[idx] = p;
}
