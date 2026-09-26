import React from 'react';
import {
  requireNativeComponent,
  StyleProp,
  ViewStyle,
} from 'react-native';

import NativeVideoBinding from './NativeVideo';

declare global {
  // New Architecture uses the supported platform binding installers. Legacy
  // bridge modules install the same HostObject factory from their RN runtime.
  // eslint-disable-next-line no-var
  var SKRNNativeVideoOpenVideo:
    | ((uri: string) => NativeVideoWrapper)
    | undefined;
}

export type NativeVideoSize = Readonly<{
  width: number;
  height: number;
}>;

export interface NativeFrameWrapper {
  /** A tightly packed, display-oriented RGBA8 frame (4 bytes per pixel). */
  arrayBuffer(): ArrayBuffer;
  readonly size: NativeVideoSize;
  readonly bytesPerRow: number;
  readonly pixelFormat: 'rgba8';
  readonly platform: 'iOS' | 'Android';
  readonly isValid: boolean;
  /** Zero-based decoded frame index. */
  readonly index: number;
  /** Presentation timestamp in seconds. */
  readonly timestamp: number;
  /** Opaque, lifetime-safe identifier used by the native preview view. */
  readonly nativeId: string;
  /** @deprecated Compatibility alias of nativeId; this is no longer a pointer. */
  readonly nativePtrStr: string;
  /** Releases the decoded native frame. Safe to call more than once. */
  close(): void;
  base64(opts?: { format?: 'png' | 'jpg' | 'jpeg' }): string;
  /** MD5 of raw RGBA8 bytes. Android currently returns an empty string. */
  md5(): string;
}

export interface NativeVideoWrapper {
  readonly sourceUri: string;
  readonly isValid: boolean;
  /** Duration in seconds. */
  readonly duration: number;
  readonly numFrames: number;
  /** Average/nominal frames per second. */
  readonly frameRate: number;
  /** Display-oriented dimensions. */
  readonly size: NativeVideoSize;
  /** Exact PTS when available; otherwise a nominal FPS-derived timestamp. */
  getFrameTimestampAtIndex(index: number): number;
  /** Selects the frame at or immediately before this presentation time. */
  getFrameIndexAtTime(timeSeconds: number): number;
  getFrameAtIndex(index: number): NativeFrameWrapper;
  getFramesAtIndex(index: number, length: number): NativeFrameWrapper[];
  getFrameAtTime(timeSeconds: number): NativeFrameWrapper;
  /** Releases decoder resources. Safe to call more than once. */
  close(): void;
}

/** Compatibility helper retained from the original package. */
export function multiply(a: number, b: number): Promise<number> {
  return Promise.resolve(NativeVideoBinding.multiply(a, b));
}

/**
 * Opens a native decoder synchronously and returns a C++ HostObject. The URI
 * must identify a local file on iOS; Android additionally accepts data sources
 * supported by MediaMetadataRetriever.
 */
export function openVideo(uri: string): NativeVideoWrapper {
  const factory = globalThis.SKRNNativeVideoOpenVideo;
  if (typeof factory !== 'function') {
    throw new Error(
      "react-native-native-video's JSI bindings were not installed. " +
        'Rebuild the native app and verify that NativeVideo is linked.'
    );
  }
  return factory(uri);
}

type NativeVideoFrameNativeProps = {
  frameId?: string;
  resizeMode?: 'contain' | 'cover' | 'stretch';
  style?: StyleProp<ViewStyle>;
};

const SKRNNativeFrameView =
  requireNativeComponent<NativeVideoFrameNativeProps>('SKRNNativeFrameView');

export type NativeVideoFrameViewProps = Readonly<{
  frameData?: NativeFrameWrapper;
  resizeMode?: 'contain' | 'cover' | 'stretch';
  style?: StyleProp<ViewStyle>;
}>;

/**
 * Lightweight native preview. Frame extraction and raw access do not depend
 * on this compatibility view.
 */
export class NativeVideoFrameView extends React.PureComponent<NativeVideoFrameViewProps> {
  render(): React.ReactNode {
    const { frameData, ...props } = this.props;
    return (
      <SKRNNativeFrameView
        {...props}
        frameId={frameData?.nativeId}
      />
    );
  }
}
