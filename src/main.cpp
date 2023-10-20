#include <Arduino.h>
#include <vector>
#include <ledhelper.h>
#include <messagehandler.h>
#include <WiFi.h>
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

void setup() {
    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(YELLOW_LED_PIN, OUTPUT);
    Serial.begin(921600);

    // Set up the PI PICO serial port
    Serial1.begin(921600, SERIAL_8N1, RX_PIN, TX_PIN);

    // print mac address
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());

    // connect to Wi-Fi
    WiFiClass::mode(WIFI_STA);

    // set name of this device for the router to see
    WiFiClass::setHostname("UDOO-KEY-1");

    WiFi.disconnect();
}

// define the variables that could be used over multiple loops if a message is incomplete during that loop

void loop() {
    // if Wi-Fi is disconnected, reconnect
    if (!WiFi.isConnected()) {

        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        while (!WiFi.isConnected()) {
            Serial.println("Connecting to Wi-Fi...");
            Serial.println(WIFI_SSID);
            delay(100);
        }

        // make a request to the server to get the IP address
        WiFiClient client;

        if (!client.connect("google.com", 443)) {
            Serial.println("Connection to server failed");
            return;
        }

        client.stop();
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
        Serial.print("Receiving: ");
        Serial.println((char) data);

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