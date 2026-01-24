import React from 'react';
import { requireNativeComponent, ViewProps } from 'react-native';

interface ARFitKitViewProps extends ViewProps {
    // Add custom props here if needed, e.g.,
    // onFrameUpdate?: (event: any) => void;
}

const NativeARFitKitView = requireNativeComponent<ARFitKitViewProps>('ARFitKitView');

export const ARFitKitView: React.FC<ARFitKitViewProps> = (props) => {
    return <NativeARFitKitView { ...props } />;
};
