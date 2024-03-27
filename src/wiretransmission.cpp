//
// Created by conor on 30/09/23.
//
#include <wiretransmission.h>
#include <crc32/CRC32.h>
#include <Arduino.h>
#include <vector>
#include <headers.h>
#include "transmissionhelpers.h"

// bytesToCRC32 converts a byte array back to a CRC32 checksum (int32)
uint32_t bytesToCRC32(const std::vector<uint8_t>& bytes) {
    return (int32_t) bytes[0] << 24 | (int32_t) bytes[1] << 16 | (int32_t) bytes[2] << 8 | (int32_t) bytes[3];
}

// crc32toBytes calculates the crc32 of the input bytes and returns it as a byte array
std::vector<uint8_t> Crc32ToBytes(const std::string& bytes) {
    CRC32 crc32;

    crc32.update(bytes.data(), bytes.size());

    uint32_t crc32int = crc32.finalize();

    std::vector<uint8_t> crc32Bytes;
    crc32Bytes.push_back((uint8_t) (crc32int >> 24));
    crc32Bytes.push_back((uint8_t) (crc32int >> 16));
    crc32Bytes.push_back((uint8_t) (crc32int >> 8));
    crc32Bytes.push_back((uint8_t) crc32int);

    return crc32Bytes;
}

uint32_t crc32String(const std::string& bytes) {
    CRC32 crc32;

    crc32.update(bytes.data(), bytes.size());

    return crc32.finalize();
}

std::pair<WireTransmission, Error> deserializeWireTransmission(const std::string& headers, const std::string& body, const std::vector<uint8_t>& checksum) {
    WireTransmission wireTransmission;

    // deserialize headers
    std::pair<Headers, Error> deserializedHeaders = deserializeHeaders(headers);

    // if there's an error, return it
    if (!deserializedHeaders.second.empty()) {
        return std::make_pair(wireTransmission, deserializedHeaders.second);
    }

    wireTransmission.headers = deserializedHeaders.first;

    // check checksum
    if (bytesToCRC32(checksum) != crc32String(headers + body)) {
        // if the checksum is invalid, check if the request is for a rebroadcast
        std::string rebroadcastHeader = getHeader(wireTransmission.headers, "id");
        if (!rebroadcastHeader.empty()) {
            // parse str int
            int rebroadcastID = std::stoi(rebroadcastHeader);

            std::pair<int, Error> data = RequestRebroadcast(rebroadcastID);

            if (!data.second.empty()) {
                return std::make_pair(wireTransmission, data.second);
            }

            return std::make_pair(wireTransmission, ERR_CHECKSUM_INVALID_REQUESTED_REBROADCAST);
        }

        return std::make_pair(wireTransmission, ERR_CHECKSUM_INVALID);
    }

    wireTransmission.body = body;

    return std::make_pair(wireTransmission, "");
}

std::vector<byte> EncodeByteSafe(byte b) {
    if (b == START_HEADER || b == START_TEXT || b == END_TEXT || b == END_TRANSMISSION || b == ESCAPE_CHAR) {
        return {ESCAPE_CHAR, b};
    }

    return {b};
}

std::pair<std::vector<byte>, Error> serializeWireTransmission(WireTransmission wireTransmission) {
    std::pair<std::string, Error> serializedHeaders = serializeHeaders(wireTransmission.headers);

    if (!serializedHeaders.second.empty()) {
        return std::make_pair(std::vector<byte>(), serializedHeaders.second);
    }

    wireTransmission.checksum = Crc32ToBytes(serializedHeaders.first + wireTransmission.body);

    std::vector<byte> encoded;
    encoded.push_back(START_HEADER);
    for (byte b : serializedHeaders.first) {
        std::vector<byte> encodedByte = EncodeByteSafe(b);
        encoded.insert(encoded.end(), encodedByte.begin(), encodedByte.end());
    }
    encoded.push_back(START_TEXT);
    for (byte b : wireTransmission.body) {
        std::vector<byte> encodedByte = EncodeByteSafe(b);
        encoded.insert(encoded.end(), encodedByte.begin(), encodedByte.end());
    }
    encoded.push_back(END_TEXT);
    for (byte b : wireTransmission.checksum) {
        std::vector<byte> encodedByte = EncodeByteSafe(b);
        encoded.insert(encoded.end(), encodedByte.begin(), encodedByte.end());
    }
    encoded.push_back(END_TRANSMISSION);

    return std::make_pair(encoded, "");
}