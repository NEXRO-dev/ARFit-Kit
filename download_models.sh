#!/bin/bash

# ARfit-kit ML Model Downloader
# This script downloads the necessary ML models for Body Tracking and Garment Segmentation

MODEL_DIR="ml-models"
mkdir -p $MODEL_DIR

echo "Downloading MediaPipe Pose Landmarker (Lite)..."
curl -L -o $MODEL_DIR/pose_landmarker_lite.task https://storage.googleapis.com/mediapipe-models/pose_landmarker/pose_landmarker_lite/float16/1/pose_landmarker_lite.task

echo "Downloading U-2-Net (ONNX)..."
# Using a placeholder URL or a known source for U-2-Net ONNX
# curl -L -o $MODEL_DIR/u2net.onnx https://github.com/danielgatis/rembg/releases/download/v0.0.0/u2net.onnx
echo "Skipping U-2-Net download for now (large file). Please download manually if needed."

echo "Models downloaded to $MODEL_DIR"
