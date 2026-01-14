/**
 * ARFitKit - Web SDK for AR Virtual Try-On
 *
 * オープンソースのAR試着SDK
 * TypeScriptでWebブラウザ向けAR機能を提供
 */

import { Pose, Results as PoseResults } from '@mediapipe/pose';
import { Camera } from '@mediapipe/camera_utils';
import * as THREE from 'three';

// Types
export enum GarmentType {
  UNKNOWN = 'unknown',
  TSHIRT = 'tshirt',
  SHIRT = 'shirt',
  JACKET = 'jacket',
  COAT = 'coat',
  DRESS = 'dress',
  PANTS = 'pants',
  SHORTS = 'shorts',
  SKIRT = 'skirt',
}

export interface SessionConfig {
  targetFPS?: number;
  enableClothSimulation?: boolean;
  enableShadows?: boolean;
  maxGarments?: number;
  serverEndpoint?: string;
  useHybridProcessing?: boolean;
}

export interface BodyPose {
  landmarks: THREE.Vector3[];
  visibility: number[];
  confidence: number;
  timestamp: number;
}

export interface Garment {
  id: string;
  type: GarmentType;
  imageUrl: string;
  mesh?: THREE.Mesh;
  isLoading: boolean;
  conversionProgress: number;
}

export type FrameCallback = (canvas: HTMLCanvasElement) => void;
export type PoseCallback = (pose: BodyPose) => void;
export type ErrorCallback = (error: Error) => void;

/**
 * ARFitKit メインクラス
 */
export class ARFitKit {
  private config: SessionConfig = {};
  private canvas: HTMLCanvasElement | null = null;
  private video: HTMLVideoElement | null = null;
  
  private pose: Pose | null = null;
  private camera: Camera | null = null;
  
  private scene: THREE.Scene | null = null;
  private threeCamera: THREE.PerspectiveCamera | null = null;
  private renderer: THREE.WebGLRenderer | null = null;
  
  private activeGarments: Garment[] = [];
  private currentPose: BodyPose | null = null;
  
  private isActive = false;
  private currentFPS = 0;
  private lastFrameTime = 0;
  
  // Callbacks
  public onFrameProcessed: FrameCallback | null = null;
  public onPoseUpdated: PoseCallback | null = null;
  public onError: ErrorCallback | null = null;
  
  /**
   * SDKを初期化
   */
  async initialize(config: SessionConfig = {}): Promise<void> {
    this.config = {
      targetFPS: 60,
      enableClothSimulation: true,
      enableShadows: true,
      maxGarments: 3,
      useHybridProcessing: true,
      ...config,
    };
    
    // Initialize MediaPipe Pose
    this.pose = new Pose({
      locateFile: (file) => {
        return `https://cdn.jsdelivr.net/npm/@mediapipe/pose/${file}`;
      },
    });
    
    this.pose.setOptions({
      modelComplexity: 1,
      smoothLandmarks: true,
      enableSegmentation: false,
      minDetectionConfidence: 0.5,
      minTrackingConfidence: 0.5,
    });
    
    this.pose.onResults(this.onPoseResults.bind(this));
    
    console.log('ARFitKit initialized');
  }
  
  /**
   * セッションを開始
   */
  async startSession(canvas: HTMLCanvasElement): Promise<void> {
    if (this.isActive) return;
    
    this.canvas = canvas;
    
    // Create video element for camera
    this.video = document.createElement('video');
    this.video.style.display = 'none';
    document.body.appendChild(this.video);
    
    // Setup Three.js
    this.setupThreeJS(canvas);
    
    // Setup MediaPipe camera
    this.camera = new Camera(this.video, {
      onFrame: async () => {
        if (this.pose && this.video) {
          await this.pose.send({ image: this.video });
        }
      },
      width: canvas.width,
      height: canvas.height,
    });
    
    await this.camera.start();
    this.isActive = true;
    
    // Start render loop
    this.animate();
    
    console.log('ARFitKit session started');
  }
  
  /**
   * セッションを停止
   */
  stopSession(): void {
    if (this.camera) {
      this.camera.stop();
      this.camera = null;
    }
    
    if (this.video) {
      this.video.remove();
      this.video = null;
    }
    
    if (this.renderer) {
      this.renderer.dispose();
      this.renderer = null;
    }
    
    this.isActive = false;
    this.activeGarments = [];
    
    console.log('ARFitKit session stopped');
  }
  
