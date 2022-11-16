#ifndef GLOBALS_UTILS_H
#define GLOBALS_UTILS_H

#include <array>
#include <string>

#include <Bela.h>
#include <Gpio.h>

#include "../qsdk/RTPacket.h"
#include "../qsdk/RTProtocol.h"

#include "./config.h"

/************************************************/
/*            NON-USER VARIABLES                */
/************************************************/

// you probably don't need to change anything below this line.

// if the current trial is the task trial specified in gTaskConditionIndex
bool gUseTaskBasedSonification = false;
// thow many trials have been completed
unsigned int gCurrentConditionIdx = 0;
// how many times the current trial has run
unsigned int gCurrentTrialRep = 0;
// the current trials duration in samples
unsigned int gCurrentTrialDuration = 0;
// is a trial currently running?
bool gTrialRunning = false;
// is the current trial done
bool gTrialDone = false;
// is the current condition done
bool gConditionDone = false;
// has the experiment started?
bool gExperimentStarted = false;
// has the experiment finished?
bool gExperimentFinished = false;

Gpio gBelaCapeButton;

bool gWaitingForButtonPress = false;
bool gBelaButtonPressed = false;

/************************************************/
/*            SPATIAL VARIABLES                 */
/************************************************/

// record of maximum distance travelled, for debugging.
std::array<float, NUM_SUBJECTS> gMaxStep{};

// array of size n_subjects x n_history x 3 (x, y, z)
std::array<std::array<std::array<float, NUM_COORDS>, NUM_SUBJECTS>, NUM_SAMPLES>
    gPos3D{};

// keep track of last step distance for each subject.
std::array<float, NUM_SUBJECTS> gStepDistance{};

// frequency
std::array<float, NUM_SUBJECTS> gFreq{};


/************************************************/
/*                QTM VARIABLES                 */
/************************************************/

// record of last rendered QTM frame.
unsigned int gLastFrame = 0;

// QTM protocol
CRTProtocol* rtProtocol = NULL;

// QTM communication packet
CRTPacket* rtPacket = NULL;

// QTM packet type (we want Data Packets (CRTPacket::PacketData).
CRTPacket::EPacketType packetType;

// Connect to host (adjust as neccessary)
const char serverAddr[] = "192.168.6.1";
// Default port for QTM is 22222
const unsigned short basePort = 22222;
// Protocol version, 1.23 is the latest
const int majorVersion = 1;
const int minorVersion = 23;
// Leave as false
const bool bigEndian = false;
// server port
unsigned short nPort = 0;

// are we connected to QTM?
bool gConnected = false;

// are we currently streaming from QTM?
bool gStreaming = false;

// IDs of corresponding markers will be stored here.
std::array<int, NUM_SUBJECTS> gSubjMarker{};

/************************************************/
/*              AUDIO VARIABLES                 */
/************************************************/

// while this is true, no sound is generated
bool gSilence = true;

// output sample rate
const float gSampleRate = 44100.0f;

// the duration of trials for each condition in samples
const std::array<float, NUM_TRIALS> gTrialDurationsSamples = {{
  gTrialDurationsSec[0] * gSampleRate,
  gTrialDurationsSec[1] * gSampleRate,
  gTrialDurationsSec[2] * gSampleRate,
  gTrialDurationsSec[3] * gSampleRate
}};

// the duration of the break between trials in samples
const float gBreakDurationSamples = gBreakDurationSec * gSampleRate;

bool gOnBreak = false;
bool gBreakDone = false;

// start and end tone configuration

// start tone
const std::array<float, 6> gTrialStartTones = {{
  // C4, C4, C5
  261.6256f, 0.0f, 261.6256f, 0.0f, 523.2511f
}};

// how long the tones should play for
const float gTrialStartToneDuration = 1.0f * gSampleRate;

const std::array<float, 1> gTrialEndTones = {{
  // C5
  523.2511f
}};

bool startTonePlayed = false;
bool endTonePlayed = false;
bool startTonePlaying = false;
bool endTonePlaying = false;

float gCurrentToneFreq = 0.0f;
float gCurrentTonePhase = 0.0f;
const float gCurrentToneInvSampleRate = 1.0f / gSampleRate;
unsigned int gCurrentToneIdx = 0;

// how long the tones should play for
const float gTrialEndToneDuration = 2.0f * gSampleRate;

// current sample out
float gOut;

// the entire undertone file buffer
std::vector<float> gUndertoneSampleData;

// the entire overtone file buffer
std::vector<float> gOvertoneSampleData;

// read pointers for the sample output
float gReadPtrOvertone = 0.0f;
float gReadPtrUndertone = 0.0f;
float gReadPtrUndertone2 = 0.0f;

// the amplitude modulation pointer
unsigned int gAmpModPtr = 0;

// the current amplitude modulation value
float gAmpMod = 0.0f;

float undertone_sr = 0.0f;
float overtone_sr = 0.0f;
float overtone_amp = 0.0f;
std::array<float, 2> undertone_srs = {{0.0f, 0.0f}};

// define Bela aux task to avoid render slowdown.
AuxiliaryTask gFillBufferTask;
AuxiliaryTask gRunExperimentTask;



#endif
