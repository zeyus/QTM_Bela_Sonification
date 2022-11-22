#include <iostream>
#include <array>
#include <iterator>
#include <string>

#include <Bela.h>
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


bool prepare_sonification_condition() {
  // make sure there's 3D data
  bool dataAvailable;
  if (!rtProtocol->Read3DSettings(dataAvailable)) return false;

  // Start streaming from QTM
  if (gStreamUDP) {
    if (!rtProtocol->StreamFrames(CRTProtocol::RateAllFrames, 0, nPort, NULL,
                                 CRTProtocol::cComponent3d)) {
      printf("Error streaming from QTM\n");
      return false;
    }
  } else {
    // Start the 3D data stream.
    if (!rtProtocol->StreamFrames(CRTProtocol::RateAllFrames, 0, 0, NULL,
                                 CRTProtocol::cComponent3d)) {
      printf("Error streaming from QTM\n");
      return false;
    }
  }
  printf("Started streaming 3D data...\n");
  gStreaming = true;
  if (!reindexMarkers(rtProtocol)) return false;

  // start getting the 3D data.
  // Bela_scheduleAuxiliaryTask(gFillBufferTask);
  gSilence = false;
  // Bela_deleteAllAuxiliaryTasks();
  return true;
}

bool end_sonification_condition() {
  // Stop streaming from QTM
  if (!rtProtocol->StreamFramesStop()) {
    printf("Error stopping streaming from QTM\n");
    return false;
  }
  printf("Stopped streaming 3D data\n");
  gStreaming = false;
  gSilence = true;
  return true;
}

void resetDuration() {
  gCurrentTrialDuration = 0;
}

void resetTrial() {
  resetDuration();
  gTrialRunning = false;
  startTonePlayed = false;
  endTonePlayed = false;
  gTrialDone = false;
  gReadPtrOvertone = 0.0f;
  gReadPtrUndertone = 0.0f;
  gReadPtrUndertone2 = 0.0f;
  gAmpModPtr = 0;

}

void startBreak() {
  printf("Starting break\n");
  resetDuration();
  gOnBreak = true;
  gBreakDone = false;
}

void endBreak() {
  printf("Ending break\n");
  gOnBreak = false;
  gBreakDone = true;
}

void resetCondition() {
  // resetTrial();
  gCurrentTrialRep = 0;
}

void resetSine() {
  gCurrentTonePhase = 0.0f;
}

void startStartTone() {
  resetDuration();
  resetSine();
  gSilence = false;
  startTonePlaying = true;
  gCurrentToneIdx = 0;
  gCurrentToneFreq = gTrialStartTones[gCurrentToneIdx];
}

void endStartTone() {
  gSilence = true;
  startTonePlaying = false;
  startTonePlayed = true;
}

void startEndTone() {
  resetDuration();
  resetSine();
  gSilence = false;
  endTonePlaying = true;
  gCurrentToneIdx = 0;
  gCurrentToneFreq = gTrialEndTones[gCurrentToneIdx];
}

void endEndTone() {
  gSilence = true;
  endTonePlaying = false;
  endTonePlayed = true;
}

void startExperiment() {
  // start the experiment
  gExperimentStarted = true;
  printf("Experiment started.\n");
  // start the first trial
  gCurrentConditionIdx = 0;
  resetCondition();
  resetTrial();
  // startBreak();
  // if we're using bela to start / stop capture, do it here.
  if (gControlQTMCapture) {
    startCapture(rtProtocol);
    printf("QTM capture started.\n");
  }
  sendEventLabel(rtProtocol, Labels::EXPERIMENT_START);
}

void endExperiment() {
  // experiment is finished, place marker and plan exit
  // TODO: confirm type conversion
  printf("Experiment ended.\n");
  sendEventLabel(rtProtocol, Labels::EXPERIMENT_END);
  // if we're using bela to start / stop capture, do it here.
  if (gControlQTMCapture) {
    stopCapture(rtProtocol);
    printf("QTM capture stopped.\n");
  }
  printf("Exiting.\n");
  Bela_requestStop();
}

void startTrial() {
  sendEventLabel(rtProtocol, Labels::TRIAL_START);
  printf("Trial %d started.\n", gCurrentTrialRep);
  resetTrial();
  gTrialRunning = true;
}


void startTrialIfReady() {
  if (gOnBreak && !gBreakDone) {
    // this is a break between trials
    if (gCurrentTrialDuration >= gBreakDurationSamples) {
      endBreak();
    } else {
      return;
    }
  }

  if (!startTonePlayed && !startTonePlaying) {
    // start tone hasn't been played yet
    printf("Playing start tone.\n");
    startStartTone();
    return;
  }

  if (!startTonePlayed) {
    if (gCurrentTrialDuration >= gTrialStartToneDuration) {
      if (gCurrentToneIdx + 1 >= gTrialStartTones.size()) {
        printf("Start tone complete.\n");
        endStartTone();
      } else {
        // play the next tone
        resetDuration();
        gCurrentToneIdx++;
        gCurrentToneFreq = gTrialStartTones[gCurrentToneIdx];
        gSilence = gCurrentToneFreq == 0.0f;
        return;
      }
    } else {
      return;
    }
  }
  
  // tone and break are done, so now start the trial
    
  if (gCurrentConditionIdx == Condition::NO_SONIFICATION) {
    sendEventLabel(rtProtocol, ConditionLabels::NO_SONIFICATION);
    printf("Condition %d: NO_SONIFICATION\n", gCurrentConditionIdx);
  } else if (gCurrentConditionIdx == Condition::TASK_SONIFICATION) {
    printf("Starting sonification.\n");
    prepare_sonification_condition();
    sendEventLabel(rtProtocol, ConditionLabels::TASK_SONIFICATION);
    printf("Condition %d: TASK_SONIFICATION\n", gCurrentConditionIdx);

  } else if (gCurrentConditionIdx == Condition::SYNC_SONIFICATION) {
    printf("Starting sonification.\n");
    prepare_sonification_condition();
    sendEventLabel(rtProtocol, ConditionLabels::SYNC_SONIFICATION);
    printf("Condition %d: SYNC_SONIFICATION\n", gCurrentConditionIdx);
  }
  startTrial();
  
}