  /**
   * 衣服を読み込む
   */
  async loadGarment(imageUrl: string, type: GarmentType = GarmentType.UNKNOWN): Promise<Garment> {
    const garment: Garment = {
      id: crypto.randomUUID(),
      type,
      imageUrl,
      isLoading: true,
      conversionProgress: 0,
    };
    
    try {
      // Load texture
      const textureLoader = new THREE.TextureLoader();
      const texture = await new Promise<THREE.Texture>((resolve, reject) => {
        textureLoader.load(imageUrl, resolve, undefined, reject);
      });
      
      garment.conversionProgress = 0.5;
      
      // Create mesh from template
      const geometry = this.createGarmentGeometry(type);
      const material = new THREE.MeshStandardMaterial({
        map: texture,
        side: THREE.DoubleSide,
        transparent: true,
      });
      
      garment.mesh = new THREE.Mesh(geometry, material);
      garment.isLoading = false;
      garment.conversionProgress = 1;
      
      return garment;
    } catch (error) {
      garment.isLoading = false;
      throw error;
    }
  }
  
  /**
   * 衣服を試着
   */
  tryOn(garment: Garment): void {
    if (!this.isActive) {
      throw new Error('Session not started');
    }
    
    // Check max garments
    const maxGarments = this.config.maxGarments || 3;
    if (this.activeGarments.length >= maxGarments) {
      const removed = this.activeGarments.shift();
      if (removed?.mesh && this.scene) {
        this.scene.remove(removed.mesh);
      }
    }
    
    if (garment.mesh && this.scene) {
      this.scene.add(garment.mesh);
    }
    
    this.activeGarments.push(garment);
    
    console.log(`Trying on garment: ${garment.type}`);
  }
  
  /**
   * 衣服を削除
   */
  removeGarment(garment: Garment): void {
    const index = this.activeGarments.findIndex(g => g.id === garment.id);
    if (index !== -1) {
      const removed = this.activeGarments.splice(index, 1)[0];
      if (removed.mesh && this.scene) {
        this.scene.remove(removed.mesh);
      }
    }
  }
  
  /**
   * すべての衣服を削除
   */
  removeAllGarments(): void {
    for (const garment of this.activeGarments) {
      if (garment.mesh && this.scene) {
        this.scene.remove(garment.mesh);
      }
    }
    this.activeGarments = [];
  }
  
  /**
   * スナップショットを撮影
   */
  async captureSnapshot(): Promise<Blob> {
    if (!this.canvas) {
      throw new Error('Canvas not available');
    }
    
    return new Promise((resolve, reject) => {
      this.canvas!.toBlob((blob) => {
        if (blob) {
          resolve(blob);
        } else {
          reject(new Error('Failed to capture snapshot'));
        }
      }, 'image/png');
    });
  }
  
  /**
   * 現在のFPSを取得
   */
  getCurrentFPS(): number {
    return this.currentFPS;
  }
  
  /**
   * 現在のポーズを取得
   */
  getCurrentPose(): BodyPose | null {
    return this.currentPose;
  }
  
  /**
   * アクティブな衣服を取得
   */
  getActiveGarments(): Garment[] {
    return [...this.activeGarments];
  }
  
  // Private methods
  
  private setupThreeJS(canvas: HTMLCanvasElement): void {
    this.scene = new THREE.Scene();
    
    this.threeCamera = new THREE.PerspectiveCamera(
      75,
      canvas.width / canvas.height,
      0.1,
      1000
    );
    this.threeCamera.position.z = 2;
    
    this.renderer = new THREE.WebGLRenderer({
      canvas,
      alpha: true,
      antialias: true,
    });
    this.renderer.setSize(canvas.width, canvas.height);
    this.renderer.setPixelRatio(window.devicePixelRatio);
    
    if (this.config.enableShadows) {
      this.renderer.shadowMap.enabled = true;
    }
    
    // Add lights
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.5);
    this.scene.add(ambientLight);
    
