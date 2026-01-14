/**
 * @file physics_engine.cpp
 * @brief Position Based Dynamics (PBD) physics engine implementation
 */

#include "physics_engine.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace arfit {

struct Particle {
  Point3D position;
  Point3D prevPosition;
  Point3D velocity;
  float inverseMass; // 0 for static/pinned particles
  int id;
};

struct Constraint {
  int p1_index;
  int p2_index;
  float restLength;
  float stiffness;
  enum Type { STRETCH, SHEAR, BEND } type;
};

class PhysicsEngine::Impl {
public:
  PhysicsConfig config;
  bool initialized = false;

  // Simulation state
  std::vector<Particle> particles;
  std::vector<Constraint> constraints;

  // Mapping from Garment ID to particle indices start/count
  struct GarmentRange {
    size_t startIndex;
    size_t count;
  };
  std::map<std::shared_ptr<Garment>, GarmentRange> garmentMap;

  // Collision body
  CollisionBody bodyCollider;

  Impl() {}

  void applyForces(float dt) {
    Point3D gravity = config.gravity;

    for (auto &p : particles) {
      if (p.inverseMass == 0.0f)
        continue;

      // Apply Gravity
      p.velocity = p.velocity + gravity * dt;

      // Apply Drag/Damping
      p.velocity = p.velocity * (1.0f - config.drag);
    }
  }

  void integrate(float dt) {
    for (auto &p : particles) {
      if (p.inverseMass == 0.0f)
        continue;

      p.prevPosition = p.position;
      p.position = p.position + p.velocity * dt;

      // Floor collision (simple ground plane at y = -2.0)
      if (p.position.y < -2.0f) {
        p.position.y = -2.0f;
      }
    }
  }

  void solveConstraints(float dt) {
    // Solve distance constraints
    for (int iter = 0; iter < config.substeps; ++iter) {
      for (const auto &c : constraints) {
        Particle &p1 = particles[c.p1_index];
        Particle &p2 = particles[c.p2_index];

        if (p1.inverseMass == 0.0f && p2.inverseMass == 0.0f)
          continue;

        float dx = p1.position.x - p2.position.x;
        float dy = p1.position.y - p2.position.y;
        float dz = p1.position.z - p2.position.z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (dist < 0.0001f)
          continue; // Avoid division by zero

        float correction = (dist - c.restLength) /
                           (p1.inverseMass + p2.inverseMass) * c.stiffness;

        float px = dx / dist * correction;
        float py = dy / dist * correction;
        float pz = dz / dist * correction;

        if (p1.inverseMass > 0.0f) {
          p1.position.x -= px * p1.inverseMass;
          p1.position.y -= py * p1.inverseMass;
          p1.position.z -= pz * p1.inverseMass;
        }

        if (p2.inverseMass > 0.0f) {
          p2.position.x += px * p2.inverseMass;
          p2.position.y += py * p2.inverseMass;
          p2.position.z += pz * p2.inverseMass;
        }
      }

      // Solve body collisions
      solveBodyCollision();
    }
  }

  void solveBodyCollision() {
    if (bodyCollider.vertices.empty())
      return;

    // Naive O(N*M) collision for demonstration
    // In production: Spatial Hashing or BVH is mandatory

    // Simplified: Collide with "Bone capsules" instead of full mesh
    // This is much faster and stable.

    // For now, simple sphere collision around hip center (approximate)
    if (bodyCollider.vertices.size() > 0) {
      Point3D center = bodyCollider.vertices[0]; // Approximation
      float radius = 0.3f;                       // Approximation

      for (auto &p : particles) {
        if (p.inverseMass == 0.0f)
          continue;

        float dx = p.position.x - center.x;
        float dy = p.position.y - center.y;
        float dz = p.position.z - center.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < radius * radius) {
          float dist = std::sqrt(distSq);
          if (dist < 0.0001f)
            continue;

          // Push out
          float penetrate = radius - dist;
          float nx = dx / dist;
          float ny = dy / dist;
          float nz = dz / dist;

          p.position.x += nx * penetrate;
          p.position.y += ny * penetrate;
          p.position.z += nz * penetrate;

          // Friction
          // p.velocity = p.velocity * 0.9f;
        }
      }
    }
  }

  void updateVelocities(float dt) {
    for (auto &p : particles) {
      if (p.inverseMass == 0.0f)
        continue;

      p.velocity.x = (p.position.x - p.prevPosition.x) / dt;
      p.velocity.y = (p.position.y - p.prevPosition.y) / dt;
      p.velocity.z = (p.position.z - p.prevPosition.z) / dt;
    }
  }
};

