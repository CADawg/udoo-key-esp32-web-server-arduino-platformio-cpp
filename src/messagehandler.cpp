//
// Created by conor on 01/10/23.
//

#include <messagehandler.h>
#include <wiretransmission.h>

int LastHttpResponseID;
std::basic_string<char> LastHttpResponse;
std::basic_string<char> LastHttpError;

ResponseCache responseCache;

int CacheExpirationTime = 60 * 5 * 1000;  // 5 mins

void HandleMessage(const WireTransmission& message) {
    // print message neatly
    Serial.println();
    Serial.println();
    Serial.println("Received message:");
    Serial.println(message.headers.size());
    for (const auto& header : message.headers) {
        Serial.println(header.key.c_str());
        Serial.println(header.value.c_str());
    }
    Serial.println();
    Serial.println();

    Serial.println(message.body.c_str());

    std::string messageType = getHeader(message.headers, "type");

    if (messageType == "http") {
        // parse id
        std::string responseToHeader = getHeader(message.headers, "responseTo");

        if (responseToHeader.empty()) {
            LastHttpError = "no id header";
            return;
        }

        LastHttpResponseID = std::stoi(responseToHeader);

        std::string statusHeader = getHeader(message.headers, "status");

        if (statusHeader.empty()) {
            LastHttpError = "no status header";
            return;
        }

        if (statusHeader == "ok") {
            LastHttpError = "";
            // parse body
            LastHttpResponse = message.body;

            // get the url from the headers
            String url = getHeader(message.headers, "url").c_str();

            struct tm timeinfo;
            getLocalTime(&timeinfo);

            // Update the std::map with the latest response
            ResponseCacheEntry entry = {millis(), timeinfo, LastHttpResponse};

            responseCache[url] = entry;
        } else {
            LastHttpError = "http error";
            // parse body
            LastHttpError = message.body;
        }

        // display the result
        Serial.println("Received HTTP response:");
        Serial.println(LastHttpResponseID);
        Serial.println(LastHttpResponse.c_str());
        Serial.println(LastHttpError.c_str());
    }
}

ResponseCacheEntry GetResponseCacheEntry(const String& url, bool acceptExpired) {
    // check if the url is in the cache
    if (responseCache.find(url) == responseCache.end()) {
        return {}; // return an empty object
    }

    // get the entry
    ResponseCacheEntry entry = responseCache[url];

    // check if the entry is expired
    if (!acceptExpired && entry.timestamp + CacheExpirationTime < millis()) {
        return {}; // return an empty object
    }

    return entry;
}