    const directionalLight = new THREE.DirectionalLight(0xffffff, 0.8);
    directionalLight.position.set(0, 1, 1);
    if (this.config.enableShadows) {
      directionalLight.castShadow = true;
    }
    this.scene.add(directionalLight);
  }
  
  private createGarmentGeometry(type: GarmentType): THREE.BufferGeometry {
    // Create basic garment geometry based on type
    switch (type) {
      case GarmentType.TSHIRT:
      case GarmentType.SHIRT:
        return this.createTShirtGeometry();
      case GarmentType.PANTS:
      case GarmentType.SHORTS:
        return new THREE.PlaneGeometry(0.8, 1.0, 20, 30);
      default:
        return new THREE.PlaneGeometry(1, 1, 20, 20);
    }
  }
  
  private createTShirtGeometry(): THREE.BufferGeometry {
    const rows = 20;
    const cols = 15;
    const vertices: number[] = [];
    const indices: number[] = [];
    const uvs: number[] = [];
    
    for (let r = 0; r < rows; r++) {
      const y = 1.0 - (r / (rows - 1)) * 1.5;
      
      for (let c = 0; c < cols; c++) {
        const t = c / (cols - 1);
        let x = (t - 0.5) * 0.8;
        
        // Add sleeve width
        if (r >= 2 && r <= 5) {
          const sleeveExtend = 0.3 * (1.0 - Math.abs(r - 3.5) / 2.0);
          if (t < 0.3) x -= sleeveExtend;
          if (t > 0.7) x += sleeveExtend;
        }
        
        vertices.push(x, y, 0);
        uvs.push(t, r / (rows - 1));
      }
    }
    
    for (let r = 0; r < rows - 1; r++) {
      for (let c = 0; c < cols - 1; c++) {
        const i = r * cols + c;
        indices.push(i, i + 1, i + cols + 1);
        indices.push(i, i + cols + 1, i + cols);
      }
    }
    
    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute('position', new THREE.Float32BufferAttribute(vertices, 3));
    geometry.setAttribute('uv', new THREE.Float32BufferAttribute(uvs, 2));
    geometry.setIndex(indices);
    geometry.computeVertexNormals();
    
    return geometry;
  }
  
  private onPoseResults(results: PoseResults): void {
    if (!results.poseLandmarks) return;
    
    const landmarks = results.poseLandmarks.map(lm => 
      new THREE.Vector3(lm.x * 2 - 1, -(lm.y * 2 - 1), -lm.z)
    );
    
    const visibility = results.poseLandmarks.map(lm => lm.visibility || 0);
    
    this.currentPose = {
      landmarks,
      visibility,
      confidence: visibility.reduce((a, b) => a + b, 0) / visibility.length,
      timestamp: Date.now(),
    };
    
    // Update garment positions based on pose
    this.updateGarmentPositions();
    
    if (this.onPoseUpdated) {
      this.onPoseUpdated(this.currentPose);
    }
  }
  
  private updateGarmentPositions(): void {
    if (!this.currentPose) return;
    
    const { landmarks } = this.currentPose;
    
    // Simple positioning based on shoulders
    const leftShoulder = landmarks[11];
    const rightShoulder = landmarks[12];
    const leftHip = landmarks[23];
    const rightHip = landmarks[24];
    
    if (!leftShoulder || !rightShoulder) return;
    
    const centerX = (leftShoulder.x + rightShoulder.x) / 2;
    const centerY = (leftShoulder.y + rightShoulder.y) / 2;
    const shoulderWidth = Math.abs(leftShoulder.x - rightShoulder.x);
    
    for (const garment of this.activeGarments) {
      if (!garment.mesh) continue;
      
      garment.mesh.position.x = centerX;
      garment.mesh.position.y = centerY - 0.3;
      garment.mesh.scale.x = shoulderWidth * 1.5;
      garment.mesh.scale.y = shoulderWidth * 1.5;
      
      // TODO: Implement cloth physics simulation
      // This would update vertex positions based on PBD solver
    }
  }
  
  private animate(): void {
    if (!this.isActive) return;
    
    requestAnimationFrame(() => this.animate());
    
    // Calculate FPS
    const now = performance.now();
    if (this.lastFrameTime > 0) {
      this.currentFPS = 1000 / (now - this.lastFrameTime);
    }
    this.lastFrameTime = now;
    
    // Draw camera feed
    if (this.video && this.canvas) {
      const ctx = this.canvas.getContext('2d');
      if (ctx) {
        ctx.drawImage(this.video, 0, 0, this.canvas.width, this.canvas.height);
      }
    }
    
    // Render 3D
    if (this.renderer && this.scene && this.threeCamera) {
      this.renderer.render(this.scene, this.threeCamera);
    }
    
    if (this.onFrameProcessed && this.canvas) {
      this.onFrameProcessed(this.canvas);
    }
  }
}

// Export default instance factory
export function createARFitKit(): ARFitKit {
  return new ARFitKit();
}

export default ARFitKit;
