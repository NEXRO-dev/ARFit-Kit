/**
 * @file physics_engine.cpp
 * @brief 位置ベース動力学 (PBD) による布シミュレーションの実装
 */

#include "physics_engine.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>

namespace arfit {

/**
 * @brief シミュレーション用の粒子点
 */
struct Particle {
  Point3D position;
  Point3D prevPosition;
  Point3D velocity;
  float invMass; // 0なら固定点（pinned）
  int anchorBoneId = -1; // 追従するボーンID (-1はなし)
};

/**
 * @brief 距離制約（バネ）
 */
struct Constraint {
  int p1;
  int p2;
  float restLength;
  float stiffness;
};

class PhysicsEngine::Impl {
public:
  PhysicsConfig config;
  bool initialized = false;

  std::vector<Particle> particles;
  std::vector<Constraint> constraints;
  
  // 衣服ごとの粒子範囲
  struct Range { size_t start; size_t count; };
  std::map<std::shared_ptr<Garment>, Range> garmentMap;

  // ボディトラッキングから得られた衝突判定用データ
  CollisionBody lastBody;

  Impl() {}

  /**
   * 物理状態の更新（メインループ）
   */
  void update(float dt) {
    if (particles.empty()) return;

    // 1. 外部力（重力）の適用と予測位置の計算
    Point3D gravity = {0, -9.81f, 0};
    for (auto &p : particles) {
      if (p.invMass > 0) {
        p.velocity = p.velocity + gravity * dt;
        p.prevPosition = p.position;
        p.position = p.position + p.velocity * dt;
      } else if (p.anchorBoneId != -1 && p.anchorBoneId < lastBody.vertices.size()) {
          // 固定点（肩など）はボディの関節座標に直接追随
          p.prevPosition = p.position;
          p.position = lastBody.vertices[p.anchorBoneId];
      }
    }

    // 2. 制約解消（反復計算）
    for (int i = 0; i < config.solverIterations; ++i) {
      // 距離制約（バネの伸縮を解決）
      for (const auto &c : constraints) {
        Particle &p1 = particles[c.p1];
        Particle &p2 = particles[c.p2];
        
        Point3D delta = p1.position - p2.position;
        float dist = std::sqrt(delta.x*delta.x + delta.y*delta.y + delta.z*delta.z);
        if (dist < 0.0001f) continue;
        
        float diff = (dist - c.restLength) / (p1.invMass + p2.invMass + 0.0001f) * c.stiffness;
        Point3D correction = delta * (diff / dist);
        
        if (p1.invMass > 0) p1.position = p1.position - correction * p1.invMass;
        if (p2.invMass > 0) p2.position = p2.position + correction * p2.invMass;
      }
      
      // 3. 衝突判定と解消
      solveCollisions();
    }

    // 4. 速度の更新（PBDにおける速度計算）
    for (auto &p : particles) {
      if (p.invMass > 0) {
        p.velocity = (p.position - p.prevPosition) * (1.0f / dt) * config.damping;
      }
    }
  }

  /**
   * 人体とのリアルな衝突判定（球体モデル）
   */
  void solveCollisions() {
      // ボディの主要な関節を球体として近似
      float headRadius = 0.15f;
      float armRadius = 0.08f;
      float torsoRadius = 0.22f;

      for (auto &p : particles) {
          if (p.invMass <= 0) continue;
          
          for (size_t i = 0; i < lastBody.vertices.size(); ++i) {
              const auto& bv = lastBody.vertices[i];
              float radius = armRadius;
              
              // ランドマークIDに基づいて半径を調整
              if (i == (int)BodyLandmark::NOSE) radius = headRadius;
              if (i == (int)BodyLandmark::LEFT_HIP || i == (int)BodyLandmark::RIGHT_HIP) radius = torsoRadius;

              Point3D diff = p.position - bv;
              float distSq = diff.x*diff.x + diff.y*diff.y + diff.z*diff.z;
              float limit = radius + config.collisionMargin;
              
              if (distSq < limit * limit) {
                  float dist = std::sqrt(distSq);
                  Point3D normal = diff * (1.0f / (dist + 1e-6f));
                  // 衝突面まで押し戻す
                  p.position = bv + normal * limit;
                  // 垂直成分の速度を減衰（摩擦）
                  p.velocity = p.velocity * 0.7f;
              }
          }
      }
  }

