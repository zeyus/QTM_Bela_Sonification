#ifndef QTM_UTILS_H
#define QTM_UTILS_H

#include "../qsdk/RTPacket.h"
#include "../qsdk/RTProtocol.h"


// wrapper to retrieve latest QTM 3d packet
bool get3DPacket(CRTProtocol* rtProtocol, CRTPacket*& rtPacket, CRTPacket::EPacketType& packetType) {
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

bool sendEventLabel(CRTProtocol* rtProtocol, Labels pLabel) {
  return rtProtocol->SetQTMEvent((char*)pLabel);
}

bool sendEventLabel(CRTProtocol* rtProtocol, ConditionLabels pLabel) {
  return rtProtocol->SetQTMEvent((char*)pLabel);
}

bool startCapture(CRTProtocol* rtProtocol) {
  return rtProtocol->StartCapture();
}

bool stopCapture(CRTProtocol* rtProtocol) {
  return rtProtocol->StopCapture();
}

#endif