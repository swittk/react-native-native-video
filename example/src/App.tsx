import * as React from 'react';
import * as ImagePicker from 'expo-image-picker';

import { StyleSheet, View, Text, Button, Alert, TextInput } from 'react-native';
import { openVideo, NativeVideoWrapper, NativeVideoFrameView, NativeFrameWrapper } from 'react-native-native-video';
import Slider from '@react-native-community/slider';

export default function App() {
  const [pickedUri, setPickedUri] = React.useState<string>();
  const [frameIdx, setFrameIdx] = React.useState(0);
  const [frame, setFrame] = React.useState<NativeFrameWrapper>();
  const [goToFrame, setGoToFrame] = React.useState<string>(String(frameIdx));
  const [numFrames, setNumFrames] = React.useState(0);
  const vidRef = React.useRef<NativeVideoWrapper>();
  const onPickVideo = async () => {
    await ImagePicker.getMediaLibraryPermissionsAsync();
    const res = await ImagePicker.launchImageLibraryAsync({ mediaTypes: ImagePicker.MediaTypeOptions.Videos });
    if (res.cancelled) return;
    const uri = res.uri;
    setPickedUri(uri);
  }
  const onVideoProperties = async () => {
    if (!pickedUri) return;
    let video = vidRef.current;
    if (!vidRef.current || vidRef.current.sourceUri != pickedUri) {
      video = openVideo(pickedUri);
      vidRef.current = video;
    }
    video = video!; // Make typescript not complain.
    Alert.alert(`Opened video`, `Video props are duration : ${video.duration}, frameRate: ${video.frameRate}, numFrames ${video.numFrames}, size ${video.size}`);
    setNumFrames(video.numFrames);
  }

  const onVideoFrameTest = async () => {
    if (!pickedUri) return;
    let video = vidRef.current;
    if (!vidRef.current || vidRef.current.sourceUri != pickedUri) {
      video = openVideo(pickedUri);
      vidRef.current = video;
    }
    video = video!; // Make typescript not complain.
    const frame = video.getFrameAtIndex(frameIdx);
    Alert.alert(`Video frame ${frameIdx} isValid ${frame.isValid} frameSize ${JSON.stringify(frame.size)}`);
    // const arrayBuffer = frame.arrayBuffer;
    // console.log('arrayBuffer status', arrayBuffer);
    setFrame(frame);
  }

  const recentAnimFrame = React.useRef<ReturnType<typeof requestAnimationFrame>>();
  const safeSetViewFrameIndex = React.useCallback((idx: number) => {
    if (recentAnimFrame.current) {
      cancelAnimationFrame(recentAnimFrame.current);
    }
    const me = requestAnimationFrame(() => {
      const vid = vidRef.current;
      if (!vid) return;
      const frame = vid.getFrameAtIndex(Math.floor(idx));
      setFrame(frame);
      if (recentAnimFrame.current == me) {
        recentAnimFrame.current = undefined;
      }
    });
    recentAnimFrame.current = me;
  }, []);

  return (
    <View style={styles.container}>
      <Button title='Pick Video' onPress={onPickVideo} />
      <TextInput style={{ fontSize: 16, padding: 8, borderRadius: 6, borderWidth: 1, borderColor: '#888' }} value={goToFrame} onChangeText={setGoToFrame} onEndEditing={() => {
        let num = Number(goToFrame);
        if (Number.isNaN(num)) {
          Alert.alert('enter valid number pls');
          return;
        }
        setFrameIdx(num);
      }} />
      <Slider
        minimumValue={0}
        maximumValue={numFrames - 1}
        minimumTrackTintColor="#FFFFFF"
        maximumTrackTintColor="#000000"
        onValueChange={(v) => {
          const vid = vidRef.current;
          if (!vid) return;
          safeSetViewFrameIndex(Math.floor(v));
        }}
      />
      <Text>Video uri: {pickedUri}</Text>
      <Button title='Get video properties' onPress={onVideoProperties} />
      <Button title='Test Frame getting' onPress={onVideoFrameTest} />
      <NativeVideoFrameView
        style={{ backgroundColor: 'blue', borderWidth: 1, borderRadius: 8, flex: 1, alignSelf: 'stretch' }}
        frameData={frame}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    alignItems: 'stretch',
    justifyContent: 'center',
  },
  box: {
    width: 60,
    height: 60,
    marginVertical: 20,
  },
});