  /**
   * メッシュのエッジ情報からストレッチ・ベンディング制約を生成
   */
  void createConstraintsFromMesh(std::shared_ptr<Garment> garment, size_t startOffset) {
    const auto& faces = garment->getMesh()->getFaces();
    std::set<std::pair<int, int>> edges;

    // ストレッチ制約（面を構成する3辺）
    for (const auto& face : faces) {
      for (int i = 0; i < 3; ++i) {
        int a = (int)(startOffset + face.indices[i]);
        int b = (int)(startOffset + face.indices[(i + 1) % 3]);
        if (a > b) std::swap(a, b);
        
        if (edges.find({a, b}) == edges.end()) {
          edges.insert({a, b});
          Constraint c;
          c.p1 = a; c.p2 = b;
          Point3D d = particles[a].position - particles[b].position;
          c.restLength = std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z);
          c.stiffness = config.stretchStiffness;
          constraints.push_back(c);
        }
      }
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

Result<void> PhysicsEngine::addGarment(std::shared_ptr<Garment> garment) {
  if (!garment || !garment->getMesh()) return {.error = ErrorCode::INVALID_IMAGE};

  size_t start = pImpl->particles.size();
  const auto& vertices = garment->getMesh()->getVertices();
  
  for (size_t i = 0; i < vertices.size(); ++i) {
    Particle p;
    p.position = vertices[i].position;
    p.prevPosition = p.position;
    p.velocity = {0, 0, 0};
    p.invMass = 1.0f;
    
    // Y座標とX座標に基づき、肩をボーンにアンカー
    if (vertices[i].position.y > 0.45f && std::abs(vertices[i].position.x) > 0.15f) {
        p.invMass = 0.0f; // 固定
        p.anchorBoneId = (vertices[i].position.x < 0) ? 
            (int)BodyLandmark::LEFT_SHOULDER : (int)BodyLandmark::RIGHT_SHOULDER;
    }
    
    pImpl->particles.push_back(p);
  }

  pImpl->createConstraintsFromMesh(garment, start);
  pImpl->garmentMap[garment] = {start, vertices.size()};
  
  return {.error = ErrorCode::SUCCESS};
}

void PhysicsEngine::updateCollisionBody(const CollisionBody &body) {
  pImpl->lastBody = body;
}

Result<PhysicsResult> PhysicsEngine::step(float dt) {
  pImpl->update(dt);
  PhysicsResult res;
  res.simulationTimeMs = 0.0f; 
  return {.value = res, .error = ErrorCode::SUCCESS};
}

std::vector<Point3D> PhysicsEngine::getParticlePositions(std::shared_ptr<Garment> garment) {
  std::vector<Point3D> pos;
  auto it = pImpl->garmentMap.find(garment);
  if (it != pImpl->garmentMap.end()) {
    for (size_t i = 0; i < it->second.count; ++i) {
      pos.push_back(pImpl->particles[it->second.start + i].position);
    }
  }
  return pos;
}

void PhysicsEngine::removeGarment(std::shared_ptr<Garment> garment) {
    pImpl->garmentMap.erase(garment);
}

void PhysicsEngine::reset() {
  pImpl->particles.clear();
  pImpl->constraints.clear();
  pImpl->garmentMap.clear();
}

bool PhysicsEngine::isInitialized() const { return pImpl->initialized; }
void PhysicsEngine::applyExternalForce(const Point3D &force) {}
bool PhysicsEngine::isGPUAccelerationEnabled() const { return false; }
void PhysicsEngine::setGPUAccelerationEnabled(bool enabled) {}

} // namespace arfit
