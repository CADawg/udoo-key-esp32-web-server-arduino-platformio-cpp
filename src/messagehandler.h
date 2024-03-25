//
// Created by conor on 01/10/23.
//

#ifndef ARDUINO_WEBSERVER_MESSAGEHANDLER_H
#define ARDUINO_WEBSERVER_MESSAGEHANDLER_H

#include "wiretransmission.h" // Assuming this is the corresponding header for wiretransmission.cpp

/**
 * Handles a received WireTransmission message.
 *
 * @param message The received WireTransmission message.
 */
void HandleMessage(const WireTransmission& message);

extern int LastHttpResponseID;
extern String LastHttpResponse;
extern String LastHttpError;

#endif // ARDUINO_WEBSERVER_MESSAGEHANDLER_H
