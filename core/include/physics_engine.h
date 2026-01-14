/**
 * @file physics_engine.h
 * @brief Cloth physics simulation using Position Based Dynamics (PBD)
 */

#pragma once

#include "garment_converter.h"
#include "types.h"
#include <memory>
#include <vector>

namespace arfit {

/**
 * @brief Physics simulation configuration
 */
struct PhysicsConfig {
  float gravity = -9.81f;
  float timeStep = 1.0f / 60.0f;
  int solverIterations = 10;
  float damping = 0.99f;
  float friction = 0.5f;

  // Cloth properties
  float stretchStiffness = 0.9f;
  float bendStiffness = 0.5f;
  float shearStiffness = 0.7f;

  // Collision
  float collisionMargin = 0.01f;
  bool enableSelfCollision = true;
};

/**
 * @brief Collision body for body-cloth interaction
 */
struct CollisionBody {
  std::vector<Point3D> vertices;
  std::vector<std::array<int, 3>> triangles;
  Transform transform;
};

/**
 * @brief Physics simulation result
 */
struct PhysicsResult {
  std::vector<Point3D> particlePositions;
  std::vector<Point3D> particleNormals;
  float simulationTimeMs;
};

/**
 * @brief Position Based Dynamics cloth physics engine
 */
class PhysicsEngine {
public:
  PhysicsEngine();
  ~PhysicsEngine();

  // Prevent copying
  PhysicsEngine(const PhysicsEngine &) = delete;
  PhysicsEngine &operator=(const PhysicsEngine &) = delete;

  /**
   * @brief Initialize the physics engine
   * @param config Physics configuration
   * @return Result indicating success or failure
   */
  Result<void> initialize(const PhysicsConfig &config = PhysicsConfig{});

  /**
   * @brief Add a garment to the simulation
   * @param garment Garment to simulate
   */
  Result<void> addGarment(std::shared_ptr<Garment> garment);

  /**
   * @brief Remove a garment from simulation
   * @param garment Garment to remove
   */
  void removeGarment(std::shared_ptr<Garment> garment);

  /**
   * @brief Update collision body (from body tracking)
   * @param body Updated collision body mesh
   */
  void updateCollisionBody(const CollisionBody &body);

  /**
   * @brief Step the simulation forward
   * @param deltaTime Time step in seconds
   * @return Simulation result
   */
  Result<PhysicsResult> step(float deltaTime);

  /**
   * @brief Get current particle positions for a garment
   * @param garment Garment to query
   * @return Current particle positions
   */
  std::vector<Point3D> getParticlePositions(std::shared_ptr<Garment> garment);

  /**
   * @brief Apply external force to simulation
   * @param force Force vector to apply
   */
  void applyExternalForce(const Point3D &force);

  /**
   * @brief Reset simulation state
   */
  void reset();

  /**
   * @brief Check if engine is initialized
   */
  bool isInitialized() const;

  // GPU acceleration
  bool isGPUAccelerationEnabled() const;
  void setGPUAccelerationEnabled(bool enabled);

private:
  class Impl;
  std::unique_ptr<Impl> pImpl;
};

} // namespace arfit
