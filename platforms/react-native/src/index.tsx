/**
 * ARFitKit - React Native SDK for AR Virtual Try-On
 *
 * オープンソースのAR試着SDK
 * React NativeでiOS/Android向けAR機能を提供
 */

import {
    forwardRef,
    useCallback,
    useEffect,
    useImperativeHandle,
    useRef,
    useState,
} from 'react';
import {
    NativeModules,
    requireNativeComponent,
    StyleSheet,
    View,
    ViewStyle,
} from 'react-native';


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

export interface Garment {
    id: string;
    type: GarmentType;
    imageUri: string;
    isLoading: boolean;
    conversionProgress: number;
}

export interface BodyPose {
    landmarks: Array<{ x: number; y: number; z: number }>;
    visibility: number[];
    confidence: number;
    timestamp: number;
}

export interface ARFitKitViewProps {
    style?: ViewStyle;
    config?: SessionConfig;
    onReady?: () => void;
    onPoseUpdated?: (pose: BodyPose) => void;
    onError?: (error: Error) => void;
    onFrameProcessed?: () => void;
}

export interface ARFitKitViewRef {
    captureSnapshot: () => Promise<string>;
    getCurrentFPS: () => number;
}

// Native module
const ARFitKitNative = NativeModules.ARFitKit || {
    initialize: () => Promise.resolve(),
    startSession: () => Promise.resolve(),
    stopSession: () => Promise.resolve(),
    loadGarment: () => Promise.resolve('dummy_id'),
    tryOn: () => Promise.resolve(),
    removeGarment: () => Promise.resolve(),
    removeAllGarments: () => Promise.resolve(),
    captureSnapshot: () => Promise.resolve(''),
};

// Native component
const NativeARFitKitView = requireNativeComponent<any>('ARFitKitView');

/**
 * ARFitKit View Component
 */
export const ARFitKitView = forwardRef<ARFitKitViewRef, ARFitKitViewProps>(
    (
        {
            style,
            config = {},
            onReady,
            onPoseUpdated,
            onError,
            onFrameProcessed,
        },
        ref
    ) => {
        const nativeRef = useRef<any>(null);
        const [isReady, setIsReady] = useState(false);
        const [currentFPS, setCurrentFPS] = useState(0);

        useEffect(() => {
            // Initialize native module
            ARFitKitNative.initialize(config)
                .then(() => {
                    setIsReady(true);
                    onReady?.();
                })
                .catch((error: Error) => {
                    onError?.(error);
                });

            return () => {
                ARFitKitNative.stopSession();
            };
        }, []);

        useImperativeHandle(ref, () => ({
            captureSnapshot: async () => {
                return ARFitKitNative.captureSnapshot();
            },
            getCurrentFPS: () => currentFPS,
        }));

        const handlePoseUpdate = useCallback(
            (event: any) => {
                const pose: BodyPose = event.nativeEvent;
                onPoseUpdated?.(pose);
            },
            [onPoseUpdated]
        );

        const handleFrame = useCallback(
            (event: any) => {
                setCurrentFPS(event.nativeEvent.fps);
                onFrameProcessed?.();
            },
            [onFrameProcessed]
        );

        const handleError = useCallback(
            (event: any) => {
                onError?.(new Error(event.nativeEvent.message));
            },
            [onError]
        );

        return (
            <View style={[styles.container, style]}>
                <NativeARFitKitView
                    ref={nativeRef}
                    style={styles.view}
                    config={config}
                    onPoseUpdate={handlePoseUpdate}
                    onFrame={handleFrame}
                    onError={handleError}
                />
            </View>
        );
    }
);

const styles = StyleSheet.create({
    container: {
        flex: 1,
        backgroundColor: 'black',
    },
    view: {
        flex: 1,
    },
});

// Hook for ARFitKit
export function useARFitKit() {
    const [isSessionActive, setIsSessionActive] = useState(false);
    const [activeGarments, setActiveGarments] = useState<Garment[]>([]);
    const [currentPose, setCurrentPose] = useState<BodyPose | null>(null);
    const [isLoading, setIsLoading] = useState(false);

    const loadGarment = useCallback(
        async (imageUri: string, type: GarmentType = GarmentType.UNKNOWN): Promise<Garment> => {
            setIsLoading(true);
            try {
                const garmentId = await ARFitKitNative.loadGarment(imageUri, type);
                const garment: Garment = {
                    id: garmentId,
                    type,
                    imageUri,
                    isLoading: false,
                    conversionProgress: 1,
                };
                return garment;
            } finally {
                setIsLoading(false);
            }
        },
        []
    );

    const tryOn = useCallback(async (garment: Garment): Promise<void> => {
        await ARFitKitNative.tryOn(garment.id);
        setActiveGarments((prev) => [...prev, garment]);
    }, []);

    const removeGarment = useCallback(async (garment: Garment): Promise<void> => {
        await ARFitKitNative.removeGarment(garment.id);
        setActiveGarments((prev) => prev.filter((g) => g.id !== garment.id));
    }, []);

    const removeAllGarments = useCallback(async (): Promise<void> => {
        await ARFitKitNative.removeAllGarments();
        setActiveGarments([]);
    }, []);

    return {
        isSessionActive,
        activeGarments,
        currentPose,
        isLoading,
        loadGarment,
        tryOn,
        removeGarment,
        removeAllGarments,
    };
}

// Standalone functions (for non-hook usage)
export async function initialize(config: SessionConfig = {}): Promise<void> {
    return ARFitKitNative.initialize(config);
}

export async function loadGarment(
    imageUri: string,
    type: GarmentType = GarmentType.UNKNOWN
): Promise<Garment> {
    const garmentId = await ARFitKitNative.loadGarment(imageUri, type);
    return {
        id: garmentId,
        type,
        imageUri,
        isLoading: false,
        conversionProgress: 1,
    };
}

export async function tryOn(garment: Garment): Promise<void> {
    return ARFitKitNative.tryOn(garment.id);
}

export async function removeGarment(garment: Garment): Promise<void> {
    return ARFitKitNative.removeGarment(garment.id);
}

export async function removeAllGarments(): Promise<void> {
    return ARFitKitNative.removeAllGarments();
}

export async function captureSnapshot(): Promise<string> {
    return ARFitKitNative.captureSnapshot();
}

export default {
    ARFitKitView,
    useARFitKit,
    initialize,
    loadGarment,
    tryOn,
    removeGarment,
    removeAllGarments,
    captureSnapshot,
    GarmentType,
};
