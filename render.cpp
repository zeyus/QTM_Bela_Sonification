#include <Bela.h>
#include <libraries/math_neon/math_neon.h>

#include <array>
#include <iterator>

#include "qsdk/RTPacket.h"
#include "qsdk/RTProtocol.h"

#define NUM_SAMPLES 2
#define NUM_SUBJECTS 2
#define NUM_COORDS 3

/*
 * USER CONFIGURATION
 */

// in dummy project : L_FCC (left heel)
const std::array<int, NUM_SUBJECTS> gSubjMarker{{33, 46}};

const float gFreqMin = 250.0;
const float gFreqMax = 500.0;

const float gStepDistanceMin = 0.0;
const float gStepDistanceMax = 80.0;

/*
 * GLOBAL VARIABLES
 */
// use UDP? TCP if set to false.
// UDP has less overhead so try to use that if no problems.
const bool gStreamUDP = true;

// array of size n_subjects x n_history x 3 (x, y, z)
std::array<std::array<std::array<float, NUM_COORDS>, NUM_SUBJECTS>, NUM_SAMPLES>
    gPos3D{};

// keep track of last step distance for each subject.
std::array<float, NUM_SUBJECTS> gStepDistance{};

// frequency
std::array<float, NUM_SUBJECTS> gFreq{};

// for use in render
CRTPacket *rtPacket;
CRTProtocol rtProtocol;
CRTPacket::EPacketType packetType;
bool gConnected = false;
bool gInitialized = false;

std::array<float, NUM_SUBJECTS> gPhase{};
std::array<float, NUM_SUBJECTS> gInverseSampleRate{};

AuxiliaryTask gFillBufferTask;
void fillBuffer(void *) {
  if (rtProtocol.Receive(packetType, true) != CNetwork::ResponseType::success) {
    printf("Problem reading data...\n");
    return;
  }
  if (packetType == CRTPacket::PacketData) {
    // get the current data packet
    rtPacket = rtProtocol.GetRTPacket();

    for (int i = 0; i < NUM_SUBJECTS; i++) {
      auto &prevPos = gPos3D[1][i];
      auto &currPos = gPos3D[0][i];
      // get the position of the marker
      rtPacket->Get3DMarker(gSubjMarker[i], currPos[0], currPos[1], currPos[2]);
      if (!gInitialized) {
        prevPos[0] = currPos[0];
        prevPos[1] = currPos[1];
        prevPos[2] = currPos[2];
      }

      // calculate the distance between the current and previous position
      gStepDistance[i] = sqrtf_neon(powf_neon(currPos[0] - prevPos[0], 2) +
                                    powf_neon(currPos[1] - prevPos[1], 2) +
                                    powf_neon(currPos[2] - prevPos[2], 2));

      gStepDistance[i] =
          constrain(gStepDistance[i], gStepDistanceMin, gStepDistanceMax);

      gFreq[i] = map(gStepDistance[i], gStepDistanceMin, gStepDistanceMax,
                     gFreqMin, gFreqMax);
    }
    // copy the current position to the previous position
    if (!gInitialized) {
      gInitialized = true;
    }
    // swap the current and previous positions
    // this means we always have the previous position in gPos3D[1]
    std::swap(gPos3D[0], gPos3D[1]);
  }
}

bool setup(BelaContext *context, void *userData) {
  if ((gFillBufferTask =
           Bela_createAuxiliaryTask(&fillBuffer, 90, "fill-buffer")) == 0)
    return false;

  gInverseSampleRate[0] = 1.0 / context->audioSampleRate;
  gInverseSampleRate[1] = gInverseSampleRate[0];
  
  printf("Starting project. SR: %9.3f, ChOut: %d\n", context->audioSampleRate,
         context->audioOutChannels);

  // Connect to host (adjust as neccessary)
  const char serverAddr[] = "192.168.6.1";
  // Default port for QTM is 22222
  const unsigned short basePort = 22222;
  // Protocol version, 1.23 is the latest
  const int majorVersion = 1;
  const int minorVersion = 23;
  // Leave as false
  const bool bigEndian = false;

  // Connect to the QTM application
  if (!rtProtocol.Connect(serverAddr, basePort, 0, majorVersion, minorVersion,
                          bigEndian))
    return false;

  unsigned short nPort = rtProtocol.GetUdpServerPort();
  // make sure there's 3D data
  bool dataAvailable;
  if (!rtProtocol.Read3DSettings(dataAvailable)) return false;

  // Start streaming from QTM
  if (gStreamUDP) {
    if (!rtProtocol.StreamFrames(CRTProtocol::RateAllFrames, 0, nPort, NULL,
                                 CRTProtocol::cComponent3d)) {
      printf("Error streaming from QTM\n");
      return false;
    }
  } else {
    if (!rtProtocol.StreamFrames(CRTProtocol::RateAllFrames, 0, 0, NULL,
                                 CRTProtocol::cComponent3d)) {
      printf("Error streaming from QTM\n");
      return false;
    }
  }
  printf("Streaming 3D data\n\n");
  gConnected = true;
  Bela_scheduleAuxiliaryTask(gFillBufferTask);
  return true;
}

void render(BelaContext *context, void *userData) {
  float out;
  for (unsigned int n = 0; n < context->audioFrames; n++) {
    for (unsigned int channel = 0; channel < context->audioOutChannels;
         channel++) {
      out = 0.8f * sinf_neon(gPhase[channel]);
      gPhase[channel] += 2.0f * (float)M_PI * gFreq[channel] * gInverseSampleRate[channel];
      if (gPhase[channel] > M_PI) gPhase[channel] -= 2.0f * (float)M_PI;

      audioWrite(context, n, channel, out);
    }
  }
  Bela_scheduleAuxiliaryTask(gFillBufferTask);
}

void cleanup(BelaContext *context, void *userData) {
  // disconnect nicely from QTM
  if (gConnected) {
    gConnected = false;
    rtProtocol.Disconnect();
  }
}