## Fork of the AOSP 'platform_external_sonivox' project to use it outside of Android

# NOTICE: This repository is now deprecated and has been transferred to [EmbeddedSynth/sonivox](https://github.com/EmbeddedSynth/sonivox).

[![Linux CI](https://github.com/pedrolcl/sonivox/actions/workflows/linux-ci.yml/badge.svg)](https://github.com/pedrolcl/sonivox/actions/workflows/linux-ci.yml)

[![Windows MSYS2 CI](https://github.com/pedrolcl/sonivox/actions/workflows/win-msys2.yml/badge.svg)](https://github.com/pedrolcl/sonivox/actions/workflows/win-msys2.yml)

[![Windows MSVC CI](https://github.com/pedrolcl/sonivox/actions/workflows/win-msvc.yml/badge.svg)](https://github.com/pedrolcl/sonivox/actions/workflows/win-msvc.yml)

[![macOS CI](https://github.com/pedrolcl/sonivox/actions/workflows/mac-ci.yml/badge.svg)](https://github.com/pedrolcl/sonivox/actions/workflows/mac-ci.yml)

[![FreeBSD CI](https://github.com/pedrolcl/sonivox/actions/workflows/freebsd-ci.yml/badge.svg)](https://github.com/pedrolcl/sonivox/actions/workflows/freebsd-ci.yml)

[![Android CI](https://github.com/pedrolcl/sonivox/actions/workflows/android-ci.yml/badge.svg)](https://github.com/pedrolcl/sonivox/actions/workflows/android-ci.yml)

This project is a fork of the Android Open Source Project 'platform_external_sonivox', including a CMake based build system to be used not on Android, but on any other computer Operating System.
Google licensed this work originally named Sonivox EAS (Embedded Audio Synthesis) from the company Sonic Network Inc. under the terms of the Apache License 2.0.

This is a Wave Table synthesizer, not using external soundfont files by default but embedded samples. It also supports external DLS or SF2 soundfont files for better rendering quality. It is also a real time GM synthesizer. It consumes very little resources, so it may be indicated in projects for small embedded devices.
There is neither MIDI input nor Audio output facilities included in the library. You need to provide your own input/output.

You may find several projects already using this library as a git submodule:

* [Drumstick::RT](https://github.com/pedrolcl/drumstick) multiplatform realtime MIDI library. It has a Sonivox output backend.
* [Linux-SonivoxEas](https://github.com/pedrolcl/Linux-SonivoxEas) with ALSA Sequencer MIDI input and Pulseaudio output.
* [multiplatform-sonivoxeas](https://github.com/pedrolcl/multiplatform-sonivoxeas) with Drumstick::RT MIDI input and Qt Multimedia audio output.

Projects using it as an optional dependency:

* [ScummVM](https://github.com/scummvm/scummvm)

## Build options

The build system has the following options:

* `USE_44KHZ`: Renders 44100 Hz audio (ON by default). If set to OFF will output 22050 Hz audio.
* `USE_16BITS_SAMPLES`: Uses 16 bits samples (instead of 8 bit). ON by default. The rendered audio uses always 16 bits.
* `BUILD_SHARED_LIBS`: to control the generation and install of both the static or shared libraries from the sources. (ON by default).
* `BUILD_TESTING`: ON by default, to control if the unit tests are built, which require Google Test.
* `BUILD_APPLICATION`: ON by default, to build and install the CLI program. ON by default.
* `NEW_HOST_WRAPPER`: Uses the new CRT-based host wrapper for faster file loading. ON by default.
* `SF2_SUPPORT`: Enable SF2 support and float DCF. ON by default.
* `ZLIB_SUPPORT`: Enable XMF ZLIB Unpacker support. ON by default.
* `BUILD_MANPAGE`: Build the manpage of the CLI program. OFF by default.

The synthesizer types options (at least one must be enabled):

* `EAS_WT_SYNTH`: Enable WaveTable Synth. ON by default.
* `EAS_FM_SYNTH`: Enable FM Synth. OFF by default.
* `EAS_HYBRID_SYNTH`: Enable Hybrid Synth. OFF by default. Requires both `USE_44KHZ` and `USE_16BITS_SAMPLES` to be OFF.

* `MAX_VOICES`: Maximum number of voices. 64 by default.

See also the [CMake documentation](https://cmake.org/cmake/help/latest/index.html) for common build options.

## Differences with upstream

This fork currently reverts these commits:

* Full revert of [af41595](https://github.com/pedrolcl/platform_external_sonivox/commit/af41595537b044618234fe7dd9ebfcc652de1576) (Remove unused code from midi engine)
* Full revert of [34ba480](https://github.com/pedrolcl/platform_external_sonivox/commit/34ba4804f643549b8ac74e5f56bfe64db3234447) (Remove unused code)
* Partial revert of [2fa59c8](https://github.com/pedrolcl/platform_external_sonivox/commit/2fa59c8c6851b453271f33f254c7549fa79d07fb) (Build separate sonivox libs with and without jet...)

All the sources from the Android repository are kept in place, but some are not built and included in the compiled products. A few headers, mostly empty, are included in the 'fakes' subdirectory to allow compilation outside Android.

## Using the library

You may find and use the installed libraries with `pkg-config` or the `find_package()` CMake command. The library API is documented in the 'docs' directory contents.

The 'example' directory contains a simple command line utility to render standard MIDI files into raw PCM audio streams. This utility may be compiled when building the library and also installed. You may find a standalone project containing this program here: https://github.com/pedrolcl/sonivox-example

You can use the program to listen MIDI files or to create audio files (like MP3 or WAV). These are the available command line options:

~~~
$ ./sonivoxrender -h
Usage: ./sonivoxrender [-h|--help] [-v|--version] [-d|--dls soundfont] [-r|--reverb 0..4] [-w|--wet 0..32767] [-n|--dry 0..32767] [-c|--chorus 0..4] [-l|--level 0..32767] [-g|--gain 0..196] [-V|--Verbosity 0..5] [-R|--reverb-post-mix] [-C|--chorus-post-mix] [-s|--sndlib 1..3] file.mid ...
Render standard MIDI files into raw PCM audio.
Options:
        -h, --help              this help message.
        -v, --version           sonivox version.
        -d, --dls soundfont     DLS or SF2 soundfont.
        -r, --reverb n          reverb preset: 0=no, 1=large hall, 2=hall, 3=chamber, 4=room.
        -w, --wet n             reverb wet: 0..32767.
        -n, --dry n             reverb dry: 0..32767.
        -c, --chorus n          chorus preset: 0=no, 1..4=presets.
        -l, --level n           chorus level: 0..32767.
        -g, --gain n            master gain: 0..196. 100 = +0dB.
        -V, --Verbosity n       Verbosity: 0=no, 1=fatals, 2=errors, 3=warnings, 4=infos, 5=details
        -R, --reverb-post-mix   ignore CC91 reverb send.
        -C, --chorus-post-mix   ignore CC93 chorus send.
        -s, --sndlib n		sound engine library: 1=wt, 2=fm, 3=hybrid.
~~~

The following examples assume the default option USE_44KHZ=ON:

Example 1: Render a MIDI file and save the rendered audio as a raw audio file (PCM format: little endian signed 16 bits samples, 2 channels, sample rate = 44100 Hz)

    $ sonivoxrender ants.mid > ants.pcm

Example 2: pipe the rendered audio thru the Linux ALSA 'aplay' utility:

    $ sonivoxrender ants.mid | aplay -c 2 -f S16_LE -r 44100

equivalent to:

    $ sonivoxrender ants.mid | aplay -f cd

Example 3: pipe the rendered audio thru the ['lame'](https://lame.sourceforge.io) utility creating a MP3 file:

    $ sonivoxrender ants.mid | lame -r -s 44100 - ants.mp3
    
Example 4: pipe the rendered audio thru the ['sox'](https://sourceforge.net/projects/sox/) utility creating a WAV file:

    $ sonivoxrender ants.mid | sox -t s16 -c 2 -r 44100 - ants.wav

Example 5: pipe the rendered audio thru the PulseAudio's 'pacat' utility:

    $ sonivoxrender ants.mid | pacat

Example 6: pipe the rendered audio thru the PipeWire's 'pw-play' utility:

    $ sonivoxrender ants.mid | pw-play --rate 44100 -

Example 7: pipe the rendered audio thru the [FFmpeg](https://ffmpeg.org/)'s 'ffplay' utility:

    $ sonivoxrender ants.mid | ffplay -i - -f s16le -ar 44.1k -ac 2 -nodisp -autoexit -loglevel quiet

This has the advantage of being multiplatform. Depending on the FFmpeg installed version, you may need instead:

    $ sonivoxrender ants.mid | ffplay -i - -f s16le -ar 44.1k -ch_layout stereo -nodisp -autoexit -loglevel quiet

Example 8: pipe the rendered audio thru the ['mpv' media player](https://mpv.io/):

    $ sonivoxrender ants.mid | mpv --demuxer=rawaudio -demuxer-rawaudio-format=s16le --demuxer-rawaudio-rate=44100 --demuxer-rawaudio-channels=2 --no-video -

Besides being multiplatform, this supports progress view and better navigation (backed by in-memory cache).

You may replace "ants.mid" by another MIDI or XMF file, like "test/res/testmxmf.mxmf"

## Unit tests

The Android unit tests have been integrated in the CMake build system, with little modifications. A requirement is GoogleTest, either installed system wide or it will be downloaded from the git repository. 

It is **strongly** recommended that you run the test suite after changing some code or before trying the library on a new platform/compiler. Some compiler versions are known to output crashing products. Running the unit tests is a practical way to quickly detect this problem.

To run the tests, you may use this command:

    $ cmake --build <build_directory> --target test
        
or simply:

    $ ctest

There are two environment variables that you may set before running the tests (mandatory for the Qt Creator integrated test runner).

    TEMP		< path to a temporary location with write permission for the output file >
    TEST_RESOURCES	< path to the location of the input MIDI files: source_directory/test/res/ for instance >

## License

Copyright (c) 2022-2025 Pedro López-Cabanillas and others.

Copyright (c) 2008-2024, The Android Open Source Project.

Copyright (c) 2004-2006 Sonic Network Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
