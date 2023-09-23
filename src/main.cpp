#include <Arduino.h>
#include <vector>

#define TX_LED_PIN GPIO_NUM_32
#define RX_LED_PIN GPIO_NUM_33

#define RX_PIN 22   // Serial1 receive pin (connected to RP2040 transmit pin)
#define TX_PIN 19   // Serial1 transmit pin (connected to RP2040 receive pin)

#define START_HEADER 0x01
#define START_TEXT 0x02
#define END_TEXT 0x03
#define END_TRANSMISSION 0x04
#define ESCAPE_CHAR 0x1B

void setup() {
    pinMode(TX_LED_PIN, OUTPUT);
    pinMode(RX_LED_PIN, OUTPUT);
    Serial.begin(921600);

    // Set up the PI PICO serial port
    Serial1.begin(921600, SERIAL_8N1, RX_PIN, TX_PIN);
}

// define the variables that could be used over multiple loops if a message is incomplete during that loop
typedef struct {
    std::string header;
    std::string body;
} Message;

std::vector<Message> receiveChannel;
std::vector<Message> sendChannel;
std::string currentHeader;
std::string currentBody;
bool isTransmissionCorrupt = false;
bool isStreamingHeader = false;
bool isStreamingBody = false;
bool isStreamingChecksum = false;
bool isEscaping = false;
u_long lastLEDToggleTime = 0;
bool LEDState = false;

void loop() {
    // if there is data on the serial port to be read, read all the bytes until there are no more
    while (Serial1.available()) {
        // define currentHeader, currentBody, isStreamingHeader, isStreamingBody, isEscaping

        // set the rx led to on
        digitalWrite(RX_LED_PIN, HIGH);
        ulong ledOnTime = millis();

        // read the next byte
        uint8_t data = Serial1.read();

        // print to debugging serial what we're receiving as strings
        Serial.print("Receiving: ");
        Serial.println((char) data);

        // if the byte is the start of the header and is not being escaped
        if (data == START_HEADER && !isEscaping) {
            // set header to streaming and body to not streaming
            isStreamingHeader = true;
            isStreamingBody = false;
            // reset the current header
            currentHeader.clear();
            // if the byte is the start of the body and is not being escaped
        } else if (data == START_TEXT && !isEscaping) {
            // set the body to streaming
            isStreamingBody = true;
            isStreamingHeader = false;
            // reset the current body
            currentBody.clear();
            // if the byte is the end of the body and is not being escaped
        } else if ((data == END_TEXT || data == END_TRANSMISSION) && isStreamingBody && !isEscaping) {
            // set the body to not streaming
            isStreamingBody = false;
            isStreamingHeader = false;

            receiveChannel.insert(receiveChannel.end(), {currentHeader, currentBody});
            // if the byte is the end of the transmission and is not being escaped
        } else if (data == ESCAPE_CHAR && !isEscaping) {
            isEscaping = true;
        } else if (isStreamingHeader) {
            // Add the next byte to the current header
            currentHeader.insert(currentHeader.end(), data);
            isEscaping = false;
        } else if (isStreamingBody) {
            // Add the next byte to the current body
            currentBody.insert(currentBody.end(), data);
            isEscaping = false;
        }

        if (millis() - ledOnTime < 10) {
            delay(10 - (millis() - ledOnTime));
        }

        // set the rx led to off
        digitalWrite(RX_LED_PIN, LOW);
    }

    // if there is data on the send channel, send it
    if (!sendChannel.empty()) {
        // we need to escape the StartHeader, StartText, EndText and EndTransmission bytes
        // , so we can use them to mark the start and end of the header and body
        // we do this by adding an EscapeChar before the byte
        // this way we can differentiate between the bytes that mark the start and end of the header and body

        // get the next header and body from the send channel
        Message data = sendChannel[0];

        // send over debugging serial what we're sending as strings
//        Serial.print("Sending header: ");
//        for (uint8_t b : data.header) {
//            Serial.print((char) b);
//        }
//
//        Serial.print("Sending body: ");
//        for (uint8_t b : data.body) {
//            Serial.print((char) b);
//        }
//
//        Serial.println();

        // set the TX LED to on
        digitalWrite(TX_LED_PIN, HIGH);
        ulong ledOnTime = millis();

        // send the header
        Serial1.write(START_HEADER);
        for (uint8_t b : data.header) {
            if (b == START_HEADER || b == START_TEXT || b == END_TEXT || b == END_TRANSMISSION) {
                Serial1.write(ESCAPE_CHAR);
            }
            Serial1.write(b);
        }
        Serial1.write(START_TEXT);
        for (uint8_t b : data.body) {
            if (b == START_HEADER || b == START_TEXT || b == END_TEXT || b == END_TRANSMISSION) {
                Serial1.write(ESCAPE_CHAR);
            }
            Serial1.write(b);
        }
        Serial1.write(END_TEXT);
        Serial1.write(END_TRANSMISSION);

        if (millis() - ledOnTime < 10) {
            delay(10 - (millis() - ledOnTime));
        }

        // set the TX LED to off
        digitalWrite(TX_LED_PIN, LOW);

        // remove the header and body from the send channel
        sendChannel.erase(sendChannel.begin());
    }

    // if there is data on the receipt channel, process it
    if (!receiveChannel.empty()) {
        // get the next header and body from the receipt channel
        Message data = receiveChannel[0];

        // define a string using strings library

        // process the header and body
        if (data.header == "☺Response") {
            if (data.body == "☺") {
                LEDState = true;
            } else if (data.body == "☹") {
                LEDState = false;
            }
        }


        // remove the header and body from the receipt channel
        receiveChannel.erase(receiveChannel.begin());
    }

    // send a LED -> PI PICO message every 5 seconds
    if (millis() - lastLEDToggleTime > 5000) {
        // add the message to the send channel (header: "led")
        if (LEDState) {
            // turn off
            sendChannel.insert(sendChannel.end(), {"☺", "☹"});
        } else {
            // turn on
            sendChannel.insert(sendChannel.end(), {"☺", "☺"});
        }

        // set the last led toggle time to now
        lastLEDToggleTime = millis();
    }
}

uint8_t utf8Encode(std::string str) {
    uint8_t utf8Char = 0;
    for (uint8_t b : str) {
        utf8Char = utf8Char << 8;
        utf8Char = utf8Char | b;
    }
    return utf8Char;
}