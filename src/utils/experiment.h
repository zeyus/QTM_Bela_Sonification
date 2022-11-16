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

  // number of labelled markers
  const unsigned int nLabels = rtProtocol->Get3DLabeledMarkerCount();
  // printf("Found labels: \n");
  // loop through labels to find ones we are interested in.
  for (unsigned int i = 0; i < nLabels; i++) {
    const char *cLabelName = rtProtocol->Get3DLabelName(i);
    // printf("- %s\n", cLabelName);
    for (unsigned int j = 0; j < NUM_SUBJECTS; j++) {
      // if the label is one of our specified markers, keep the ID.
      if (cLabelName == gSubjMarkerLabels[j]) {
        printf("Found marker: %s id: %d\n", cLabelName, i);
        gSubjMarker[j] = i;
      }
    }
  }
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

void waitForButton() {
  printf("\n\nCondition ended. Please press the Bela button to continue...\n\n");
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
      printf("Stopping sonification.\n");
      end_sonification_condition();
    }
    if (gCurrentTrialRep >= gTrialCounts[gCurrentConditionIdx]) {
      // we have completed all the reps for this trial, move on to the next one
      gCurrentConditionIdx++;
      endCondition();
    }
    sendEventLabel(rtProtocol, Labels::TRIAL_END);
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
  }
}

// update buffer of QTM data
void fillBuffer(void *) {
  // Make sure we successfully get the data
  if (!gStreaming) return;
  if (!get3DPacket(rtProtocol, rtPacket, packetType)) return;

  // this helps us when we're doing realtime playback, because it loops.
  const unsigned int uPacketFrame = rtPacket->GetFrameNumber();

  for (int i = 0; i < NUM_SUBJECTS; i++) {
    // auto &prevPos = gPos3D[1][i];
    auto &currPos = gPos3D[0][i];
    // get the position of the marker
    rtPacket->Get3DMarker(gSubjMarker[i], currPos[0], currPos[1], currPos[2]);
  }

  // update last processed frame
  gLastFrame = uPacketFrame;

  if (!gInitialized) {
    gInitialized = true;
  }
  // swap the current and previous positions
  // this means we always have the previous position in gPos3D[1]
  // but overwrite gPos3D[0]
  std::swap(gPos3D[0], gPos3D[1]);
}

#endif
