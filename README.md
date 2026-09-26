# react-native-native-video

Native, synchronous video metadata and frame access for React Native on iOS
and Android.

The package supports both the RN 0.73 legacy architecture and the RN 0.83 New
Architecture. iOS uses AVFoundation; Android uses the platform media APIs.

## Installation

```sh
pnpm add react-native-native-video
```

Install iOS pods after adding the dependency.

## Usage

`openVideo(uri)` returns a native-backed video object. Frames can be selected
by index or presentation time, read as RGBA8, encoded, previewed, and closed
explicitly.

```tsx
import {
  NativeVideoFrameView,
  openVideo,
} from 'react-native-native-video';

const video = openVideo(localVideoUri);
const middleIndex = video.getFrameIndexAtTime(video.duration / 2);
const frame = video.getFrameAtIndex(middleIndex);

console.log({
  duration: video.duration,
  frameRate: video.frameRate,
  numFrames: video.numFrames,
  timestamp: frame.timestamp,
  rgbaBytes: frame.arrayBuffer().byteLength,
});

// Render while `frame` remains open:
// <NativeVideoFrameView frameData={frame} style={{height: 240}} />

frame.close();
video.close();
```

The real [RN 0.73 example](example/) uses a document picker and exercises
metadata, timestamp/index mapping, individual and batch frame decode, RGBA8,
PNG/base64, MD5, native preview, and explicit close/reopen behavior. It keeps
`newArchEnabled=false` specifically to validate the legacy RCTBridgeModule/JSI
installer. The package root remains on RN 0.83.10 for New Architecture builds.

## Contributing

See the [contributing guide](CONTRIBUTING.md).

## License

MIT

---

If this project helps you, the original author accepts tips:

```text
Stellar Lumens (XLM):
GCVKPZQUDXWVNPIIMF3FXR6KWAOHTEWPZZM2AQE4J3TXR6ZDHXQHP5BQ

Cardano (ADA):
addr1q9datt8urnyuc2059tquh59sva0pja7jqg4nfhnje7xcy6zpndeesglqkxhjvcgdu820flcecjzunwp6qen4yr92gm6smssug8
```
