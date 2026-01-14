/**
 * @file body_tracker.h
 * @brief Body tracking module using MediaPipe
 */

#pragma once

#include "types.h"
#include <memory>
#include <vector>

namespace arfit {

/**
 * @brief Configuration for body tracking
 */
struct BodyTrackerConfig {
  float minDetectionConfidence = 0.5f;
  float minTrackingConfidence = 0.5f;
  bool enableSegmentation = false;
  bool smoothLandmarks = true;
  int numPoses = 1; // Number of people to track
};

/**
 * @brief SMPL body model parameters
 */
struct SMPLParams {
  std::array<float, 72> pose;  // 24 joints x 3 axis-angle
  std::array<float, 10> shape; // Beta parameters
  std::array<float, 3> translation;
  float scale = 1.0f;
};

/**
 * @brief Body tracking result
 */
struct BodyTrackingResult {
  BodyPose pose;
  SMPLParams smplParams;
  std::vector<Point3D> bodyMesh; // 3D mesh vertices if available
  ImageData segmentationMask;    // Body segmentation if enabled
  float processingTimeMs = 0.0f;
};

/**
 * @brief Body tracker using MediaPipe Pose
 */
class BodyTracker {
public:
  BodyTracker();
  ~BodyTracker();

  // Prevent copying
  BodyTracker(const BodyTracker &) = delete;
  BodyTracker &operator=(const BodyTracker &) = delete;

  /**
   * @brief Initialize the body tracker
   * @param config Tracker configuration
   * @return Result indicating success or failure
   */
  Result<void>
  initialize(const BodyTrackerConfig &config = BodyTrackerConfig{});

  /**
   * @brief Process a frame and detect body pose
   * @param frame Input camera frame
   * @return Body tracking result
   */
  Result<BodyTrackingResult> processFrame(const CameraFrame &frame);

  /**
   * @brief Convert 2D landmarks to 3D pose using depth estimation
   * @param landmarks2D 2D landmark positions
   * @param frameSize Frame dimensions
   * @return 3D body pose
   */
  BodyPose estimate3DPose(const std::array<Point2D, 33> &landmarks2D,
                          const Point2D &frameSize);

  /**
   * @brief Fit SMPL model to detected pose
   * @param pose Detected body pose
   * @return SMPL parameters
   */
  SMPLParams fitSMPL(const BodyPose &pose);

  /**
   * @brief Get body mesh from SMPL parameters
   * @param params SMPL parameters
   * @return 3D mesh vertices
   */
  std::vector<Point3D> getSMPLMesh(const SMPLParams &params);

  /**
   * @brief Check if tracker is initialized
   */
  bool isInitialized() const;

  /**
   * @brief Reset tracking state
   */
  void reset();

private:
  class Impl;
  std::unique_ptr<Impl> pImpl;
};

} // namespace arfit
