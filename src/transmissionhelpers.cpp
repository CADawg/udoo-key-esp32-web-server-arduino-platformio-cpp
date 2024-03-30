//
// Created by conor on 01/10/23.
//

#include <wiretransmission.h>
#include <transmissionhelpers.h>

// as these are extern in the header, we need to define them here
std::vector<WireTransmission> receiveChannel;
std::vector<WireTransmission> sendChannel;

std::string currentHeader;
std::string currentBody;
std::vector<uint8_t> currentChecksum;

bool isStreamingHeader = false;
bool isStreamingBody = false;
bool isStreamingChecksum = false;
bool isEscaping = false;

// send cache index array
std::vector<WireTransmission*> sendCache(128);
// send cache index
int sendCacheIndex = 0;

bool IsStreaming() {
    return isStreamingHeader || isStreamingBody || isStreamingChecksum;
}

int GetAvailableID() {
    sendCacheIndex += 1;
    if (sendCacheIndex >= sendCache.size()) {
        sendCacheIndex = 0;
    }

    // wipe old data
    sendCache[sendCacheIndex] = nullptr;

    // we always return the next number because we'll never know if the transmission failed (otherwise we'd use the entire bandwidth for acknowledgements of acknowledgements)
    return sendCacheIndex;
}

Error Rebroadcast(int id) {
    if (id < 0 || id >= sendCache.size()) {
        return "invalid id";
    }

    if (sendCache[id] == nullptr) {
        return "no transmission with that id";
    }

    sendChannel.push_back(*sendCache[id]);

    return "";
}

std::pair<int, Error> SendMessage(WireTransmission wt) {
    // get ID
    int id = GetAvailableID();

    // if we couldn't get an ID, just ignore it (if we lose this packet it's just never going to be answered)
    if (id != -1) {
        wt.headers.push_back({"id", std::to_string(id)});
    }

    // check the headers are valid
    std::pair<std::string, Error> test = serializeHeaders(wt.headers);

    if (!test.second.empty()) {
        return std::make_pair(-1, test.second);
    }

    sendChannel.push_back(wt);

    // add to cache (evicting oldest)
    sendCache[id] = &wt;

    return std::make_pair(id, "");
}

std::pair<int, Error> RequestRebroadcast(int id) {
    WireTransmission wt;
    wt.headers.push_back({"type", "requestRebroadcast"});
    wt.body = std::to_string(id);

    return SendMessage(wt);
}

void HandleReceiveByte(uint8_t data) {
    if (data == START_HEADER && !isEscaping) {
        // set header to streaming and body to not streaming
        isStreamingHeader = true;
        isStreamingBody = false;
        isStreamingChecksum = false;
        // reset the current header
        currentHeader.clear();
    } else if (data == START_TEXT && !isEscaping) {
        // set the body to streaming
        isStreamingBody = true;
        isStreamingHeader = false;
        isStreamingChecksum = false;
        // reset the current body
        currentBody.clear();
    } else if (data == END_TEXT && isStreamingBody && !isEscaping) {
        isStreamingBody = false;
        isStreamingHeader = false;
        isStreamingChecksum = true;

        // reset the current checksum
        currentChecksum.clear();
    } else if (data == END_TRANSMISSION && !isEscaping) {
        // set the body to not streaming
        isStreamingBody = false;
        isStreamingHeader = false;
        isStreamingChecksum = false;

        // casting to string should be enough to utf-8 decode the bytes
        std::pair<WireTransmission, Error> deserializedWireTransmission = deserializeWireTransmission(currentHeader, currentBody, currentChecksum);

        if (deserializedWireTransmission.second.empty()) {
            receiveChannel.push_back(deserializedWireTransmission.first);
        }
    } else if (data == ESCAPE_CHAR && !isEscaping) {
        isEscaping = true;
    } else if (isStreamingHeader) {
        // Add the next byte to the current header
        currentHeader += (char)data;
        // reset the escaping flag
        isEscaping = false;
    } else if (isStreamingBody) {
        // Add the next byte to the current body
        currentBody += (char)data;
        // reset the escaping flag
        isEscaping = false;
    } else if (isStreamingChecksum) {
        currentChecksum.push_back(data);
        isEscaping = false;
    }
}