/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "SonivoxTest"
#include <utils/Log.h>

#include <fcntl.h>
#include <fstream>

#include <libsonivox/eas.h>
#include <libsonivox/eas_report.h>
#include <libsonivox/eas_reverb.h>

#include "SonivoxTestEnvironment.h"

// number of Sonivox output buffers to aggregate into one MediaBuffer
static constexpr uint32_t kNumBuffersToCombine = 4;
static constexpr uint32_t kSeekBeyondPlayTimeOffsetMs = 10;

#ifdef _SAMPLE_RATE_44100
static constexpr uint32_t sampleRate = 44100;
#else
static constexpr uint32_t sampleRate = 22050;
#endif

static constexpr uint32_t numChannels = 2;

static SonivoxTestEnvironment *gEnv = nullptr;
#ifndef NEW_HOST_WRAPPER
static int readAt(void *, void *, int, int);
static int getSize(void *);
#endif

class SonivoxTest : public ::testing::TestWithParam<tuple</*fileName*/ string,
                                                          /*audioPlayTimeMs*/ uint32_t,
                                                          /*soundFont*/ string>>
{
public:
    SonivoxTest()
        : mFd(-1)
        , mEASDataHandle(nullptr)
        , mEASStreamHandle(nullptr)
        , mPCMBuffer(nullptr)
        , mAudioBuffer(nullptr)
        , mEASConfig(nullptr)
    {}

    ~SonivoxTest()
    {
        if (mFd >= 0)
            close(mFd);
        if (mPCMBuffer) {
            delete[] mPCMBuffer;
            mPCMBuffer = nullptr;
        }
        if (mAudioBuffer) {
            delete[] mAudioBuffer;
            mAudioBuffer = nullptr;
        }
        if (gEnv->cleanUp())
            remove(gEnv->OUTPUT_FILE);
    }

    virtual void SetUp() override
    {
        EAS_RESULT result;
        struct stat buf;
        tuple<string, uint32_t, string> params = GetParam();
        mInputMediaFile = gEnv->getRes() + get<0>(params);
        mAudioplayTimeMs = get<1>(params);
        mSoundFont = get<2>(params);
        mTotalAudioChannels = numChannels;
        mAudioSampleRate = sampleRate;

        result = EAS_Init(&mEASDataHandle);
        ASSERT_EQ(result, EAS_SUCCESS) << "Failed to initialize synthesizer library";

        ASSERT_NE(mEASDataHandle, nullptr) << "Failed to initialize EAS data handle";

        if (mSoundFont.length() > 0) {
            string soundfontpath = gEnv->getTmp() + mSoundFont;
            mFd = open(soundfontpath.c_str(), O_RDONLY | OPEN_FLAG);
            ASSERT_GE(mFd, 0) << "Failed to get the file descriptor for file: " << soundfontpath;

            int8_t err = stat(soundfontpath.c_str(), &buf);
            ASSERT_EQ(err, 0) << "Failed to get information for file: " << soundfontpath;

            mBase = 0;
            mLength = buf.st_size;
            memset(&mDLSFile, 0, sizeof(mDLSFile));

#ifdef NEW_HOST_WRAPPER
            mDLSFile.handle = fdopen(mFd, "rb");
            ASSERT_NE(mDLSFile.handle, nullptr)
                << "Failed to open " << soundfontpath << " error: " << strerror(errno);
#else
            mDLSFile.handle = this;
            mDLSFile.readAt = ::readAt;
            mDLSFile.size = ::getSize;
#endif
            result = EAS_LoadDLSCollection(mEASDataHandle, nullptr, &mDLSFile);
            ASSERT_EQ(result, EAS_SUCCESS) << "Failed to load DLS file: " << soundfontpath;
#ifdef NEW_HOST_WRAPPER
            close(mFd);
#endif
        }

        mFd = open(mInputMediaFile.c_str(), O_RDONLY | OPEN_FLAG);
        ASSERT_GE(mFd, 0) << "Failed to get the file descriptor for file: " << mInputMediaFile;

        int8_t err = stat(mInputMediaFile.c_str(), &buf);
        ASSERT_EQ(err, 0) << "Failed to get information for file: " << mInputMediaFile;

        mBase = 0;
        mLength = buf.st_size;
        memset(&mEasFile, 0, sizeof(mEasFile));

#ifdef NEW_HOST_WRAPPER
        mEasFile.handle = fdopen(mFd, "rb");
#else
        mEasFile.handle = this;
        mEasFile.readAt = ::readAt;
        mEasFile.size = ::getSize;
#endif

        result = EAS_OpenFile(mEASDataHandle, &mEasFile, &mEASStreamHandle);
        ASSERT_EQ(result, EAS_SUCCESS) << "Failed to open file: " << mInputMediaFile;

        ASSERT_NE(mEASStreamHandle, nullptr) << "Failed to initialize EAS stream handle";

        result = EAS_Prepare(mEASDataHandle, mEASStreamHandle);
        ASSERT_EQ(result, EAS_SUCCESS) << "Failed to prepare EAS data and stream handles";

        EAS_I32 playTimeMs;
        result = EAS_ParseMetaData(mEASDataHandle, mEASStreamHandle, &playTimeMs);
        ASSERT_EQ(result, EAS_SUCCESS) << "Failed to parse meta data";

        ASSERT_EQ(playTimeMs, mAudioplayTimeMs)
            << "Invalid audio play time found for file: " << mInputMediaFile;

        EAS_I32 locationMs = -1;
        /* EAS_ParseMetaData resets the parser to the starting of file */
        result = EAS_GetLocation(mEASDataHandle, mEASStreamHandle, &locationMs);
        ASSERT_EQ(result, EAS_SUCCESS) << "Failed to get the location after parsing meta data";

        ASSERT_EQ(locationMs, 0) << "Expected position: 0, found: " << locationMs;

        mEASConfig = EAS_Config();
        ASSERT_NE(mEASConfig, nullptr) << "Failed to configure the library";

        ASSERT_GT(mEASConfig->mixBufferSize, 0) << "Mix buffer size must be greater than 0";

        ASSERT_GT(mEASConfig->numChannels, 0) << "Number of channels must be greater than 0";

        mPCMBufferSize = sizeof(EAS_PCM) * mEASConfig->mixBufferSize * mEASConfig->numChannels
                         * kNumBuffersToCombine;

        mPCMBuffer = new (std::nothrow) EAS_PCM[mPCMBufferSize];
        ASSERT_NE(mPCMBuffer, nullptr) << "Failed to allocate a memory of size: " << mPCMBufferSize;

        mAudioBuffer = new (std::nothrow)
            EAS_PCM[mEASConfig->mixBufferSize * mEASConfig->numChannels];
        ASSERT_NE(mAudioBuffer, nullptr) << "Failed to allocate a memory of size: "
                                         << mEASConfig->mixBufferSize * mEASConfig->numChannels;
    }

      virtual void TearDown() override
      {
          EAS_RESULT result;
          if (mEASDataHandle) {
              if (mEASStreamHandle) {
                  result = EAS_CloseFile(mEASDataHandle, mEASStreamHandle);
                  ASSERT_EQ(result, EAS_SUCCESS) << "Failed to close audio file/stream";
              }
              result = EAS_Shutdown(mEASDataHandle);
              ASSERT_EQ(result, EAS_SUCCESS)
                  << "Failed to deallocate the resources for synthesizer library";
          }
      }

    bool seekToLocation(EAS_I32);
    bool renderAudio();
#ifndef NEW_HOST_WRAPPER
    int readAt(void *buf, int offset, int size);
    int getSize();
#endif

    string mInputMediaFile;
    string mSoundFont;
    uint32_t mAudioplayTimeMs;
    uint32_t mTotalAudioChannels;
    uint32_t mAudioSampleRate;
    off64_t mBase;
    int64_t mLength;
    int mFd;

    EAS_DATA_HANDLE mEASDataHandle;
    EAS_HANDLE mEASStreamHandle;
    EAS_FILE mEasFile;
    EAS_FILE mDLSFile;
    EAS_PCM *mPCMBuffer;
    EAS_PCM *mAudioBuffer;
    EAS_I32 mPCMBufferSize;
    const S_EAS_LIB_CONFIG *mEASConfig;
};

