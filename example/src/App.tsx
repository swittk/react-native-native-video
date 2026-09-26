import Slider from '@react-native-community/slider';
import * as React from 'react';
import DocumentPicker from 'react-native-document-picker';
import {
  Alert,
  AppState,
  Button,
  NativeModules,
  Platform,
  SafeAreaView,
  ScrollView,
  StyleSheet,
  Text,
  TextInput,
  View,
} from 'react-native';

import {
  NativeFrameWrapper,
  NativeVideoFrameView,
  NativeVideoWrapper,
  openVideo,
} from 'react-native-native-video';

function iosLaunchSetting(name: string): unknown {
  if (Platform.OS !== 'ios') {
    return undefined;
  }
  const settings = NativeModules.SettingsManager?.settings as
    | Record<string, unknown>
    | undefined;
  return settings?.[name];
}

function initialManualUri(): string {
  const value = iosLaunchSetting('NativeVideoSmokeURI');
  return typeof value === 'string' ? value : '';
}

function shouldAutoRunSmoke(): boolean {
  const value = iosLaunchSetting('NativeVideoSmokeAutoRun');
  return value === true || value === 'YES' || value === '1';
}

/** Exercises the legacy bridge installer and the synchronous NativeVideo JSI API. */
export default function App(): React.JSX.Element {
  const videoRef = React.useRef<NativeVideoWrapper>();
  const frameRef = React.useRef<NativeFrameWrapper>();
  const autoSmokeEnabled = React.useRef(shouldAutoRunSmoke()).current;
  const autoSmokeInitialDone = React.useRef(false);
  const autoSmokeBackgrounded = React.useRef(false);
  const autoSmokeResumeDone = React.useRef(false);
  const [sourceUri, setSourceUri] = React.useState('');
  const [manualUri, setManualUri] = React.useState(initialManualUri);
  const [video, setVideo] = React.useState<NativeVideoWrapper>();
  const [frame, setFrame] = React.useState<NativeFrameWrapper>();
  const [frameIndexText, setFrameIndexText] = React.useState('0');
  const [status, setStatus] = React.useState(
    'Pick a local video to exercise openVideo() and the frame APIs.',
  );
  const [autoSmokeStatus, setAutoSmokeStatus] = React.useState(
    autoSmokeEnabled ? 'waiting' : 'off',
  );

  /** Replaces the preview frame and explicitly releases its native pixels. */
  const replaceFrame = React.useCallback((next?: NativeFrameWrapper) => {
    frameRef.current?.close();
    frameRef.current = next;
    setFrame(next);
  }, []);

  /** Releases all native resources held by the current test session. */
  const closeVideo = React.useCallback(() => {
    replaceFrame();
    videoRef.current?.close();
    videoRef.current = undefined;
    setVideo(undefined);
    setStatus('Native video and frame resources closed.');
  }, [replaceFrame]);

  React.useEffect(
    () => () => {
      frameRef.current?.close();
      videoRef.current?.close();
    },
    [],
  );

  /** Opens a selected file through the package's old-architecture JSI installer. */
  const openSelectedVideo = React.useCallback(
    (uri: string) => {
      closeVideo();
      try {
        const opened = openVideo(uri);
        if (!opened.isValid) {
          opened.close();
          throw new Error('The platform decoder could not open this video.');
        }
        videoRef.current = opened;
        setVideo(opened);
        setSourceUri(uri);
        setFrameIndexText('0');
        setStatus(
          `Opened ${opened.numFrames} frames at ${opened.frameRate.toFixed(3)} fps`,
        );
      } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        setStatus(`Open failed: ${message}`);
        Alert.alert('Unable to open video', message);
      }
    },
    [closeVideo],
  );

  /** Copies an iOS security-scoped selection into cache and opens its local URI. */
  const pickVideo = React.useCallback(async () => {
    try {
      const result = await DocumentPicker.pickSingle({
        type: DocumentPicker.types.video,
        copyTo: 'cachesDirectory',
      });
      const source =
        Platform.OS === 'ios' ? result.fileCopyUri ?? result.uri : result.uri;
      openSelectedVideo(source);
    } catch (error) {
      if (!DocumentPicker.isCancel(error)) {
        const message = error instanceof Error ? error.message : String(error);
        setStatus(`Picker failed: ${message}`);
        Alert.alert('Video picker failed', message);
      }
    }
  }, [openSelectedVideo]);

  /** Decodes one frame by index and exercises raw RGBA/base64/MD5 access. */
  const decodeFrameAtIndex = React.useCallback(
    (requestedIndex: number) => {
      const opened = videoRef.current;
      if (!opened || opened.numFrames < 1) {
        return;
      }
      const index = Math.max(0, Math.min(opened.numFrames - 1, requestedIndex));
      const decoded = opened.getFrameAtIndex(index);
      const rgba = decoded.arrayBuffer();
      const base64Prefix = decoded.base64({format: 'png'}).slice(0, 24);
      const digest = decoded.md5() || '(not implemented on this platform)';
      replaceFrame(decoded);
      setFrameIndexText(String(index));
      setStatus(
        `Frame ${decoded.index} @ ${decoded.timestamp.toFixed(6)}s: ` +
          `${rgba.byteLength} RGBA bytes, PNG ${base64Prefix}…, MD5 ${digest}`,
      );
    },
    [replaceFrame],
  );

  /** Uses the presentation-time index and time-based decode APIs together. */
  const decodeMiddleTime = React.useCallback(() => {
    const opened = videoRef.current;
    if (!opened || opened.numFrames < 1) {
      return;
    }
    const time = opened.duration / 2;
    const mappedIndex = opened.getFrameIndexAtTime(time);
    const mappedTimestamp = opened.getFrameTimestampAtIndex(mappedIndex);
    const decoded = opened.getFrameAtTime(time);
    replaceFrame(decoded);
    setFrameIndexText(String(mappedIndex));
    setStatus(
      `Time ${time.toFixed(6)}s maps to frame ${mappedIndex} ` +
        `at ${mappedTimestamp.toFixed(6)}s; decoded frame ${decoded.index}.`,
    );
  }, [replaceFrame]);

  /** Exercises batch decoding while retaining one frame for native preview. */
  const decodeBatch = React.useCallback(() => {
    const opened = videoRef.current;
    if (!opened || opened.numFrames < 1) {
      return;
    }
    const requestedIndex = Number.parseInt(frameIndexText, 10);
    const start = Number.isFinite(requestedIndex) ? requestedIndex : 0;
    const decoded = opened.getFramesAtIndex(start, 3);
    if (decoded.length === 0) {
      setStatus('Batch decode returned no frames.');
      return;
    }
    const totalBytes = decoded.reduce(
      (sum, item) => sum + item.arrayBuffer().byteLength,
      0,
    );
    const [preview, ...remainder] = decoded;
    remainder.forEach(item => item.close());
    replaceFrame(preview);
    setFrameIndexText(String(preview.index));
    setStatus(`Batch decoded ${decoded.length} frames (${totalBytes} RGBA bytes).`);
  }, [frameIndexText, replaceFrame]);

  const exerciseDecodedFrame = React.useCallback(
    (opened: NativeVideoWrapper, index: number): void => {
      const clamped = Math.max(0, Math.min(opened.numFrames - 1, index));
      const decoded = opened.getFrameAtIndex(clamped);
      const rgba = decoded.arrayBuffer();
      if (rgba.byteLength !== opened.size.width * opened.size.height * 4) {
        decoded.close();
        throw new Error(`Unexpected RGBA byte length for frame ${clamped}.`);
      }
      if (!decoded.base64({format: 'png'}).startsWith('iVBOR')) {
        decoded.close();
        throw new Error(`PNG conversion failed for frame ${clamped}.`);
      }
      decoded.md5();
      decoded.close();
    },
    [],
  );

  const runInitialAutoSmoke = React.useCallback(() => {
    const uri = manualUri.trim();
    if (!uri) {
      throw new Error('NativeVideoSmokeURI is required for auto smoke.');
    }
    openSelectedVideo(uri);
    const opened = videoRef.current;
    if (!opened || opened.numFrames < 1) {
      throw new Error('Auto smoke could not open the video.');
    }

    [0, opened.numFrames - 1, Math.floor(opened.numFrames / 2), 5].forEach(
      index => exerciseDecodedFrame(opened, index),
    );

    const middleTime = opened.duration / 2;
    const mappedIndex = opened.getFrameIndexAtTime(middleTime);
    opened.getFrameTimestampAtIndex(mappedIndex);
    const timeFrame = opened.getFrameAtTime(middleTime);
    timeFrame.arrayBuffer();
    timeFrame.close();

    const batch = opened.getFramesAtIndex(Math.max(0, mappedIndex - 1), 3);
    if (batch.length === 0) {
      throw new Error('Auto smoke batch decode returned no frames.');
    }
    batch.forEach(item => {
      item.arrayBuffer();
      item.close();
    });

    const previewIndex = Math.min(5, opened.numFrames - 1);
    const preview = opened.getFrameAtIndex(previewIndex);
    preview.arrayBuffer();
    replaceFrame(preview);
    setFrameIndexText(String(previewIndex));
    setAutoSmokeStatus('initial passed');
    setStatus(
      `AUTO QA initial passed: ${opened.numFrames} frames; preview ${previewIndex}.`,
    );
  }, [exerciseDecodedFrame, manualUri, openSelectedVideo, replaceFrame]);

  const runResumeAutoSmoke = React.useCallback(() => {
    const opened = videoRef.current;
    if (!opened || opened.numFrames < 1) {
      throw new Error('Auto smoke resume lost the open video.');
    }

    [1, Math.max(0, opened.numFrames - 2), 3].forEach(index =>
      exerciseDecodedFrame(opened, index),
    );

    const uri = manualUri.trim();
    closeVideo();
    openSelectedVideo(uri);
    const reopened = videoRef.current;
    if (!reopened || reopened.numFrames < 1) {
      throw new Error('Auto smoke could not reopen the video.');
    }
    const finalIndex = Math.min(12, reopened.numFrames - 1);
    const preview = reopened.getFrameAtIndex(finalIndex);
    preview.arrayBuffer();
    replaceFrame(preview);
    setFrameIndexText(String(finalIndex));
    setAutoSmokeStatus('resume passed');
    setStatus(
      `AUTO QA resume passed: reopened video; preview ${finalIndex}.`,
    );
  }, [
    closeVideo,
    exerciseDecodedFrame,
    manualUri,
    openSelectedVideo,
    replaceFrame,
  ]);

  React.useEffect(() => {
    if (
      !autoSmokeEnabled ||
      autoSmokeInitialDone.current ||
      !manualUri.trim()
    ) {
      return;
    }
    autoSmokeInitialDone.current = true;
    try {
      runInitialAutoSmoke();
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      setAutoSmokeStatus(`failed: ${message}`);
      setStatus(`AUTO QA failed: ${message}`);
    }
  }, [autoSmokeEnabled, manualUri, runInitialAutoSmoke]);

  React.useEffect(() => {
    if (!autoSmokeEnabled) {
      return;
    }
    const subscription = AppState.addEventListener('change', nextState => {
      if (nextState !== 'active') {
        autoSmokeBackgrounded.current = true;
        return;
      }
      if (
        !autoSmokeBackgrounded.current ||
        !autoSmokeInitialDone.current ||
        autoSmokeResumeDone.current
      ) {
        return;
      }
      autoSmokeResumeDone.current = true;
      try {
        runResumeAutoSmoke();
      } catch (error) {
        const message = error instanceof Error ? error.message : String(error);
        setAutoSmokeStatus(`failed: ${message}`);
        setStatus(`AUTO QA resume failed: ${message}`);
      }
    });
    return () => subscription.remove();
  }, [autoSmokeEnabled, runResumeAutoSmoke]);

  const requestedIndex = Number.parseInt(frameIndexText, 10);
  const sliderMaximum = Math.max(0, (video?.numFrames ?? 1) - 1);

  return (
    <SafeAreaView style={styles.safeArea}>
      <ScrollView contentContainerStyle={styles.container}>
        <Text style={styles.title}>NativeVideo RN 0.73 Old Architecture</Text>
        <Text style={styles.metadata}>Auto QA: {autoSmokeStatus}</Text>
        <Button accessibilityLabel="Pick and open a local video" title="Pick and open a local video" onPress={pickVideo} />
        <TextInput
          accessibilityLabel="Video URI"
          autoCapitalize="none"
          autoCorrect={false}
          onChangeText={setManualUri}
          placeholder="Optional local URI/path for repeatable testing"
          placeholderTextColor="#64748b"
          style={styles.input}
          value={manualUri}
        />
        <View style={styles.buttonGap}>
          <Button
            accessibilityLabel="Open entered URI"
            disabled={!manualUri.trim()}
            title="Open entered URI"
            onPress={() => openSelectedVideo(manualUri.trim())}
          />
        </View>
        <Text selectable style={styles.uri}>
          {sourceUri || 'No video selected'}
        </Text>
        {video ? (
          <Text style={styles.metadata}>
            {`${video.size.width}×${video.size.height} • ${video.duration.toFixed(
              3,
            )}s • ${video.numFrames} frames • ${video.frameRate.toFixed(3)} fps`}
          </Text>
        ) : null}

        <TextInput
          accessibilityLabel="Frame index"
          editable={Boolean(video)}
          keyboardType="number-pad"
          onChangeText={setFrameIndexText}
          style={styles.input}
          value={frameIndexText}
        />
        <Slider
          disabled={!video}
          maximumValue={sliderMaximum}
          minimumValue={0}
          onSlidingComplete={value => decodeFrameAtIndex(Math.floor(value))}
          step={1}
          value={Math.max(
            0,
            Math.min(sliderMaximum, Number.isFinite(requestedIndex) ? requestedIndex : 0),
          )}
        />

        <View style={styles.buttonGap}>
          <Button
            accessibilityLabel="Decode frame at index"
            disabled={!video}
            title="Decode frame at index"
            onPress={() => decodeFrameAtIndex(requestedIndex || 0)}
          />
        </View>
        <View style={styles.buttonGap}>
          <Button accessibilityLabel="Decode middle timestamp" disabled={!video} title="Decode middle timestamp" onPress={decodeMiddleTime} />
        </View>
        <View style={styles.buttonGap}>
          <Button accessibilityLabel="Decode three-frame batch" disabled={!video} title="Decode three-frame batch" onPress={decodeBatch} />
        </View>

        <NativeVideoFrameView
          frameData={frame}
          resizeMode="contain"
          style={styles.preview}
        />
        <Text selectable style={styles.status}>
          {status}
        </Text>
        <Button accessibilityLabel="Close native resources" disabled={!video} title="Close native resources" onPress={closeVideo} />
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safeArea: {backgroundColor: '#111827', flex: 1},
  container: {gap: 10, padding: 18},
  title: {color: '#f9fafb', fontSize: 22, fontWeight: '700'},
  uri: {color: '#93c5fd', fontSize: 12},
  metadata: {color: '#e5e7eb'},
  input: {
    backgroundColor: '#f9fafb',
    borderRadius: 6,
    color: '#111827',
    fontSize: 16,
    paddingHorizontal: 10,
    paddingVertical: 8,
  },
  buttonGap: {marginTop: 2},
  preview: {
    alignSelf: 'stretch',
    backgroundColor: '#030712',
    borderColor: '#374151',
    borderRadius: 8,
    borderWidth: 1,
    height: 260,
  },
  status: {color: '#d1d5db', minHeight: 60},
});
