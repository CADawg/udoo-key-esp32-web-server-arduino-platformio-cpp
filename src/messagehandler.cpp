//
// Created by conor on 01/10/23.
//

#include <messagehandler.h>
#include <wiretransmission.h>

int LastHttpResponseID;
String LastHttpResponse;
String LastHttpError;

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
            LastHttpResponse = message.body.c_str();
        } else {
            LastHttpError = "http error";
            // parse body
            LastHttpError = message.body.c_str();
        }

        // display the result
        Serial.println("Received HTTP response:");
        Serial.println(LastHttpResponseID);
        Serial.println(LastHttpResponse.c_str());
        Serial.println(LastHttpError.c_str());
    }
}