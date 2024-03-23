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

#define USerial SerialUSB1

// Define the maximum size of the JSON input
const int JSON_BUFFER_SIZE = 256;
bool cardInitialized = false;

void listFiles(const char *dirname);
void processSerialCommand();

void setup()
{
	//Serial.begin(115200);
    USerial.begin(115200);
    //Serial1.begin(115200);
    //Serial2.begin(115200);
    if (!SD.begin(BUILTIN_SDCARD)) {
        USerial.println("SD initialization failed!");
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
    if (USerial.available() > 0) {
        // Read the incoming bytes into a buffer until a newline character is received
        char jsonBuffer[JSON_BUFFER_SIZE];
        int bytesRead = USerial.readBytesUntil('\n', jsonBuffer, JSON_BUFFER_SIZE - 1);
        jsonBuffer[bytesRead] = '\0'; // Null-terminate the string
        if (strncmp(jsonBuffer, "json:", 5) != 0)
        {
            if (strncmp(jsonBuffer, "ping", 4) == 0)
                USerial.println("pong");
            return;
        }
        // Parse the JSON string
        StaticJsonDocument<JSON_BUFFER_SIZE> doc;
        DeserializationError error = deserializeJson(doc, jsonBuffer+5);

        // Check if parsing was successful
        if (error) {
        USerial.print("error - parsing json failed: ");
        USerial.println(error.c_str());
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
            if (SF2::ReadFile(filePath) == false)
            {
                USerial.print(SF2::lastError);
                USerial.print(" @ position: ");
                USerial.print(SF2::lastErrorPosition);
                USerial.print(", lastReadCount: ");
                USerial.println(SF2::lastReadCount);
                // TODO. open and print a part of file contents if possible
                // using lastReadCount and position plus reading some bytes extra backwards
            }
            //else
            {
                USerial.printf("\ninfo - file size: %u, sfbk size: %u, info size: %u", SF2::sfFile.size, SF2::sfFile.sfbk.size, SF2::sfFile.sfbk.info.size); USerial.print(", sdta size:"); USerial.print(SF2::sfFile.sfbk.sdta.size); USerial.print(", pdta size: "); USerial.println(SF2::sfFile.sfbk.pdta.size);
                USerial.println(SF2::sfFile.sfbk.info.ToString());
            }
        }
        else if (strcmp(command, "list_instruments") == 0)
        {
            USerial.printf("Instrument count: %ld\n\n", SF2::sfFile.sfbk.pdta.inst_count);

            for (uint32_t i = 0; i < SF2::sfFile.sfbk.pdta.inst_count; i++)
            {
                SF2::printRawBytes(SF2::sfFile.sfbk.pdta.inst[i].achInstName, 20);
                USerial.println("\n");
            }
        }
        else if (strcmp(command, "ping") == 0)
        {
            USerial.println("pong");
        }
        else
        {
            USerial.print("info - command not found:'"); USerial.println(command);
        }
    }
}

void listFiles(const char *dirname) {
    File root = SD.open(dirname, FILE_READ);
    if (!root) {
        USerial.println("warning - cannot open directory");
        USerial.println(dirname);
        return;
    }
    if (!root.isDirectory()) {
        USerial.println("warning - Not a directory");
        return;
    }

    //DSerial.println("Files found in root directory:");
    USerial.print("json:{'files':[");

    while (true) {
        File entry = root.openNextFile(FILE_READ);
        if (!entry) {
        // no more files
        break;
        }
        USerial.print("{'name':'");
        USerial.print(entry.name());
        USerial.print("','size':");
        if (entry.isDirectory() == false)
            USerial.print(entry.size());
        else
            USerial.print(-1); // size -1 mean directory
        USerial.print("},");
        entry.close();
    }
    USerial.print("]}\n");
    root.close();
}