void endTrial() {
  resetTrial();
  if (gCurrentConditionIdx >= gTrialCounts.size()) {
    // we have completed all the trials, stop the experiment
    gExperimentFinished = true;
  }
  startBreak();
}

void endTrialIfReady() {
  if (gTrialDone) {
    if (!endTonePlayed && !endTonePlaying) {
      // start tone hasn't been played yet
      printf("Playing end tone.\n");
      startEndTone();
      return;
    }

    if (endTonePlaying && gCurrentTrialDuration >= gTrialEndToneDuration) {
      if (gCurrentToneIdx + 1 >= gTrialEndTones.size()) {
        printf("End tone complete.\n");
        endEndTone();
        endTrial();
        return;
      }
    
      // play the next tone
      resetDuration();
      gCurrentToneIdx++;
      gCurrentToneFreq = gTrialEndTones[gCurrentToneIdx];
      gSilence = gCurrentToneFreq == 0.0f;
      return;
    }
  } else if (gCurrentTrialDuration >= gTrialDurationsSamples[gCurrentConditionIdx]) {
    // now we have to keep running the experiment until it reaches the sample limit
    gTrialDone = true;
    printf("Trial %d ended.\n", gCurrentTrialRep);
    gCurrentTrialRep++;
    if (gCurrentConditionIdx != Condition::NO_SONIFICATION) {
      printf("Stopping sonification.\n");
      end_sonification_condition();
    }
    if (gCurrentTrialRep >= gTrialCounts[gCurrentConditionIdx]) {
      // we have completed all the reps for this trial, move on to the next one
      gCurrentConditionIdx++;
      resetCondition();
      
    }
    sendEventLabel(rtProtocol, Labels::TRIAL_END);
  }
}

// experiment runner
void runExperiment(void *) {
  if (!gExperimentStarted && !gExperimentFinished) {
    startExperiment();
  } else if (!gTrialRunning) {
    // trials will start if it's not during a break
    startTrialIfReady();
  } else if (!gExperimentFinished) {
    // trials will end if they've run for the specified duration
    endTrialIfReady();
  }
  if (gExperimentFinished) {
    endExperiment();
  }
}

// update buffer of QTM data
void fillBuffer(void *) {
  // Make sure we successfully get the data
  if (!gConnected || !gTrialRunning) return;
  if (!get3DPacket(rtProtocol, rtPacket, packetType)) return;

  // this helps us when we're doing realtime playback, because it loops.
  const unsigned int uPacketFrame = rtPacket->GetFrameNumber();

  for (int i = 0; i < NUM_SUBJECTS; i++) {
    // auto &prevPos = gPos3D[1][i];
    auto &currPos = gPos3D[0][i];
    // get the position of the marker
    if (!rtPacket->Get3DMarker(gSubjMarker[i], currPos[0], currPos[1], currPos[2])) {
      // the marker failed, we can try reindexing
      printf("Marker %d failed, reindexing.\n", gSubjMarker[i]);
      if (reindexMarkers(rtProtocol)) {
        printf("Reindexing successful.\n");
        // reindexing was successful, so we can try again
        if (!rtPacket->Get3DMarker(gSubjMarker[i], currPos[0], currPos[1], currPos[2])) {
          // the marker failed again, so we'll just set the position to 0
          printf("Marker %d failed again, setting position to 0.\n", gSubjMarker[i]);
          currPos[0] = 0.0f;
          currPos[1] = 0.0f;
          currPos[2] = 0.0f;
        }
      } else {
        // reindexing failed, so we'll just set the position to 0
        printf("Reindexing failed, setting position to 0.\n");
        currPos[0] = 0.0f;
        currPos[1] = 0.0f;
        currPos[2] = 0.0f;
      }

    }
  }

  // update last processed frame
  gLastFrame = uPacketFrame;

  // swap the current and previous positions
  // this means we always have the previous position in gPos3D[1]
  // but overwrite gPos3D[0]
  std::swap(gPos3D[0], gPos3D[1]);
}

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
  if (!gSilence && gStreaming) {
    // just keep polling for 3D data.
    Bela_scheduleAuxiliaryTask(gFillBufferTask);
  }
}

// bela cleanup function
void cleanup(BelaContext *context, void *userData) {
  // disconnect nicely from QTM
  if (gConnected) {
    gConnected = false;
    rtProtocol->Disconnect();
  }
}
