#include <Bela.h>

#include <libraries/math_neon/math_neon.h>
#include <libraries/AudioFile/AudioFile.h>

#include <array>
#include <iterator>
#include <string>

#include "qsdk/RTPacket.h"
#include "qsdk/RTProtocol.h"

// #include "utils/latency_check.cpp" // include this file to do a latency check

// how much history to keep
#define NUM_SAMPLES 2
// how many signals to process (at the moment this is required to be 2)
// because we match them to out channels...for now.
#define NUM_SUBJECTS 2
// just x, y, z. but maybe we want to track other things?
#define NUM_COORDS 3

/*
 * USER CONFIGURATION
 */

// names of tracked markers in QTM.
const std::array<std::string, NUM_SUBJECTS> gSubjMarkerLabels{{"CAR1", "CAR2"}};
// IDs of corresponding markers will be stored here.
std::array<int, NUM_SUBJECTS> gSubjMarker{};

// minimum frequency for generated sin wave.
const float gFreqMin = 250.0;
// maximum frequencey for generated sin wave.
const float gFreqMax = 1000.0;

// minimum distance that should be tracked (this is mm/frame)
const float gStepDistanceMin = 0.025;
// maximum distance that tracked markers will move in a single frame (mm/frame).
const float gStepDistanceMax = 43.2;

const std::string gUndertoneFile = "./res/130bpm_8thnote_LFO_As3_1osc.wav";
const std::string gOvertoneFile = "./res/130bpm_8thnote_LFO_F4_1osc.wav";

const int gSampleLength = 10604;


// minimum frequency for playback.
const float gFreqMin1 = 232.819;
// maximum frequencey for playback.
const float gFreqMax1 = 369.577;

// minimum frequency for playback.
const float gFreqMin2 = 349.23;
// maximum frequencey for playback.
const float gFreqMax2 = 554.365;

// track start and end points (on corresponding axis)
const float gTrackStart = 0.0;
const float gTrackEnd = 1.0;

unsigned int gReadPtr;
// track axis
const int gTrackAxis = 0;



// use UDP for QTM connection.
// UDP has less overhead so try to use that if no problems.
const bool gStreamUDP = true;

/*
 * GLOBAL VARIABLES
 */

// record of maximum distance travelled, for debugging.
std::array<float, NUM_SUBJECTS> gMaxStep{};

// record of last rendered QTM frame.
unsigned int gLastFrame = 0;

float gOut;

// array of size n_subjects x n_history x 3 (x, y, z)
std::array<std::array<std::array<float, NUM_COORDS>, NUM_SUBJECTS>, NUM_SAMPLES>
    gPos3D{};

// keep track of last step distance for each subject.
std::array<float, NUM_SUBJECTS> gStepDistance{};

// frequency
std::array<float, NUM_SUBJECTS> gFreq{};

// QTM protocol
CRTProtocol* rtProtocol = NULL;

// QTM communication packet
CRTPacket* rtPacket = NULL;

// QTM packet type (we want Data Packets (CRTPacket::PacketData).
CRTPacket::EPacketType packetType;

// are we connected to QTM?
bool gConnected = false;

// have we initialized the variables.
bool gInitialized = false;

// current sin wave phase (per channel)
std::array<float, NUM_SUBJECTS> gPhase{};

// current inverse sample rate (per channel)
std::array<float, NUM_SUBJECTS> gInverseSampleRate{};

std::vector<float> gUndertoneSampleData;

std::vector<float> gOvertoneSampleData;

// define Bela aux task to avoid render slowdown.
AuxiliaryTask gFillBufferTask;

// wrapper to retrieve latest QTM 3d packet
bool get3DPacket() {
  // Ask QTM for latest packet.
  if (rtProtocol->Receive(packetType, true) != CNetwork::ResponseType::success) {
    printf("Problem reading data...\n");
    return false;
  }
  // we got a data packet from QTM
  if (packetType == CRTPacket::PacketData) {
    // get the current data packet
    rtPacket = rtProtocol->GetRTPacket();
    return true;
  } else {
    return false;
  }
}