#ifndef NEW_HOST_WRAPPER
static int readAt(void *handle, void *buffer, int offset, int size) {
    return ((SonivoxTest *)handle)->readAt(buffer, offset, size);
}

static int getSize(void *handle) {
    return ((SonivoxTest *)handle)->getSize();
}

int SonivoxTest::readAt(void *buffer, int offset, int size) {
    if (offset > mLength) offset = mLength;
    lseek(mFd, mBase + offset, SEEK_SET);
    if (offset + size > mLength) {
        size = mLength - offset;
    }

    return read(mFd, buffer, size);
}

int SonivoxTest::getSize() {
    return mLength;
}
#endif

bool SonivoxTest::seekToLocation(EAS_I32 locationExpectedMs) {
    EAS_RESULT result = EAS_Locate(mEASDataHandle, mEASStreamHandle, locationExpectedMs, false);
    if (result != EAS_SUCCESS) return false;

    // position in milliseconds
    EAS_I32 locationReceivedMs;
    result = EAS_GetLocation(mEASDataHandle, mEASStreamHandle, &locationReceivedMs);
    if (result != EAS_SUCCESS) return false;

    if (locationReceivedMs != locationExpectedMs) return false;

    return true;
}

bool SonivoxTest::renderAudio() {
    EAS_I32 count = -1;
    EAS_PCM *pcm = mAudioBuffer;

    EAS_RESULT result = EAS_Render(mEASDataHandle, pcm, mEASConfig->mixBufferSize, &count);
    if (result != EAS_SUCCESS) {
        ALOGE("Failed to render audio");
        return false;
    }
    if (count != mEASConfig->mixBufferSize) {
        ALOGE("%ld of %ld bytes rendered", count, mEASConfig->mixBufferSize);
        return false;
    }

    return true;
}

