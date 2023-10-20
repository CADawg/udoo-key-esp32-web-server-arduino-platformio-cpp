//
// Created by conor on 01/10/23.
//

#include <messagehandler.h>
#include <wiretransmission.h>

void HandleMessage(const WireTransmission& message) {
    std::string messageType = getHeader(message.headers, "type");
}