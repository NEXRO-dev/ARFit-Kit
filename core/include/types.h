/**
 * @file types.h
 * @brief Common types and structures for ARfit-kit
 */

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace arfit {

// Forward declarations
class Mesh;
class Texture;
class Garment;

/**
 * @brief 2D Point
 */
struct Point2D {
    float x = 0.0f;
    float y = 0.0f;
};

/**
 * @brief 3D Point/Vector
 */
struct Point3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    
    Point3D operator+(const Point3D& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }
    
    Point3D operator-(const Point3D& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }
    
    Point3D operator*(float scalar) const {
        return {x * scalar, y * scalar, z * scalar};
    }
};

/**
 * @brief Quaternion for rotation
 */
struct Quaternion {
    float w = 1.0f;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

/**
 * @brief 4x4 Transformation matrix
 */
struct Transform {
    std::array<float, 16> matrix = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    static Transform identity() {
        return Transform{};
    }
};

/**
 * @brief Body landmark indices (MediaPipe Pose compatible)
 */
enum class BodyLandmark : uint8_t {
    NOSE = 0,
    LEFT_EYE_INNER = 1,
    LEFT_EYE = 2,
    LEFT_EYE_OUTER = 3,
    RIGHT_EYE_INNER = 4,
    RIGHT_EYE = 5,
    RIGHT_EYE_OUTER = 6,
    LEFT_EAR = 7,
    RIGHT_EAR = 8,
    MOUTH_LEFT = 9,
    MOUTH_RIGHT = 10,
    LEFT_SHOULDER = 11,
    RIGHT_SHOULDER = 12,
    LEFT_ELBOW = 13,
    RIGHT_ELBOW = 14,
    LEFT_WRIST = 15,
    RIGHT_WRIST = 16,
    LEFT_PINKY = 17,
    RIGHT_PINKY = 18,
    LEFT_INDEX = 19,
    RIGHT_INDEX = 20,
    LEFT_THUMB = 21,
    RIGHT_THUMB = 22,
    LEFT_HIP = 23,
    RIGHT_HIP = 24,
    LEFT_KNEE = 25,
    RIGHT_KNEE = 26,
    LEFT_ANKLE = 27,
    RIGHT_ANKLE = 28,
    LEFT_HEEL = 29,
    RIGHT_HEEL = 30,
    LEFT_FOOT_INDEX = 31,
    RIGHT_FOOT_INDEX = 32,
    NUM_LANDMARKS = 33
};

/**
 * @brief Body pose with 3D landmarks
 */
struct BodyPose {
    std::array<Point3D, 33> landmarks;
    std::array<float, 33> visibility;  // 0.0 - 1.0
    float confidence = 0.0f;
    
    const Point3D& getLandmark(BodyLandmark landmark) const {
        return landmarks[static_cast<size_t>(landmark)];
    }
};

/**
 * @brief Garment type enumeration
 */
enum class GarmentType {
    UNKNOWN = 0,
    TSHIRT,
    SHIRT,
    JACKET,
    COAT,
    DRESS,
    PANTS,
    SHORTS,
    SKIRT
};

/**
 * @brief Image data container
 */
struct ImageData {
    std::vector<uint8_t> pixels;
    int width = 0;
    int height = 0;
    int channels = 4;  // RGBA
};

/**
 * @brief Camera frame data
 */
struct CameraFrame {
    ImageData image;
    Transform cameraTransform;
    float timestamp = 0.0f;
};

/**
 * @brief Session configuration
 */
struct SessionConfig {
    int targetFPS = 60;
    bool enableClothSimulation = true;
    bool enableShadows = true;
    int maxGarments = 3;
    
    // Server-side processing configuration
    std::string serverEndpoint = "";
    bool useHybridProcessing = true;
};

/**
 * @brief Error codes
 */
enum class ErrorCode {
    SUCCESS = 0,
    INITIALIZATION_FAILED,
    CAMERA_ACCESS_DENIED,
    GPU_NOT_AVAILABLE,
    MODEL_LOAD_FAILED,
    GARMENT_CONVERSION_FAILED,
    INVALID_IMAGE,
    SESSION_NOT_STARTED,
    NETWORK_ERROR
};

/**
 * @brief Result wrapper
 */
template<typename T>
struct Result {
    T value;
    ErrorCode error = ErrorCode::SUCCESS;
    std::string message;
    
    bool isSuccess() const { return error == ErrorCode::SUCCESS; }
    operator bool() const { return isSuccess(); }
};

}  // namespace arfit
