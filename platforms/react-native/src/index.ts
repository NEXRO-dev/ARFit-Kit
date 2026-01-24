import { NativeEventEmitter, NativeModules, Platform } from 'react-native';

const LINKING_ERROR =
    `The package 'arfit-kit-react-native' doesn't seem to be linked. Make sure: \n\n` +
    Platform.select({ ios: "- You have run 'pod install'\n", default: '' }) +
    '- You rebuilt the app after installing the package\n' +
    '- You are not using Expo Go\n';

const ARFitKitModule = NativeModules.ARFitKit || {
    addListener: () => { },
    removeListeners: () => { },
    initialize: () => Promise.resolve(),
    startSession: () => Promise.resolve(),
    stopSession: () => Promise.resolve(),
    loadGarment: () => Promise.resolve({ id: 'dummy' }),
    tryOn: () => Promise.resolve(),
    removeGarment: () => Promise.resolve(),
    removeAllGarments: () => Promise.resolve(),
    captureSnapshot: () => Promise.resolve(''),
};

const eventEmitter = new NativeEventEmitter(ARFitKitModule as any);

export interface SessionConfig {
    targetFPS?: number;
    enableClothSimulation?: boolean;
    enableShadows?: boolean;
    maxGarments?: number;
}

export interface Garment {
    id: string;
    type: GarmentType;
    imageUri: string;
}

export enum GarmentType {
    UNKNOWN = 0,
    TSHIRT = 1,
    SHIRT = 2,
    JACKET = 3,
    COAT = 4,
    DRESS = 5,
    PANTS = 6,
    SHORTS = 7,
    SKIRT = 8,
}

export interface BodyPose {
    landmarks: Array<{ x: number; y: number; z: number }>;
    confidence: number;
}

// Event listeners
export function onPoseUpdated(callback: (pose: BodyPose) => void) {
    return eventEmitter.addListener('onPoseUpdated', callback);
}

export function onFPSUpdated(callback: (fps: number) => void) {
    return eventEmitter.addListener('onFPSUpdated', callback);
}

export function onError(callback: (error: string) => void) {
    return eventEmitter.addListener('onError', callback);
}

// Core API
export function initialize(config?: SessionConfig): Promise<void> {
    return ARFitKitModule.initialize(config || {});
}

export function startSession(): Promise<void> {
    return ARFitKitModule.startSession();
}

export function stopSession(): Promise<void> {
    return ARFitKitModule.stopSession();
}

export function loadGarment(
    imageUri: string,
    type: GarmentType = GarmentType.UNKNOWN
): Promise<Garment> {
    return ARFitKitModule.loadGarment(imageUri, type);
}

export function tryOn(garmentId: string): Promise<void> {
    return ARFitKitModule.tryOn(garmentId);
}

export function removeGarment(garmentId: string): Promise<void> {
    return ARFitKitModule.removeGarment(garmentId);
}

export function removeAllGarments(): Promise<void> {
    return ARFitKitModule.removeAllGarments();
}

export function captureSnapshot(): Promise<string> {
    return ARFitKitModule.captureSnapshot();
}

export { ARFitKitView } from './ARFitKitView';

export default {
    initialize,
    startSession,
    stopSession,
    loadGarment,
    tryOn,
    removeGarment,
    removeAllGarments,
    captureSnapshot,
    onPoseUpdated,
    onFPSUpdated,
    onError,
    GarmentType,
};