TEST_P(SonivoxTest, DecodeTest) {
    EAS_I32 totalChannels = mEASConfig->numChannels;
    ASSERT_EQ(totalChannels, mTotalAudioChannels)
        << "Expected: " << mTotalAudioChannels << " channels, Found: " << totalChannels;

    EAS_I32 sampleRate = mEASConfig->sampleRate;
    ASSERT_EQ(sampleRate, mAudioSampleRate)
        << "Expected: " << mAudioSampleRate << " sample rate, Found: " << sampleRate;

    // TODO(b/158231824): Check and verify the output with other parameters present at eas_reverb.h
    // TODO: Check and verify the output with other parameters present at eas_chorus.h
    // TODO: Check and verify the output with CC override

    EAS_I32 count;
    EAS_STATE state;
    EAS_RESULT result = EAS_SUCCESS;

    FILE *filePtr = fopen(gEnv->OUTPUT_FILE, "wb");
    ASSERT_NE(filePtr, nullptr) << "Failed to open file: " << gEnv->OUTPUT_FILE;

    while (1) {
        EAS_PCM *pcm = mPCMBuffer;
        int32_t numBytesOutput = 0;
        result = EAS_State(mEASDataHandle, mEASStreamHandle, &state);
        ASSERT_EQ(result, EAS_SUCCESS) << "Failed to get EAS State";

        ASSERT_NE(state, EAS_STATE_ERROR) << "Error state found";

        /* is playback complete */
        if (state == EAS_STATE_STOPPED) {
            break;
        }

        EAS_I32 locationMs;
        result = EAS_GetLocation(mEASDataHandle, mEASStreamHandle, &locationMs);
        ASSERT_EQ(result, EAS_SUCCESS) << "Failed to get the current location in ms";

        if (locationMs >= mAudioplayTimeMs) {
            ASSERT_NE(state, EAS_STATE_STOPPED)
                    << "Invalid state reached when rendering is complete";

            break;
        }

        for (uint32_t i = 0; i < kNumBuffersToCombine; i++) {
            result = EAS_Render(mEASDataHandle, pcm, mEASConfig->mixBufferSize, &count);
            ASSERT_EQ(result, EAS_SUCCESS) << "Failed to render the audio data";

            pcm += count * mEASConfig->numChannels;
            numBytesOutput += count * mEASConfig->numChannels * sizeof(EAS_PCM);
        }
        int32_t numBytes = fwrite(mPCMBuffer, 1, numBytesOutput, filePtr);
        ASSERT_EQ(numBytes, numBytesOutput)
                << "Wrote " << numBytes << " of " << numBytesOutput << " to file: " << gEnv->OUTPUT_FILE;
    }
    fclose(filePtr);
}

TEST_P(SonivoxTest, SeekTest) {
    bool status = seekToLocation(0);
    ASSERT_TRUE(status) << "Seek test failed for location(ms): 0";

    status = seekToLocation(mAudioplayTimeMs / 2);
    ASSERT_TRUE(status) << "Seek test failed for location(ms): " << mAudioplayTimeMs / 2;

    status = seekToLocation(mAudioplayTimeMs);
    ASSERT_TRUE(status) << "Seek test failed for location(ms): " << mAudioplayTimeMs;

    status = seekToLocation(mAudioplayTimeMs + kSeekBeyondPlayTimeOffsetMs);
    ASSERT_FALSE(status) << "Invalid seek position: "
                         << mAudioplayTimeMs + kSeekBeyondPlayTimeOffsetMs;
}

