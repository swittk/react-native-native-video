import { NativeModules, Platform } from 'react-native';

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

export function openVideo(url: string) : NativeVideoWrapper {
  return (global as any).SKRNNativeVideoOpenVideo(url);
}

interface NativeFrameWrapper {
  arrayBuffer() : ArrayBuffer;
  size(): {width: number, height: number};
  isValid: boolean;
}

interface NativeVideoWrapper {
  isValid: boolean;
  /** Duration in seconds */
  duration: number;
  numFrames: number;
  frameRate: number;
  size: {width: number, height: number};
  getFrameAtIndex(idx: number): NativeFrameWrapper;
  getFramesAtIndex(idx: number, len: number): NativeFrameWrapper[];
  getFrameAtTime(time: number): NativeFrameWrapper;
}