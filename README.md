# react-native-native-video

Native Video Access for React Native

Working for iOS and Android!

Android support is second class, however, unless I finally have more time (or more money lol).

## Installation

```sh
npm install react-native-native-video
```

## Usage

Call `openVideo(uri: string)` to get a `NativeVideoWrapper`.

Get frames by various methods.

`NativeVideoFrameView` takes a `NativeFrameWrapper` in order to display.

```tsx
import * as React from 'react';
import * as ImagePicker from 'expo-image-picker';
import { StyleSheet, View, Text, Button, Alert, TextInput } from 'react-native';
import { NativeVideoFrameView, NativeVideoWrapper, NativeFrameWrapper, openVideo } from "react-native-native-video";
export default function App() {
  const [pickedUri, setPickedUri] = React.useState<string>();
  const [frame, setFrame] = React.useState<NativeFrameWrapper>();
  const vidRef = React.useRef<NativeVideoWrapper>();
  const onPickVideo = React.useCallback(async () => {
    await ImagePicker.getMediaLibraryPermissionsAsync();
    const res = await ImagePicker.launchImageLibraryAsync({ mediaTypes: ImagePicker.MediaTypeOptions.Videos });
    if (res.cancelled) return;
    const uri = res.uri;
    setPickedUri(uri);
  });
  const onVideoProperties = React.useCallback(async () => {
    if (!pickedUri) return;
    const video = vidRef.current;
    if (!vidRef.current || vidRef.current.sourceUri != pickedUri) {
      video = openVideo(pickedUri);
      vidRef.current = video;
    }
    video = video!; // Make typescript not complain.
    Alert.alert(`Opened video`, `Video props are duration : ${video.duration}, frameRate: ${video.frameRate}, numFrames ${video.numFrames}, size ${video.size}`);
    setNumFrames(video.numFrames);
  });

  const onVideoFrameTest = React.useCallback(async () => {
    if (!pickedUri) return;
    let video = vidRef.current;
    if (!vidRef.current || vidRef.current.sourceUri != pickedUri) {
      video = openVideo(pickedUri);
      vidRef.current = video;
    }
    video = video!; // Make typescript not complain.
    const frameIdx = 0;
    const frame = video.getFrameAtIndex(frameIdx);
    Alert.alert(`Video frame ${frameIdx} isValid ${frame.isValid} frameSize ${JSON.stringify(frame.size)}`);
    // const arrayBuffer = frame.arrayBuffer;
    // console.log('arrayBuffer status', arrayBuffer);
    setFrame(frame);
  });

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
      <Slider
        minimumValue={0}
        maximumValue={numFrames - 1}
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
```

## Contributing

See the [contributing guide](CONTRIBUTING.md) to learn how to contribute to the repository and the development workflow.

## License

MIT

---
##### Tip Jar

This thing took a pretty long time to write, and it'll take some chunk of time to maintain too.
If you appreciate my work, help buy me some soda 🥤 via the following routes.

<img src="https://upload.wikimedia.org/wikipedia/commons/5/56/Stellar_Symbol.png" alt="Stellar" height="32"/>

```
Stellar Lumens (XLM) : 
GCVKPZQUDXWVNPIIMF3FXR6KWAOHTEWPZZM2AQE4J3TXR6ZDHXQHP5BQ
```

<img src="https://upload.wikimedia.org/wikipedia/commons/1/19/Coin-ada-big.svg" alt="Cardano" height="32">

```
Cardano (ADA) : 
addr1q9datt8urnyuc2059tquh59sva0pja7jqg4nfhnje7xcy6zpndeesglqkxhjvcgdu820flcecjzunwp6qen4yr92gm6smssug8
```
