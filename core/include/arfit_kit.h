/**
 * @file arfit_kit.h
 * @brief Main ARfit-kit SDK header
 */

#pragma once

#include "ar_renderer.h"
#include "body_tracker.h"
#include "garment_converter.h"
#include "physics_engine.h"
#include "types.h"

#include <functional>
#include <memory>

namespace arfit {

/**
 * @brief Callback for when a frame is processed
 */
using FrameCallback = std::function<void(const ImageData &frame)>;

/**
 * @brief Callback for body tracking updates
 */
using PoseCallback = std::function<void(const BodyPose &pose)>;

/**
 * @brief Callback for errors
 */
using ErrorCallback =
    std::function<void(ErrorCode code, const std::string &message)>;

/**
 * @brief Main ARfit-kit SDK class
 *
 * This is the primary interface for using the AR try-on functionality.
 */
class ARFitKit {
public:
  ARFitKit();
  ~ARFitKit();

  // Prevent copying
  ARFitKit(const ARFitKit &) = delete;
  ARFitKit &operator=(const ARFitKit &) = delete;

  // Allow moving
  ARFitKit(ARFitKit &&) noexcept;
  ARFitKit &operator=(ARFitKit &&) noexcept;

  /**
   * @brief Initialize the SDK with configuration
   * @param config Session configuration
   * @return Result indicating success or failure
   */
  Result<void> initialize(const SessionConfig &config = SessionConfig{});

  /**
   * @brief Start the AR session
   * @return Result indicating success or failure
   */
  Result<void> startSession();

  /**
   * @brief Stop the AR session
   */
  void stopSession();

  /**
   * @brief Check if session is currently active
   */
  bool isSessionActive() const;

  /**
   * @brief Process a camera frame
   * @param frame Camera frame to process
   * @return Processed frame with AR overlay
   */
  Result<ImageData> processFrame(const CameraFrame &frame);

  /**
   * @brief Load a garment from image data
   * @param image Image data of the garment
   * @param type Type of garment (optional, will be auto-detected if not
   * specified)
   * @return Loaded garment or error
   */
  Result<std::shared_ptr<Garment>>
  loadGarment(const ImageData &image, GarmentType type = GarmentType::UNKNOWN);

  /**
   * @brief Load a garment from URL (using hybrid server processing)
   * @param url URL of the garment image
   * @return Loaded garment or error
   */
  Result<std::shared_ptr<Garment>> loadGarmentFromUrl(const std::string &url);

  /**
   * @brief Try on a garment
   * @param garment Garment to try on
   * @return Result indicating success or failure
   */
  Result<void> tryOn(std::shared_ptr<Garment> garment);

  /**
   * @brief Remove a garment
   * @param garment Garment to remove
   */
  void removeGarment(std::shared_ptr<Garment> garment);

  /**
   * @brief Remove all garments
   */
  void removeAllGarments();

  /**
   * @brief Capture a snapshot of current view
   * @return Captured image
   */
  Result<ImageData> captureSnapshot();

  // Callbacks
  void setFrameCallback(FrameCallback callback);
  void setPoseCallback(PoseCallback callback);
  void setErrorCallback(ErrorCallback callback);

  // Components (for advanced usage)
  BodyTracker &getBodyTracker();
  GarmentConverter &getGarmentConverter();
  PhysicsEngine &getPhysicsEngine();
  ARRenderer &getARRenderer();

  // Performance metrics
  float getCurrentFPS() const;
  float getAverageLatency() const;

private:
  class Impl;
  std::unique_ptr<Impl> pImpl;
};

} // namespace arfit
