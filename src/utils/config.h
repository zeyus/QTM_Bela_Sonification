#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <array>
#include <string>

// how much history to keep
#define NUM_SAMPLES 2
// how many signals to process (at the moment this is required to be 2)
// because we match them to out channels...for now.
#define NUM_SUBJECTS 2
// just x, y, z. but maybe we want to track other things?
#define NUM_COORDS 3

#define NUM_TRIALS 4
#define NUM_CONDITIONS 3

/************************************************/
/*            EXPERIMENT VARIABLES              */
/************************************************/


/* TRIAL CONFIGURATION */

// condition indices (don't change order but you can change values in [0, NUM_CONDITIONS-1])
enum Condition {
  NO_SONIFICATION = 0,
  TASK_SONIFICATION = 1,
  SYNC_SONIFICATION = 2
};

// condition labels for printing messages to console
const char* gConditionLabels[NUM_CONDITIONS] = {
    "no_sonification",
    "task_sonification",
    "sync_sonification"
  };

// should the trial start and end sounds play?
const bool gPlayTrialSounds = true;

// how many trials per condition
const std::array<unsigned int, 3> gTrialCounts = {{
  NUM_TRIALS, NUM_TRIALS, NUM_TRIALS
}};

// how long per trial for all conditions
const std::array<float, NUM_TRIALS> gTrialDurationsSec = {{
  30.0f, 120.0f, 120.0f, 120.0f
}};

// the duration of trials for each condition in seconds
const float gBreakDurationSec = 15.0f;

/* LABELS */

// define event label char*s
enum class Labels : char {
    TRIAL_START = 's',
    TRIAL_END = 'e',
    EXPERIMENT_START = 'S',
    EXPERIMENT_END = 'E',
};

// labels for conditions
enum class ConditionLabels : char {
  NO_SONIFICATION = 'n',
  TASK_SONIFICATION = 't',
  SYNC_SONIFICATION = 'y',
};

// the order of the trials (based on the condition labels)
const std::array<unsigned int, 3> gConditionOrder = {{
  Condition::NO_SONIFICATION,
  Condition::TASK_SONIFICATION,
  Condition::SYNC_SONIFICATION
}};


/* MOCAP */

const unsigned int gPacketTimeoutMicroSec = 100000; // 100ms
// track axis
const unsigned int gTrackAxis = 1; // x: 0, y: 1, z: 2

// track start and end points (on corresponding axis)
const float gTrackStart = -250.0;
const float gTrackEnd = 900.0;

// use UDP for QTM connection.
// UDP has less overhead so try to use that if no problems.
const bool gStreamUDP = true;

// Should bela tell QTM to start and stop capture?
const bool gControlQTMCapture = false;

// names of tracked markers in QTM.
const std::array<std::string, NUM_SUBJECTS> gSubjMarkerLabels{{"CAR_W", "CAR_D"}};

/* SONIFICATION */

// file for the lower tone
const std::string gUndertoneFile = "./res/simple_As3.wav";

// minimum frequency for undertone playback.
const float gUndertoneFreqMin = 232.819;
// maximum frequencey for undertone playback.
const float gUndertoneFreqMax = 369.577;

// file for the higher tone
const std::string gOvertoneFile = "./res/simple_f4.wav";

// minimum frequency for overtone playback.
const float gOvertoneFreqMin = 349.23;
// maximum frequencey for overtone playback.
const float gOvertoneFreqMax = 554.365;
// the center overtone frequency
const float gFreqCenter = 440.0;


// length of wave files in samples
const unsigned int gSampleLength = 113145;

// Should sync condition be different for left and right channels?
const bool gSyncUseTwoChannels = false;

/* MODULATION */

// Amplitude modulation
// Frequency in samples for modulation
const unsigned int gAmpModBaseRate = gSampleLength / 15;

// Length of fade in and fade out in samples
const unsigned int gAmpModNumSamplesIO = 2515;

// Depth of modulation from 0.0 to 1.0 (0.0 = no modulation)
const float gAmpModDepth = 0.0f;




#endif