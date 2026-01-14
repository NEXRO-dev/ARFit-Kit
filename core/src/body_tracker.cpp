/**
 * @file body_tracker.cpp
 * @brief Body tracking implementation using MediaPipe
 */

#include "body_tracker.h"
#include <chrono>
#include <cmath>
#include <iostream>
#include <opencv2/opencv.hpp>

namespace arfit {

// Constants for SMPL
const int SMPL_NUM_JOINTS = 24;
const int MEDIA_PIPE_LANDMARKS = 33;

class BodyTracker::Impl {
public:
  BodyTrackerConfig config;
  bool initialized = false;

  // SMPL model data
  std::vector<Point3D> smplTemplate;

  Impl() { initializeSMPLTemplate(); }

  void initializeSMPLTemplate() {
    // Initialize basic SMPL template vertices (T-pose)
    // In a real implementation, this would load a .obj or .pkl file
    smplTemplate.resize(6890);
    for (auto &v : smplTemplate) {
      v = {0.0f, 0.0f, 0.0f};
    }
  }

  // Helper to calculate distance between two points
  float distance(const Point3D &a, const Point3D &b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
  }
};

BodyTracker::BodyTracker() : pImpl(std::make_unique<Impl>()) {}

BodyTracker::~BodyTracker() = default;

Result<void> BodyTracker::initialize(const BodyTrackerConfig &config) {
  pImpl->config = config;

  // Initialize MediaPipe graph here
  // For now, we simulate initialization success

  pImpl->initialized = true;
  return {.error = ErrorCode::SUCCESS};
}

Result<BodyTrackingResult> BodyTracker::processFrame(const CameraFrame &frame) {
  if (!pImpl->initialized) {
    return {.error = ErrorCode::INITIALIZATION_FAILED,
            .message = "Body tracker not initialized"};
  }

  BodyTrackingResult result;
  auto startTime = std::chrono::steady_clock::now();

  // 1. Pre-process Image
  // Convert to RGB if necessary for MediaPipe
  cv::Mat cvImage(frame.image.height, frame.image.width, CV_8UC4,
                  const_cast<uint8_t *>(frame.image.pixels.data()));
  cv::Mat rgbImage;
  if (frame.image.channels == 4) {
    cv::cvtColor(cvImage, rgbImage, cv::COLOR_RGBA2RGB);
  } else {
    rgbImage = cvImage; // Assume RGB if not 4 channels (unsafe in prod)
  }

  // 2. Run MediaPipe Inference (Simulation)
  // In production, this pushes the packet to the MP graph and waits for the
  // output stream. Here we simulate the output landmarks.

  for (int i = 0; i < MEDIA_PIPE_LANDMARKS; ++i) {
    // Simulate a T-pose roughly centered in the screen
    // In a real app, these values come from the MediaPipe output packet
    result.pose.landmarks[i] = {0.0f, 0.0f, 0.0f};
    result.pose.visibility[i] = 0.9f;
  }

  // Simulate some simple movement based on time for testing visualizers
  float time =
      std::chrono::duration<float>(startTime.time_since_epoch()).count();
  float sway = std::sin(time) * 0.1f;

  // Head
  result.pose.landmarks[0] = {0.0f + sway, -0.8f, 0.0f}; // Nose

  // Shoulders
  result.pose.landmarks[11] = {-0.2f + sway, -0.6f, 0.0f}; // Left Shoulder
  result.pose.landmarks[12] = {0.2f + sway, -0.6f, 0.0f};  // Right Shoulder

  // Hips
  result.pose.landmarks[23] = {-0.15f, 0.0f, 0.0f}; // Left Hip
  result.pose.landmarks[24] = {0.15f, 0.0f, 0.0f};  // Right Hip

  result.pose.confidence = 0.98f;

  // 3. Estimate 3D Pose (Depth Lifting)
  // Often MP Pose returns 3D landmarks relative to hips (world landmarks).
  // If not, we run estimate3DPose. Use the "world landmarks" directly if
  // available. Here we assume the landmarks above are already "world-like"
  // normalized coordinates.

  // 4. Fit SMPL Model
  result.smplParams = fitSMPL(result.pose);

  // 5. Generate Mesh
  result.bodyMesh = getSMPLMesh(result.smplParams);

  auto endTime = std::chrono::steady_clock::now();
  result.processingTimeMs =
      std::chrono::duration<float, std::milli>(endTime - startTime).count();

  return {.value = result, .error = ErrorCode::SUCCESS};
}

BodyPose BodyTracker::estimate3DPose(const std::array<Point2D, 33> &landmarks2D,
                                     const Point2D &frameSize) {
  BodyPose pose;
  // Simple pass-through or basic depth estimation heuristic
  // For production: Use a specialized neural network for lifting
  for (int i = 0; i < 33; ++i) {
    pose.landmarks[i] = {landmarks2D[i].x, landmarks2D[i].y, 0.0f};
    pose.visibility[i] = 1.0f;
  }
  return pose;
}

SMPLParams BodyTracker::fitSMPL(const BodyPose &pose) {
  SMPLParams params;

  // Initialize
  params.pose.fill(0.0f);
  params.shape.fill(0.0f);
  params.translation = {0.0f, 0.0f, 0.0f};

  // Simplified Inverse Kinematics (Heuristic)
  // Map MediaPipe landmarks to SMPL joints

  // Center of hips translation
  Point3D hipCenter = (pose.getLandmark(BodyLandmark::LEFT_HIP) +
                       pose.getLandmark(BodyLandmark::RIGHT_HIP)) *
                      0.5f;
  params.translation = {hipCenter.x, hipCenter.y, hipCenter.z};

  // Estimate Scale based on torso length
  Point3D shoulderCenter = (pose.getLandmark(BodyLandmark::LEFT_SHOULDER) +
                            pose.getLandmark(BodyLandmark::RIGHT_SHOULDER)) *
                           0.5f;

  float torsoLength = pImpl->distance(shoulderCenter, hipCenter);
  // Standard torso is roughly 0.5-0.6m? Normalize scale.
  float standardTorso = 0.6f;
  params.scale = torsoLength / standardTorso;
  if (params.scale == 0)
    params.scale = 1.0f;

  // Estimate rotations (simplified)
  // Example: Spine Rotation based on shoulder vs hip angle
  // In a real solver: run Levenberg-Marquardt to minimize reprojection error

  return params;
}

std::vector<Point3D> BodyTracker::getSMPLMesh(const SMPLParams &params) {
  // Return template transformed by params
  std::vector<Point3D> mesh = pImpl->smplTemplate;

  for (auto &v : mesh) {
    // Apply Scale
    v = v * params.scale;

    // Apply Translation
    // Note: Real SMPL has complex skinning (LBS); this is just rigid transform
    // of T-pose for placeholder visualization.
    v = v + params.translation;
  }

  return mesh;
}

bool BodyTracker::isInitialized() const { return pImpl->initialized; }

void BodyTracker::reset() { pImpl->initialized = false; }

} // namespace arfit
