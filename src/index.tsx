import React from 'react';
import { NativeModules, Platform, requireNativeComponent, StyleProp, ViewStyle } from 'react-native';

const LINKING_ERROR =
  `The package 'react-native-native-video' doesn't seem to be linked. Make sure: \n\n` +
  Platform.select({ ios: "- You have run 'pod install'\n", default: '' }) +
  '- You rebuilt the app after installing the package\n' +
  '- You are not using Expo managed workflow\n';

const NativeVideo = NativeModules.NativeVideo
  ? NativeModules.NativeVideo
  : new Proxy(
    {},
    {
      get() {
        throw new Error(LINKING_ERROR);
      },
    }
  );

export function multiply(a: number, b: number): Promise<number> {
  return NativeVideo.multiply(a, b);
}

export function openVideo(url: string): NativeVideoWrapper {
  return (global as any).SKRNNativeVideoOpenVideo(url);
}

export interface NativeFrameWrapper {
  arrayBuffer(): ArrayBuffer;
  size: { width: number, height: number };
  isValid: boolean;
  /** Currently iOS only */
  resizeMode?: 'contain' | 'cover' | 'stretch';
  /** Call this to free the frame once it gets unused; best to call it or risk potential leaks */
  close(): void;
}

export interface NativeVideoWrapper {
  sourceUri: string;
  isValid: boolean;
  /** Duration in seconds */
  duration: number;
  numFrames: number;
  frameRate: number;
  size: { width: number, height: number };
  getFrameAtIndex(idx: number): NativeFrameWrapper;
  getFramesAtIndex(idx: number, len: number): NativeFrameWrapper[];
  getFrameAtTime(time: number): NativeFrameWrapper;
}

const SKRNNativeFrameView = requireNativeComponent<NativeVideoFrameViewProps>('SKRNNativeFrameView');

type NativeVideoFrameViewProps = { frameData?: NativeFrameWrapper, style?: StyleProp<ViewStyle> };
type NativeVideoFrameViewState = {};
export class NativeVideoFrameView extends React.PureComponent<NativeVideoFrameViewProps, NativeVideoFrameViewState> {
  render() {
    const { frameData, style } = this.props;
    return <SKRNNativeFrameView
      frameData={frameData}
      style={style}
    />
  }
}