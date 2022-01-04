import * as React from 'react';
import * as ImagePicker from 'expo-image-picker';

import { StyleSheet, View, Text, Button, Alert } from 'react-native';
import { multiply, openVideo } from 'react-native-native-video';

export default function App() {
  const [result, setResult] = React.useState<number | undefined>();
  const [pickedUri, setPickedUri] = React.useState<string>();

  const onPickVideo = async () => {
    await ImagePicker.getMediaLibraryPermissionsAsync();
    const res = await ImagePicker.launchImageLibraryAsync({mediaTypes:ImagePicker.MediaTypeOptions.Videos});
    if(res.cancelled) return;
    const uri = res.uri;
    setPickedUri(uri);
  }
  const doStuff = async () => {
    if(!pickedUri) return;
    const video = openVideo(pickedUri);
    Alert.alert(`Opened video`, `Video props are duration : ${video.duration}, frameRate: ${video.frameRate}, numFrames ${video.numFrames}, size ${video.size}`);
  }

  React.useEffect(() => {
    multiply(3, 7).then(setResult);
  }, []);

  return (
    <View style={styles.container}>
      <Text>Result: {result}</Text>
      <Button title='Pick Video' onPress={onPickVideo}/>
      <Text>Video uri: {pickedUri}</Text>
      <Button title='Do stuff' onPress={doStuff}/>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
  },
  box: {
    width: 60,
    height: 60,
    marginVertical: 20,
  },
});
