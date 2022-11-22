#ifndef GLOBALS_EXPERIMENT_H
#define GLOBALS_EXPERIMENT_H

#include <Bela.h>

#include "../qsdk/RTPacket.h"
#include "../qsdk/RTProtocol.h"

#include "./config.h"
#include "./globals.h"
#include "./qtm.h"
#include "./sound.h"
#include "./space.h"

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
  gSilence = true;
  
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

void waitForButton() {
  printf("\n\nPlease press the Bela button to continue...\n\n");
  gWaitingForButtonPress = true;
  gBelaButtonPressed = false;
}

void resetButton() {
  gWaitingForButtonPress = false;
  gBelaButtonPressed = false;
}

void startCondition() {
  // resetTrial();
  gCurrentTrialRep = 0;
  gConditionDone = false;
}

void endCondition() {
  gConditionDone = true;
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
  
  // print number of conditions
  printf("Number of conditions: %d\n", NUM_CONDITIONS);
  // print condition order from enum Condition
  printf("Condition order: \n");
  for (int i = 0; i < NUM_CONDITIONS; i++) {
    printf("- %s: %d \n", gConditionLabels[i], gConditionOrder[i] + 1);
  }
  
  // print the number of trials
  printf("Number of trials per condition (incl. practice): %d\n", NUM_TRIALS);
  // print the durations for each trial
  printf("Trial durations: ");
  for (int i = 0; i < NUM_TRIALS; i++) {
    printf("%d: %3.1f seconds, ", i + 1, gTrialDurationsSec[i]);
  }
  printf("\n");
  // print pause duration
  printf("Trial pause duration: %3.1f seconds\n\n", gBreakDurationSec);

  // start the first trial
  gCurrentConditionIdx = 0;
  startCondition();
  resetTrial();
  // startBreak();
  // if we're using bela to start / stop capture, do it here.
  if (gControlQTMCapture) {
    startCapture(rtProtocol);
    printf("QTM capture started.\n");
  }
  sendEventLabel(rtProtocol, Labels::EXPERIMENT_START);
  waitForButton();
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
  printf("Sending trial start label...\n");
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
    printf("Starting sonification.\n");
    prepare_sonification_condition();
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
  printf("Sending trial condition label...\n");
  if (gCurrentConditionIdx == Condition::NO_SONIFICATION) {
    sendEventLabel(rtProtocol, ConditionLabels::NO_SONIFICATION);
    printf("Condition %d: NO_SONIFICATION\n", gCurrentConditionIdx);
  } else if (gCurrentConditionIdx == Condition::TASK_SONIFICATION) {
    sendEventLabel(rtProtocol, ConditionLabels::TASK_SONIFICATION);

    // printf("Starting sonification.\n");
    // prepare_sonification_condition();
    
    printf("Condition %d: TASK_SONIFICATION\n", gCurrentConditionIdx);
    gSilence = false;

  } else if (gCurrentConditionIdx == Condition::SYNC_SONIFICATION) {
    sendEventLabel(rtProtocol, ConditionLabels::SYNC_SONIFICATION);

    // printf("Starting sonification.\n");
    // prepare_sonification_condition();

    printf("Condition %d: SYNC_SONIFICATION\n", gCurrentConditionIdx);
    gSilence = false;
  }
  startTrial();
  if (!gSilence && gStreaming) {
    Bela_scheduleAuxiliaryTask(gFillBufferTask);
  }
  
}

void endTrial() {
  resetTrial();
  if (gCurrentConditionIdx >= gTrialCounts.size()) {
    // we have completed all the trials, stop the experiment
    gExperimentFinished = true;
  }
  
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
        if (!gConditionDone) {
          startBreak();
        } else {
          waitForButton();
        }
        end_sonification_condition();
        return;
      }
    
      // play the next tone
      resetDuration();
      gCurrentToneIdx++;
      gCurrentToneFreq = gTrialEndTones[gCurrentToneIdx];
      gSilence = gCurrentToneFreq == 0.0f;
      return;
    }
  } else if (gCurrentTrialDuration >= gTrialDurationsSamples[gCurrentTrialRep]) {
    // now we have to keep running the experiment until it reaches the sample limit
    gTrialDone = true;
    printf("Trial %d ended.\n", gCurrentTrialRep);
    gCurrentTrialRep++;
    if (gCurrentConditionIdx != Condition::NO_SONIFICATION) {
      gSilence = true;
    }
    if (gCurrentTrialRep >= gTrialCounts[gCurrentConditionIdx]) {
      // we have completed all the reps for this trial, move on to the next one
      gCurrentConditionIdx++;
      endCondition();
    }
    printf("Sending trial end label...\n");
    sendEventLabel(rtProtocol, Labels::TRIAL_END);
  } else {
    if (!gSilence && gStreaming) {
      // just keep polling for 3D data.
      Bela_scheduleAuxiliaryTask(gFillBufferTask);
    }
  }
}

// experiment runner
void runExperiment(void *) {
  if (gWaitingForButtonPress) {
    if (gBelaButtonPressed) {
      printf("Button pressed, starting next condition...\n");
      gWaitingForButtonPress = false;
      startCondition();
      resetButton();
    }
    return;
  }
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
    return;
  }

  
}

// update buffer of QTM data
void fillBuffer(void *) {
  // if stream is not open, or we're silenced, don't do anything
  if (!gStreaming || gSilence) return;
  // Make sure we successfully get the data
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

#endif
