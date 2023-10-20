//
// Created by conor on 28/09/23.
//

#ifndef ARDUINO_WEBSERVER_HEADERS_H
#define ARDUINO_WEBSERVER_HEADERS_H

#include <Arduino.h>
#include <vector>

// Type definitions
typedef std::string Error;

typedef struct {
    std::string key;
    std::string value;
} Header;

typedef std::vector<Header> Headers;

// Function prototypes

/**
 * Serializes a given header to a string.
 *
 * @param header The header to be serialized.
 * @return A pair containing the serialized header string and an error string (if any error occurs).
 */
std::pair<std::string, Error> serializeHeader(const Header& header);

/**
 * Retrieves the value of a header by its key from a collection of headers.
 *
 * @param headers The collection of headers.
 * @param key The key of the header whose value is to be retrieved.
 * @return The value of the header with the given key.
 */
std::string getHeader(const Headers& headers, const std::string& key);

/**
 * Serializes a collection of headers to a string.
 *
 * @param headers The collection of headers to be serialized.
 * @return A pair containing the serialized headers string and an error string (if any error occurs).
 */
std::pair<std::string, Error> serializeHeaders(const Headers& headers);

/**
 * Deserializes a string to a collection of headers.
 *
 * @param data The string containing the serialized headers.
 * @return A pair containing the deserialized collection of headers and an error string (if any error occurs).
 */
std::pair<Headers, Error> deserializeHeaders(const std::string& data);

#endif // ARDUINO_WEBSERVER_HEADERS_H
