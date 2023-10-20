//
// Created by conor on 30/09/23.
//

#ifndef ARDUINO_WEBSERVER_WIRETRANSMISSION_H
#define ARDUINO_WEBSERVER_WIRETRANSMISSION_H

#include <Arduino.h>
#include <vector>
#include <headers.h>  // Ensure you have a headers.h corresponding to headers.cpp

// Define constants if they are used outside wiretransmission.cpp
#define ERR_CHECKSUM_INVALID "checksum invalid"
#define ERR_CHECKSUM_INVALID_REQUESTED_REBROADCAST "checksum invalid, requested rebroadcast"

typedef struct {
    Headers headers;
    std::string body;
    std::vector<uint8_t> checksum;
} WireTransmission;

// Function prototypes for the public functions in wiretransmission.cpp
uint32_t bytesToCRC32(const std::vector<uint8_t>& bytes);
std::vector<uint8_t> Crc32ToBytes(const std::string& bytes);
uint32_t crc32String(const std::string& bytes);
std::pair<WireTransmission, Error> deserializeWireTransmission(const std::string& headers, const std::string& body, const std::vector<uint8_t>& checksum);
std::vector<byte> EncodeByteSafe(byte b);
std::pair<std::vector<byte>, Error> serializeWireTransmission(WireTransmission wireTransmission);

#endif // ARDUINO_WEBSERVER_WIRETRANSMISSION_H
