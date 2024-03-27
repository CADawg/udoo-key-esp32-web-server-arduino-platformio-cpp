#include <Arduino.h>
#include <vector>
#include <ledhelper.h>
#include <messagehandler.h>
#include <WiFi.h>
#include <map>
#include "transmissionhelpers.h"

#define RX_PIN 22   // Serial1 receive pin (connected to RP2040 transmit pin)
#define TX_PIN 19   // Serial1 transmit pin (connected to RP2040 receive pin)

#ifndef WIFI_SSID
#define WIFI_SSID "(no ssid set)"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "(no password set)"
#endif

ulong lastSentMessage = millis();

typedef std::string Error;

WiFiServer server(80);

void setup() {
    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);
    Serial.begin(115200);

    // Set up the PI PICO serial port
    Serial1.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

    // print mac address
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());

    // connect to Wi-Fi921600
    WiFiClass::mode(WIFI_STA);

    // set name of this device for the router to see
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname("UDOO-KEY-1");

    WiFi.disconnect();

    // print free flash space to Serial
    Serial.print("Free flash: ");
    Serial.println(ESP.getFreeSketchSpace());
    Serial.print("Total Flash: ");
    Serial.println(ESP.getFlashChipSize());
}

// define the variables that could be used over multiple loops if a message is incomplete during that loop

WiFiClient serverClient;
bool currentConnectionOpen = false;
int currentRequestID = -1;
String mimeType = "";
ulong requestStartTime = 0;

typedef struct {
    uint64_t timestamp;
    std::basic_string<char> response;
} ResponseCacheEntry;

// make a map to cache the latest responses
typedef std::map<String, ResponseCacheEntry> ResponseCache;

String getFormattedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "Failed to obtain time";
    }

    // Create a string to represent the timestamp
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);

    return {strftime_buf};
}