PhysicsEngine::PhysicsEngine() : pImpl(std::make_unique<Impl>()) {}

PhysicsEngine::~PhysicsEngine() = default;

Result<void> PhysicsEngine::initialize(const PhysicsConfig &config) {
  pImpl->config = config;
  pImpl->initialized = true;
  return {.error = ErrorCode::SUCCESS};
}

Result<void> PhysicsEngine::step(float dt) {
  if (!pImpl->initialized)
    return {.error = ErrorCode::INITIALIZATION_FAILED};

  // Sub-steps for stability
  float subDt = dt / pImpl->config.substeps;

  // XPBD main loop
  pImpl->applyForces(dt);
  pImpl->integrate(dt);
  pImpl->solveConstraints(dt);
  pImpl->updateVelocities(dt);

  return {.error = ErrorCode::SUCCESS};
}

Result<void> PhysicsEngine::addGarment(std::shared_ptr<Garment> garment) {
  if (!garment || !garment->mesh)
    return {.error = ErrorCode::INVALID_IMAGE};

  size_t startIdx = pImpl->particles.size();
  size_t count = garment->mesh->vertices.size();

  // Create particles from mesh vertices
  for (size_t i = 0; i < count; ++i) {
    Particle p;
    p.position = garment->mesh->vertices[i];
    p.prevPosition = p.position;
    p.velocity = {0, 0, 0};
    p.inverseMass = 1.0f; // Default mass
    p.id = startIdx + i;

    // Pin some vertices (e.g. shoulders for tshirt)
    // Simple heuristic: Pin top vertices
    if (p.position.y > 0.3f) { // Arbitrary threshold for demo
      p.inverseMass = 0.0f;
    }

    pImpl->particles.push_back(p);
  }

  // Create constraints from mesh topology (edges)
  const auto &indices = garment->mesh->indices;
  for (size_t i = 0; i < indices.size(); i += 3) {
    // Triangle edges
    int idx0 = startIdx + indices[i];
    int idx1 = startIdx + indices[i + 1];
    int idx2 = startIdx + indices[i + 2];

    auto addConstraint = [&](int i1, int i2) {
      Constraint c;
      c.p1_index = i1;
      c.p2_index = i2;

      float dx =
          pImpl->particles[i1].position.x - pImpl->particles[i2].position.x;
      float dy =
          pImpl->particles[i1].position.y - pImpl->particles[i2].position.y;
      float dz =
          pImpl->particles[i1].position.z - pImpl->particles[i2].position.z;
      c.restLength = std::sqrt(dx * dx + dy * dy + dz * dz);

      c.stiffness = 1.0f; // High stiffness for structural
      c.type = Constraint::STRETCH;
      pImpl->constraints.push_back(c);
    };

    addConstraint(idx0, idx1);
    addConstraint(idx1, idx2);
    addConstraint(idx2, idx0);
  }

  pImpl->garmentMap[garment] = {startIdx, count};

  return {.error = ErrorCode::SUCCESS};
}

void PhysicsEngine::removeGarment(std::shared_ptr<Garment> garment) {
  // In a real engine, we'd need efficient removal (swap and pop)
  // For now, just clearing everything is easier for demo if multi-garment isn't
  // strictly required
  reset();
}

void PhysicsEngine::updateCollisionBody(const CollisionBody &body) {
  pImpl->bodyCollider = body;
}

std::vector<Point3D>
PhysicsEngine::getParticlePositions(std::shared_ptr<Garment> garment) {
  std::vector<Point3D> positions;

  if (pImpl->garmentMap.find(garment) != pImpl->garmentMap.end()) {
    auto range = pImpl->garmentMap[garment];
    positions.reserve(range.count);
    for (size_t i = 0; i < range.count; ++i) {
      positions.push_back(pImpl->particles[range.startIndex + i].position);
    }
  }

  return positions;
}

void PhysicsEngine::reset() {
  pImpl->particles.clear();
  pImpl->constraints.clear();
  pImpl->garmentMap.clear();
}

} // namespace arfit
