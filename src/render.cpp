#define BELA_DISABLE_CPU_TIME
#define BELA_DONT_INCLUDE_UTILITIES
#include <iostream>
#include <array>
#include <iterator>
#include <string>

#include <Bela.h>
#include <bela_hw_settings.h>
#include <libraries/AudioFile/AudioFile.h>

#include "qsdk/RTPacket.h"
#include "qsdk/RTProtocol.h"

// user/experiment configuration
#include "utils/config.h"

// global variables
#include "utils/globals.h"

#include "utils/qtm.h"
#include "utils/sound.h"
#include "utils/space.h"

// #include "utils/latency_check.h" // include this file to do a latency check

#include "utils/experiment.h"

// bela setup task
bool setup(BelaContext *context, void *userData) {
  // create fill buffer function auxillary task
  if ((gFillBufferTask =
           Bela_createAuxiliaryTask(&fillBuffer, 90, "fill-buffer")) == 0)
    return false;

  if ((gRunExperimentTask =
           Bela_createAuxiliaryTask(&runExperiment, 90, "run-experiment")) == 0)
    return false;

  // initialize some default variable values.
  // gInverseSampleRate[0] = 1.0 / context->audioSampleRate;
  // gInverseSampleRate[1] = gInverseSampleRate[0];

  printf("Starting project. SR: %9.3f, ChOut: %d\n", context->audioSampleRate,
         context->audioOutChannels);

  
  rtProtocol = new CRTProtocol();
  // Connect to the QTM application
  if (!rtProtocol->Connect(serverAddr, basePort, &nPort, majorVersion, minorVersion,
                          bigEndian)) {
  	printf("\nFailed to connect to QTM RT Server. %s\n\n", rtProtocol->GetErrorString());
                system("pause");
    return false;
  }
  gConnected = true;
  printf("Connected to QTM...");

  // allocate char to stor version
  char qtmVer[64];
  // request version from QTM
  rtProtocol->GetQTMVersion(qtmVer, sizeof(qtmVer));
  // print the version
  printf("%s\n", qtmVer);

  // taking control of QTM
  if (!rtProtocol->TakeControl()) {
    printf("Failed to take control of QTM RT Server. %s\n",
           rtProtocol->GetErrorString());
    return false;
  }
  printf("Took control of QTM.\n");  

  // listen for cape button
  printf("Opening connection to Bela button...");
  gBelaCapeButton.open(kBelaCapeButtonPin, Gpio::INPUT, false);
  printf("Done\n");

#ifdef CHECK_CMD_LATENCY
  checkLatency(rtProtocol);
#endif

  printf("\n");
  // these are (and should be) small enough to load into memory.
  gUndertoneSampleData = AudioFileUtilities::loadMono(gUndertoneFile);
  gOvertoneSampleData = AudioFileUtilities::loadMono(gOvertoneFile);

  Bela_scheduleAuxiliaryTask(gRunExperimentTask);
  return true;
}

// bela main render loop function
void render(BelaContext *context, void *userData) {
  // check for button press
  if (gWaitingForButtonPress) {
    // read == false when button is pressed !?!
  	if (!gBelaCapeButton.read()) {
      gBelaButtonPressed = true;
  	}
  }
  // this is how many audio frames are rendered per loop
  for (unsigned int n = 0; n < context->audioFrames; n++) {
    gCurrentTrialDuration++;
    // if silent mode is set or the current tone is the "pause" tone
    if (gSilence) {
      // if this is between trials, or in the no sonification condition
      // just output silence.
      audioWrite(context, n, 0, 0);
      audioWrite(context, n, 1, 0);
      continue;
    } else if (startTonePlaying || endTonePlaying) {
      // if we're playing a start or end tone, we need to play a sine tone instead of sonification
      gOut = sin_freq(gCurrentTonePhase, gCurrentToneFreq, gCurrentToneInvSampleRate);
      audioWrite(context, n, 0, gOut);
      audioWrite(context, n, 1, gOut);
      continue;
    }

    gAmpMod = amp_fade_linear(gAmpModPtr, gAmpModBaseRate, gAmpModNumSamplesIO, gAmpModDepth);
    if (gAmpMod > 0.0f) {
      ++gAmpModPtr;
      if(gAmpModPtr >= gAmpModBaseRate) {
        gAmpModPtr = 0;
      }
    }
    
    if (gCurrentConditionIdx == Condition::TASK_SONIFICATION) {
      undertone_sr = pos_to_freq(gPos3D[1][0][gTrackAxis], gTrackStart, gTrackEnd, gUndertoneFreqMin, gUndertoneFreqMax);
      overtone_sr = pos_to_freq(gPos3D[1][1][gTrackAxis], gTrackStart, gTrackEnd, gOvertoneFreqMin, gOvertoneFreqMax);
      gOut = (
        warp_read_sample(gUndertoneSampleData, gReadPtrUndertone, undertone_sr / gUndertoneFreqMin, gSampleLength) +
        warp_read_sample(gOvertoneSampleData, gReadPtrOvertone, overtone_sr / gOvertoneFreqMin, gSampleLength)
        ) * 0.5f * gAmpMod;
      audioWrite(context, n, 0, gOut);
      audioWrite(context, n, 1, gOut);
    } else {
      undertone_srs = sync_to_freq(gPos3D[1][0][gTrackAxis], gPos3D[1][1][gTrackAxis], gTrackStart, gTrackEnd, gUndertoneFreqMin, gUndertoneFreqMax);
      overtone_amp = sync_to_amp(gPos3D[1][0][gTrackAxis], gPos3D[1][1][gTrackAxis], gTrackStart, gTrackEnd, 0.15f);

      gOut = (
        warp_read_sample(gUndertoneSampleData, gReadPtrUndertone, undertone_srs[0] / gUndertoneFreqMin, gSampleLength) +
        warp_read_sample(gOvertoneSampleData, gReadPtrOvertone, gFreqCenter / gOvertoneFreqMin, gSampleLength, !gSyncUseTwoChannels) * overtone_amp
      ) * 0.5f * gAmpMod;

      audioWrite(context, n, 0, gOut);

      if (gSyncUseTwoChannels) {
        gOut = (
          warp_read_sample(gUndertoneSampleData, gReadPtrUndertone2, undertone_srs[1] / gUndertoneFreqMin, gSampleLength) +
          warp_read_sample(gOvertoneSampleData, gReadPtrOvertone, gFreqCenter / gOvertoneFreqMin, gSampleLength) * overtone_amp
        ) * 0.5f * gAmpMod;
      }

      audioWrite(context, n, 1, gOut);
      
    }
    // }
  }
  Bela_scheduleAuxiliaryTask(gRunExperimentTask);
  
}

// bela cleanup function
void cleanup(BelaContext *context, void *userData) {
  // disconnect nicely from QTM
  if (gConnected) {
    gConnected = false;
    rtProtocol->Disconnect();
  }
}
