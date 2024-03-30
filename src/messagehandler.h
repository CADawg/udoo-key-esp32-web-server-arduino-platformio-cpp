//
// Created by conor on 01/10/23.
//

#ifndef ARDUINO_WEBSERVER_MESSAGEHANDLER_H
#define ARDUINO_WEBSERVER_MESSAGEHANDLER_H

#include <map>
#include "wiretransmission.h" // Assuming this is the corresponding header for wiretransmission.cpp

/**
 * Handles a received WireTransmission message.
 *
 * @param message The received WireTransmission message.
 */
void HandleMessage(const WireTransmission& message);

typedef struct {
    uint64_t timestamp;
    tm dateTime;
    std::basic_string<char> response;
} ResponseCacheEntry;

// make a map to cache the latest responses
typedef std::map<String, ResponseCacheEntry> ResponseCache;

extern ResponseCache responseCache;

extern int LastHttpResponseID;
extern std::basic_string<char> LastHttpResponse;
extern std::basic_string<char> LastHttpError;

extern int CacheExpirationTime;
ResponseCacheEntry GetResponseCacheEntry(const String& url, bool acceptExpired = false);

#endif // ARDUINO_WEBSERVER_MESSAGEHANDLER_H
