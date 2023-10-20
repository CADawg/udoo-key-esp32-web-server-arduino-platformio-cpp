//
// Created by conor on 28/09/23.
//
#include <Arduino.h>
#include <vector>
#include "headers.h"

const std::string MagicHeaderStart = "HEADERS\n";

// a function with 2 return values (string, err) to equate to this go func
std::pair<std::string, Error> serializeHeader(const Header& header) {
    if (header.key.empty()) {
        return std::make_pair("", "header key is empty");
    }

    if (header.key.find(':') != std::string::npos) {
        return std::make_pair("", "header key contains colon");
    }

    return std::make_pair(header.key + ":" + header.value, "");
}

// Headers -> Get (key string) (value string, err string) callable on Headers type
std::string getHeader(const Headers& headers, const std::string& key) {
    for (const Header& header : headers) {
        if (header.key == key) {
            return header.value;
        }
    }

    return "";
}

// Headers -> Serialize
std::pair<std::string, Error> serializeHeaders(const Headers& headers) {
    std::string serialisedHeaders;

    // add magic start chars
    serialisedHeaders += MagicHeaderStart;

    for (const Header& header : headers) {
        std::pair<std::string, std::string> serialisedHeader = serializeHeader(header);
        if (!serialisedHeader.second.empty()) {
            return std::make_pair("", serialisedHeader.second);
        }

        serialisedHeaders += serialisedHeader.first + "\n";
    }

    return std::make_pair(serialisedHeaders, "");
}

// Headers -> Deserialize
std::pair<Headers, Error> deserializeHeaders(const std::string& data) {
    Headers headers;

    // check if start is correct
    if (data.substr(0, MagicHeaderStart.length()) != MagicHeaderStart) {
        return std::make_pair(headers, "invalid headers start");
    }

    // remove magic start chars
    std::string headersData = data.substr(MagicHeaderStart.length());

    // split by new line
    std::vector<std::string> headersVector;
    std::string headerRead;
    for (char c : headersData) {
        if (c == '\n') {
            headersVector.insert(headersVector.end(), headerRead);
            headerRead = "";
        } else {
            headerRead += c;
        }
    }

    // iterate over headers
    for (const std::string& header : headersVector) {
        // split by colon
        std::vector<std::string> headerParts;
        std::string headerPart;
        for (char c : header) {
            if (c == ':') {
                headerParts.insert(headerParts.end(), headerPart);
                headerPart = "";
            } else {
                headerPart += c;
            }
        }

        // insert final part
        headerParts.insert(headerParts.end(), headerPart);

        if (headerParts.size() < 2) {
            continue; // ignore empty / half headers (even empty headers should have the :)
        }

        // if header has more than 1 :, join all the remaining parts (support ipv6 and other : containing headers)
        if (headerParts.size() > 2) {
            std::vector<std::string> headerParts2;
            headerParts2.insert(headerParts2.end(), headerParts[0]);
            std::string headerParts2Part;
            for (int i = 1; i < headerParts.size(); i++) {
                headerParts2Part += headerParts[i];

                if (i != headerParts.size() - 1) {
                    headerParts2Part += ":";
                }
            }
            headerParts2.insert(headerParts2.end(), headerParts2Part);
            headerParts = headerParts2;
        }

        // add header to headers
        headers.insert(headers.end(), {headerParts[0], headerParts[1]});
    }

    return std::make_pair(headers, "");
}