TEST_P(SonivoxTest, DecodePauseResumeTest) {

    EAS_I32 seekPosition = mAudioplayTimeMs / 2;
    // go to middle of the audio
    EAS_RESULT result = EAS_Locate(mEASDataHandle, mEASStreamHandle, seekPosition, false);
    ASSERT_EQ(result, EAS_SUCCESS) << "Failed to locate to location(ms): " << seekPosition;

    bool status = renderAudio();
    ASSERT_TRUE(status) << "Failed to render audio";

    result = EAS_Pause(mEASDataHandle, mEASStreamHandle);
    ASSERT_EQ(result, EAS_SUCCESS) << "Failed to pause";

    // will render previous audio again, no change in audio position
    status = renderAudio();
    ASSERT_TRUE(status) << "should not move audio position, since we're paused";

    // current position in milliseconds
    EAS_I32 currentPosMs = -1;
    result = EAS_GetLocation(mEASDataHandle, mEASStreamHandle, &currentPosMs);
    ASSERT_EQ(result, EAS_SUCCESS) << "Failed to get current location";

    ASSERT_EQ(currentPosMs, seekPosition) << "Must not move the audio position after pause";

    EAS_STATE state;
    result = EAS_State(mEASDataHandle, mEASStreamHandle, &state);
    ASSERT_EQ(result, EAS_SUCCESS) << "Failed to get EAS state";

    ASSERT_TRUE(state == EAS_STATE_PAUSING || state == EAS_STATE_PAUSED) << "Invalid state reached when paused";

    result = EAS_Resume(mEASDataHandle, mEASStreamHandle);
    ASSERT_EQ(result, EAS_SUCCESS) << "Failed to resume";

    status = renderAudio();
    ASSERT_TRUE(status) << "Failed to render audio after resume";

    currentPosMs = -1;
    result = EAS_GetLocation(mEASDataHandle, mEASStreamHandle, &currentPosMs);
    ASSERT_EQ(result, EAS_SUCCESS) << "Failed to get current location";

    ASSERT_GT(currentPosMs, seekPosition) << "Invalid position after resuming";

    result = EAS_State(mEASDataHandle, mEASStreamHandle, &state);
    ASSERT_EQ(result, EAS_SUCCESS) << "Failed to get EAS state";

    ASSERT_EQ(state, EAS_STATE_PLAY) << "Invalid state reached when resumed";
}

INSTANTIATE_TEST_SUITE_P(SonivoxTestAll,
                         SonivoxTest,
                         ::testing::Values(make_tuple("midi_a.mid", 2000, ""),
                                           make_tuple("midi8sec.mid", 8002, ""),
                                           make_tuple("midi_cs.mid", 2000, ""),
                                           make_tuple("midi_gs.mid", 2000, ""),
                                           make_tuple("ants.mid", 17233, ""),
                                           make_tuple("testmxmf.mxmf", 29095, ""),
                                           make_tuple("midi_a.mid", 2000, "soundfont.dls"),
                                           make_tuple("midi8sec.mid", 8002, "soundfont.dls"),
                                           make_tuple("midi_cs.mid", 2000, "soundfont.dls"),
                                           make_tuple("midi_gs.mid", 2000, "soundfont.dls"),
                                           make_tuple("ants.mid", 17233, "soundfont.dls"),
                                           make_tuple("ants.mid", 17233, "soundfont.dls")));

int main(int argc, char **argv) {
    gEnv = new SonivoxTestEnvironment();
    ::testing::AddGlobalTestEnvironment(gEnv);
    ::testing::InitGoogleTest(&argc, argv);
    int status = gEnv->initFromOptions(argc, argv);
    if (status == 0) {
        // Initalize EAS logging
        EAS_SetDebugFile(stderr, 1);
        EAS_SetDebugLevel(_EAS_SEVERITY_DETAIL); // print all messages
        status = RUN_ALL_TESTS();
        ALOGV("Test result = %d\n", status);
        EAS_Report(_EAS_SEVERITY_INFO, "Test result = %d\n", status);
    }
    return status;
}
