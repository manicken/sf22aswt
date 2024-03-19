#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>
#include "ledblinker.h"
#include "SF2/reader.h"

#include <ArduinoJson.h>
// Define the maximum size of the JSON input
const int JSON_BUFFER_SIZE = 256;
bool cardInitialized = false;

void listFiles(const char *dirname);
void processSerialCommand();

void setup()
{
	//Serial.begin(115200);
    SerialUSB1.begin(115200);
    //Serial1.begin(115200);
    //Serial2.begin(115200);
    if (!SD.begin(BUILTIN_SDCARD)) {
        SerialUSB1.println("SD initialization failed!");
    }
    else
    {
        cardInitialized = true;
    }
}
long lastMs = 0;
void loop()
{
    /*if ((millis() - lastMs) > 1000)
    {
        lastMs = millis();
        SerialUSB1.println("SerialUSB1");
    }*/
    ledBlinkTask();
    processSerialCommand();
}

void processSerialCommand()
{
    if (SerialUSB1.available() > 0) {
        // Read the incoming bytes into a buffer until a newline character is received
        char jsonBuffer[JSON_BUFFER_SIZE];
        int bytesRead = SerialUSB1.readBytesUntil('\n', jsonBuffer, JSON_BUFFER_SIZE - 1);
        jsonBuffer[bytesRead] = '\0'; // Null-terminate the string

        // Parse the JSON string
        StaticJsonDocument<JSON_BUFFER_SIZE> doc;
        DeserializationError error = deserializeJson(doc, jsonBuffer);

        // Check if parsing was successful
        if (error) {
        SerialUSB1.print("{'log':'JSON parsing failed: ");
        SerialUSB1.print(error.c_str());
        SerialUSB1.println("'}");  
        return;
        }

        // Extract command and parameters from the JSON object
        const char* command = doc["cmd"];

        if (strcmp(command, "list_files") == 0)
        {
            if (doc.containsKey("dir")) {
                const char* dir = doc["dir"];
                listFiles(dir);
            }
            else
            {
                listFiles("/");
            }
        }
        else if (strcmp(command, "read_file") == 0)
        {
            String filePath = doc["path"];
            SF2::ReadFile(filePath);
        }
        else
        {
            SerialUSB1.print("{'log':'command not found:'"); SerialUSB1.print(command); SerialUSB1.print("'}\n");
        }
    }
}

void listFiles(const char *dirname) {
    File root = SD.open(dirname);
    if (!root) {
        SerialUSB1.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        SerialUSB1.println("Not a directory");
        return;
    }

    //SerialUSB1.println("Files found in root directory:");
    SerialUSB1.print("{'files':[");

    while (true) {
        File entry = root.openNextFile();
        if (!entry) {
        // no more files
        break;
        }
        SerialUSB1.print("{'name':'");
        SerialUSB1.print(entry.name());
        SerialUSB1.print("','size':");
        if (entry.isDirectory() == false)
            SerialUSB1.print(entry.size());
        else
            SerialUSB1.print(-1); // size -1 mean directory
        SerialUSB1.print("},");
        entry.close();
    }
    SerialUSB1.print("]}\n");
    root.close();
}

