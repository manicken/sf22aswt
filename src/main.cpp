#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>
#include "ledblinker.h"


#include "SF2/helpers.h"
#include "SF2/common.h"
#include "SF2/converter.h"

#include <ArduinoJson.h>

#define USerial SerialUSB1

//#include "SF2/reader.h"
//#define SF2reader SF2::reader
#include "SF2/reader_lazy.h"
#define SF2reader SF2::lazy_reader

AudioSynthWavetable wavetable;
//AudioOutputUSB usb;




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
            long startTime = micros();
            if (doc.containsKey("dir")) {
                const char* dir = doc["dir"];
                listFiles(dir);
            }
            else
            {
                listFiles("/");
            }
            long endTime = micros();
            USerial.print("list files took: ");
            USerial.print(endTime-startTime);
            USerial.println(" microseconds");
        }
        else if (strcmp(command, "read_file") == 0)
        {
            long startTime = micros();
            if (doc.containsKey("path") == false) { USerial.println("read_file path parameter missing"); return; }

            String filePath = doc["path"];
            if (SF2reader::ReadFile(filePath) == false)
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
                
                USerial.print("\n*** info ***\nfile size: "); USerial.print(SF2::fileSize);
                USerial.print(", sfbk size: "); USerial.print(SF2reader::sfbk->size);
                USerial.print(", info size: "); USerial.print(SF2reader::sfbk->info_size);
                USerial.print(", sdta size:"); USerial.print(SF2reader::sfbk->sdta.size);
                USerial.print(", pdta size: "); USerial.println(SF2reader::sfbk->pdta.size);

                USerial.print("inst pos: "); USerial.print(SF2reader::sfbk->pdta.inst_position); USerial.print(", inst count: "); USerial.println(SF2reader::sfbk->pdta.inst_count);
                USerial.print("ibag pos: "); USerial.print(SF2reader::sfbk->pdta.ibag_position); USerial.print(", ibag count: "); USerial.println(SF2reader::sfbk->pdta.ibag_count);
                USerial.print("igen pos: "); USerial.print(SF2reader::sfbk->pdta.igen_position); USerial.print(", igen count: "); USerial.println(SF2reader::sfbk->pdta.igen_count);
                USerial.print("shdr pos: "); USerial.print(SF2reader::sfbk->pdta.shdr_position); USerial.print(", shdr count: "); USerial.println(SF2reader::sfbk->pdta.shdr_count);

                SF2::INFO info;
                File file = SD.open(SF2::filePath.c_str());
                file.seek(SF2reader::sfbk->info_position);
                SF2::readInfoBlock(file, info);
                file.close();
                USerial.println(info.ToString());
            }
            long endTime = micros();
            USerial.print("open file took: ");
            USerial.print(endTime-startTime);
            USerial.println(" microseconds");
        }
        else if (strcmp(command, "list_instruments") == 0)
        {
            long startTime = micros();
            if (SF2reader::lastReadWasOK == false) { USerial.println("file not open or last read was not ok"); return; }

            USerial.print("json:{\"instruments\":[");//, SF2reader::sfbk->pdta.inst_count);
            File file = SD.open(SF2::filePath.c_str());
            file.seek(SF2reader::sfbk->pdta.inst_position);
            SF2::inst_rec inst;
            
            for (uint32_t i = 0; i < SF2reader::sfbk->pdta.inst_count - 1; i++) // -1 the last is allways a EOI
            {
                
                file.read(&inst, 22);
                //file.read(&inst.wInstBagNdx, 2);
                //Helpers::printRawBytes(SF2reader::sfbk->pdta.inst[i].achInstName, 20);
                USerial.print("{\"name\":\"");
                Helpers::printRawBytesUntil(inst.achInstName, 20, '\0');
                USerial.print("\",\"ndx\":");
                USerial.print(inst.wInstBagNdx);
                USerial.print("},");
            }
            file.close();
            USerial.print("]}\n");
            long endTime = micros();
            USerial.print("list instruments took: ");
            USerial.print(endTime-startTime);
            USerial.println(" microseconds");
        }
        else if (strcmp(command, "load_instrument") == 0)
        {
            long startTime = micros();
            if (doc.containsKey("index") == false) {USerial.println("load_instrument index parameter missing");}
            int index = doc["index"];
            SF2::instrument_data_temp inst_temp = {0,0,nullptr};
            
            SF2reader::load_instrument(index, inst_temp);

            //USerial.println(inst_temp.ToString());

            USerial.print("sample count: "); USerial.println(inst_temp.sample_count);
            long endTime = micros();
            USerial.print("load instrument took: ");
            USerial.print(endTime-startTime);
            USerial.println(" microseconds");

            USerial.println("Start to load sample data from file");
            startTime = micros();
            //SF2::instrument_data inst_final;
            SF2::lazy_reader::ReadSampleDataFromFile(inst_temp);
            //SF2::converter::toFinal(inst_temp, inst_final);
            endTime = micros();
            USerial.print("load instrument sample data took: ");
            USerial.print(endTime-startTime);
            USerial.println(" microseconds");

            //AudioSynthWavetable::instrument_data wt_inst = SF2::converter::to_AudioSynthWavetable_instrument_data(inst_final);
            // the following is for later
            //wavetable.setInstrument(wt_inst);

            
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

