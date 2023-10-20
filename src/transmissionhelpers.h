//
// Created by conor on 01/10/23.
//

#ifndef ARDUINO_WEBSERVER_TRANSMISSION_HANDLER_H
#define ARDUINO_WEBSERVER_TRANSMISSION_HANDLER_H

#include <vector>
#include <string>
#include "wiretransmission.h"

// Type definitions
typedef std::string Error;

// Constants
#define START_HEADER 0x01
#define START_TEXT 0x02
#define END_TEXT 0x03
#define END_TRANSMISSION 0x04
#define ESCAPE_CHAR 0x1B

// External variables (you might consider making these static or encapsulate them in a class to avoid global state)
extern std::vector<WireTransmission> receiveChannel;
extern std::vector<WireTransmission> sendChannel;

// Function prototypes

/**
 * Get an available ID for the transmission.
 *
 * @return An available ID.
 */
int GetAvailableID();

/**
 * Rebroadcast a message using its ID.
 *
 * @param id The ID of the message.
 * @return An error string if there's an issue.
 */
Error Rebroadcast(int id);

/**
 * Send a message.
 *
 * @param wt The message to be sent.
 * @return An error string if there's an issue.
 */
Error SendMessage(WireTransmission wt);

/**
 * Request a rebroadcast of a message by its ID.
 *
 * @param id The ID of the message.
 * @return An error string if there's an issue.
 */
Error RequestRebroadcast(int id);

/**
 * Handle an incoming byte of data.
 *
 * @param data The incoming byte.
 */
void HandleReceiveByte(uint8_t data);

#endif // ARDUINO_WEBSERVER_TRANSMISSION_HANDLER_H
