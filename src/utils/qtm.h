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
    printf("Error sending event label: %s\n", errorStr);
  }
  return result;
}

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

bool startCapture(CRTProtocol* rtProtocol) {
  return rtProtocol->StartCapture();
}

bool stopCapture(CRTProtocol* rtProtocol) {
  return rtProtocol->StopCapture();
}

#endif
