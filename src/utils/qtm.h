#ifndef QTM_UTILS_H
#define QTM_UTILS_H

#include "../qsdk/RTPacket.h"
#include "../qsdk/RTProtocol.h"

template<typename E>
constexpr auto toUnderlyingType(E e) 
{
    return static_cast<typename std::underlying_type<E>::type>(e);
}


template<typename E>
constexpr bool sendEventLabel(CRTProtocol* rtProtocol, E pLabel) 
{
  const char label = toUnderlyingType(pLabel);
  const char* labelPtr = &label;
  const bool result = rtProtocol->SetQTMEvent(labelPtr);
  if (!result) {
    const char* errorStr = rtProtocol->GetErrorString();
    printf("Error sending event label (%c): %s\n", label, errorStr);
  }
  return result;
}

bool reindexMarkers(CRTProtocol* rtProtocol) {
  unsigned int markersFound = 0;
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
        markersFound++;
      }
    }
  }
  // if we didn't find all the markers, return false.
  if (markersFound != NUM_SUBJECTS) {
    printf("Error: not all markers found.\n");
    return false;
  }
  return true;
}

// wrapper to retrieve latest QTM 3d packet
bool get3DPacket(CRTProtocol* rtProtocol, CRTPacket*& rtPacket, CRTPacket::EPacketType& packetType) {
  // if stream is not open, or we're silenced, don't do anything
  if (!gStreaming || gSilence) {
    return false;
  }
  // Ask QTM for latest packet.
  if (rtProtocol->Receive(packetType, true, gPacketTimeoutMicroSec) != CNetwork::ResponseType::success) {
    printf("Problem reading data...\n");
    // print error
    const char* errorStr = rtProtocol->GetErrorString();
    printf("Error: %s\n", errorStr);
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

bool startCapture(CRTProtocol* rtProtocol) {
  return rtProtocol->StartCapture();
}

bool stopCapture(CRTProtocol* rtProtocol) {
  return rtProtocol->StopCapture();
}

#endif
