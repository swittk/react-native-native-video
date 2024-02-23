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
    if (res.canceled) return;
    const uri = res.assets[0].uri;
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
  React.useEffect(() => {
    if (!frame) return;
    // const arrBuf = frame.arrayBuffer();
    // const fArr = new Uint8Array(arrBuf);
    // console.log('frame arraybuf is', fArr.slice(0, 8).map((v)=>Number(v)));
    // console.log('frame arraybuf len', fArr.length);
    // fetch('http://10.146.6.56:44411/post_file', {
    //   headers: {
    //     entryname: 'switty',
    //     filename: 'test.raw',
    //     username: 'switt',
    //     'Content-Type': 'application/octet-stream'
    //   },
    //   method: 'POST',
    //   body: fArr,
    // }).then(() => { console.log('posed arraybuf'); });
    // console.log('frame md5 is', frame.md5());
  }, [frame]);

  const onVideoFrameTest = async () => {
    if (!pickedUri) return;
    let video = vidRef.current;
    if (!vidRef.current || vidRef.current.sourceUri != pickedUri) {
      video = openVideo(pickedUri);
      vidRef.current = video;
    }
    video = video!; // Make typescript not complain.
    const frame = video.getFrameAtIndex(frameIdx);
    console.log(`Video frame is${JSON.stringify(frame)}`);
    const arrayBuffer = frame.arrayBuffer();
    // console.log('arrayBuffer status', arrayBuffer.byteLength);
    const arrBufView = new Uint8Array(arrayBuffer);
    console.log('first 8 arraybuffer data', arrBufView.slice(0, 8));
    setFrame(frame);
  }
  const onTryBase64 = () => {
    console.log('got b64', frame?.base64().slice(0, 128));
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
      <View style={{ height: 44 }} />
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
        style={{ backgroundColor: 'green', borderWidth: 1, borderRadius: 8, flex: 1, alignSelf: 'stretch' }}
        frameData={frame}
      />
      <Button title='See base64 in console' onPress={onTryBase64} />
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