// update buffer of QTM data
void fillBuffer(void *) {
  // Make sure we successfully get the data
  if (!get3DPacket()) return;

  // this helps us when we're doing realtime playback, because it loops.
  const unsigned int uPacketFrame = rtPacket->GetFrameNumber();

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

    // Update max step distance but don't include QTM loop.
    if (gStepDistance[i] > gMaxStep[i] && uPacketFrame > gLastFrame) {
      printf(
          "%s [%d]: New max step distance for %9.3f curPos: x: %9.3f, y: "
          "%9.3f, z: %9.3f\n",
          gSubjMarkerLabels[i].c_str(), uPacketFrame, gStepDistance[i],
          currPos[0], currPos[1], currPos[2]);
      gMaxStep[i] = gStepDistance[i];
    }

    // limit the range so we don't get rogue values.
    gStepDistance[i] =
        constrain(gStepDistance[i], gStepDistanceMin, gStepDistanceMax);

    // map the step distnce range to frequency range.
    gFreq[i] = map(gStepDistance[i], gStepDistanceMin, gStepDistanceMax,
                   gFreqMin, gFreqMax);
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

// bela setup task
bool setup(BelaContext *context, void *userData) {
  // create fill buffer function auxillary task
  if ((gFillBufferTask =
           Bela_createAuxiliaryTask(&fillBuffer, 90, "fill-buffer")) == 0)
    return false;

  // initialize some default variable values.
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
  unsigned short nPort = 0;
  
  rtProtocol = new CRTProtocol();
  // Connect to the QTM application
  if (!rtProtocol->Connect(serverAddr, basePort, &nPort, majorVersion, minorVersion,
                          bigEndian)) {
  	printf("\nFailed to connect to QTM RT Server. %s\n\n", rtProtocol->GetErrorString());
                system("pause");
    return false;
  }
    
  printf("Connected to QTM...");

  // allocate char to stor version
  char qtmVer[64];
  // request version from QTM
  rtProtocol->GetQTMVersion(qtmVer, sizeof(qtmVer));
  // print the version
  printf("%s\n", qtmVer);
#ifdef CHECK_CMD_LATENCY
  checkLatency(rtProtocol);
#endif

  printf("\n");
  
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
  printf("Streaming 3D data\n\n");
  gConnected = true;

  // number of labelled markers
  const unsigned int nLabels = rtProtocol->Get3DLabeledMarkerCount();
  printf("Found labels: \n");
  // loop through labels to find ones we are interested in.
  for (unsigned int i = 0; i < nLabels; i++) {
    const char *cLabelName = rtProtocol->Get3DLabelName(i);
    printf("- %s\n", cLabelName);
    for (unsigned int j = 0; j < NUM_SUBJECTS; j++) {
      // if the label is one of our specified markers, keep the ID.
      if (cLabelName == gSubjMarkerLabels[j]) {
        printf("Found marker: %s id: %d\n", cLabelName, i);
        gSubjMarker[j] = i;
      }
    }
  }
  // start getting the 3D data.
  Bela_scheduleAuxiliaryTask(gFillBufferTask);


  gUndertoneSampleData = AudioFileUtilities::loadMono(gUndertoneFile);
  gOvertoneSampleData = AudioFileUtilities::loadMono(gOvertoneFile);

  return true;
}

// this will probably change, but a simple function
// to get a value for a given phase and frequency
// for a sin wave
float sin_freq(float &phase, float freq, float inv_sr) {
  const float out = 0.8f * sinf_neon(phase);
  phase += 2.0f * (float)M_PI * freq * inv_sr;
  if (phase > M_PI) phase -= 2.0f * (float)M_PI;
  return out;
}

void warp_index(unsigned int &index, float base_sr, float warp_sr) {
  index = round((warp_sr / base_sr) * (float)index);
  while (index > gSampleLength) index -= gSampleLength;
}

// bela main render loop function
void render(BelaContext *context, void *userData) {
  // this is how many audio frames are rendered per loop
  for (unsigned int n = 0; n < context->audioFrames; n++) {
    // there will probably always be 2 out channels
    ++gReadPtr;
    // replace with skip frame instead 
	  warp_index(gReadPtr, gFreqMin2, gFreqMax2);
    for (unsigned int channel = 0; channel < context->audioOutChannels;
         channel++) {
          
      // get the value for the current sample.
      // gOut = sin_freq(gPhase[channel], gFreq[channel],
      //               gInverseSampleRate[channel]);
      gOut = (gUndertoneSampleData[gReadPtr] + gOvertoneSampleData[gReadPtr]) * 0.5f;
      audioWrite(context, n, channel, gOut);
    }
  }
  // just keep polling for 3D data.
  Bela_scheduleAuxiliaryTask(gFillBufferTask);
}

// bela cleanup function
void cleanup(BelaContext *context, void *userData) {
  // disconnect nicely from QTM
  if (gConnected) {
    gConnected = false;
    rtProtocol->Disconnect();
  }
}