void loop() {
    // if Wi-Fi is disconnected, reconnect
    if (!WiFi.isConnected()) {

        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        while (!WiFi.isConnected()) {
            Serial.println("Connecting to Wi-Fi...");
            Serial.println(WIFI_SSID);
            delay(100);
        }

        // get real time from the internet
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");

        // make a request to the server to get the IP address
        WiFiClient client;

        if (!client.connect("google.com", 443)) {
            Serial.println("Connection to server failed");
            return;
        }

        client.stop();

        server.stop();

        server.begin();

        Serial.println("Server started");
    }

    if (!currentConnectionOpen) {
        // open session on the server
        serverClient = server.available();

        if (serverClient.connected() && serverClient.available()) {
            Serial.println("Client connected");
            currentConnectionOpen = true;

            // reset in case of a new connection
            LastHttpResponseID = -1;
            LastHttpResponse = "";
            LastHttpError = "";

            currentRequestID = -1;
            mimeType = "";
            requestStartTime = 0;

            // to prevent slow loris attacks, we'll close the connection after 5 seconds
            requestStartTime = millis();

            // read the request
            String request = serverClient.readStringUntil('\r');

            String requestURL = request.substring(5, request.indexOf(" ", 5));

            // ask the pico for this webpages data
            WireTransmission wt;

            wt.headers.push_back({"type", "http"});
            wt.headers.push_back({"url", requestURL.c_str()});
            wt.body = "";

            std::pair<int, Error> data = SendMessage(wt);

            if (!data.second.empty()) {
                Serial.println("Error sending message: ");
                Serial.println(data.second.c_str());

                serverClient.println("HTTP/1.1 500 Internal Server Error");
                serverClient.println("Content-Type: text/html");
                serverClient.println("Connection: close");
                serverClient.println();
                serverClient.println("<!DOCTYPE HTML>");
                serverClient.println("<html>");
                serverClient.println("<h1>Internal Server Error</h1>");
                serverClient.println("<p>UDOO Key Webserver</p>");
                serverClient.println("</html>");
                serverClient.stop();
                currentConnectionOpen = false;
            } else {
                currentRequestID = data.first;

                // work out mime type by file extension (split on . and take the last element)
                size_t lastDot = requestURL.lastIndexOf(".");
                mimeType = "text/html";

                if (lastDot != -1 && lastDot < requestURL.length() - 1) {
                    String extension = requestURL.substring(lastDot + 1, requestURL.length()).c_str();

                    if (extension == "css") {
                        mimeType = "text/css";
                    } else if (extension == "js") {
                        mimeType = "text/javascript";
                    } else if (extension == "ico") {
                        mimeType = "image/x-icon";
                    } else if (extension == "svg") {
                        mimeType = "image/svg+xml";
                    } else if (extension == "json") {
                        mimeType = "application/json";
                    } else if (extension == "txt") {
                        mimeType = "text/plain";
                    } else if (extension == "webp") {
                        mimeType = "image/webp";
                    } else {
                        mimeType = "text/html";
                    }
                }
            }
        }
    } else {
        // if the connection has been open for more than 5 seconds, close it
        if (requestStartTime + 2000 < millis()) {
            Serial.println("Timeout");
            serverClient.println("HTTP/1.1 408 Request Timeout");
            serverClient.println("Content-Type: text/html");
            serverClient.println("Connection: close");
            serverClient.println();
            serverClient.println("<!DOCTYPE HTML>");
            serverClient.println("<html>");
            serverClient.println("<h1>Request timed out</h1>");
            serverClient.println("<p>UDOO Key Webserver</p>");
            serverClient.println("</html>");

            currentConnectionOpen = false;
            serverClient.stop();
        } else {
            // check if we have the response
            if (LastHttpResponseID == currentRequestID && currentRequestID != -1) {
                serverClient.println("HTTP/1.1 200 OK");
                serverClient.println("Content-Type: " + mimeType);
                serverClient.println("Connection: close");

                // if it's html replace the hard coded strings {serve_location} and {utc_timestamp}
                if (mimeType == "text/html") {
                    serverClient.println("Cache-Control: no-cache");
                    serverClient.println();
                    String response = LastHttpResponse.c_str();
                    response.replace("{serve_location}", "Pi Pico (Uncached)");
                    response.replace("{utc_timestamp}", getFormattedTime());

                    serverClient.println(response);
                } else if (mimeType == "image/webp") {
                    serverClient.println("Cache-Control: public, max-age=31536000, immutable");
                    serverClient.println();
                    serverClient.write(LastHttpResponse.c_str(), LastHttpResponse.length());
                } else {
                    serverClient.println();
                    serverClient.write(LastHttpResponse.c_str(), LastHttpResponse.length());
                }

                currentConnectionOpen = false;
                serverClient.stop();
            } else if (!LastHttpError.empty()) {
                serverClient.println("HTTP/1.1 500 Internal Server Error");
                serverClient.println("Content-Type: text/html");
                serverClient.println("Connection: close");
                serverClient.println();
                serverClient.println("<!DOCTYPE HTML>");
                serverClient.println("<html>");
                serverClient.println("<h1>Internal Server Error</h1>");
                serverClient.print("<p>");
                serverClient.print(LastHttpError.c_str());
                serverClient.println("</p>");
                serverClient.println("<p>UDOO Key Webserver</p>");
                serverClient.println("</html>");

                currentConnectionOpen = false;
                serverClient.stop();
            }
        }
    }

    // turn off the LED if it's been on for the required time
    ledTick();

    // if there is data on the serial port to be read, read all the bytes until there are no more
    while (Serial1.available()) {
        // set the rx led to on
        turnOnLed(YELLOW_LED_PIN);

        // read the next byte
        uint8_t data = Serial1.read();

        // print to debugging serial what we're receiving as strings
        //Serial.print("Receiving: ");
        //Serial.println((char) data);

        HandleReceiveByte(data);
    }

    // if there is data on the send channel, send it
    while (!sendChannel.empty()) {
        // we need to escape the StartHeader, StartText, EndText and EndTransmission bytes
        // , so we can use them to mark the start and end of the header and body
        // we do this by adding an EscapeChar before the byte
        // this way we can differentiate between the bytes that mark the start and end of the header and body

        // get the next header and body from the send channel
        WireTransmission data = sendChannel[0];

        turnOnLed(BLUE_LED_PIN);

        std::pair<std::vector<byte>, Error> wtData = serializeWireTransmission(data);

        if (!wtData.second.empty()) {
            continue;
        }

        // send it byte by byte
        for (byte b : wtData.first) {
            Serial1.write(b);
        }

        // remove the header and body from the send channel
        sendChannel.erase(sendChannel.begin());
    }

    if (lastSentMessage + 5000 < millis()) {
        WireTransmission wt;
        wt.headers.push_back({"type", "ping"});
        wt.body = "ping";

        SendMessage(wt);

        lastSentMessage = millis();
    }

    // if there is data on the receipt channel, process it
    if (!receiveChannel.empty()) {
        WireTransmission message = receiveChannel[0];

        // handle rebroadcasts separately as they're not a message we'd want to handle with a custom handler
        if (getHeader(message.headers, "type") == "requestRebroadcast") {
            // parse str int
            try {
                int rebroadcastID = std::stoi(message.body);

                Error err = Rebroadcast(rebroadcastID);

                if (!err.empty()) {
                    Serial.println(err.c_str());
                }
                // catch int parse error
            } catch (std::invalid_argument const& ex) {
                Serial.println("invalid argument");
            } catch (std::out_of_range const& ex) {
                Serial.println("out of range");
            }
        } else {
            HandleMessage(message);
        }

        // remove the message from the receipt channel
        receiveChannel.erase(receiveChannel.begin());
    }
}