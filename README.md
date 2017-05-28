####################################################################
#
#     music - Qt music player
#
####################################################################

## Description

A simple player for playback media files.

Support next file types:
```
- mp3
- mpga
- wav
- flac
- wma
- ogg
```

Support two mode play:
```
- stream (play from file on fly)
- sound (get media from file to memory buffer and when play)
```

Support next play effects (DSP type):
```
- Normalize
- Mixer
- Echo
- HighPass
- LowPass
- Chorus
- Compressor
- Distortion
- Parameq
- Flange
- PitchShift
- SfxReverb
- Tremolo
```


## Required
```
- Qt5 framework + IDE Qt Creator 3.5.1
- fmodex api library ver.4.X (from http://www.fmod.org/download)
```

## Start application

./music param1 param2

where,
* param1 - stream or sound
* param2 - dsp

