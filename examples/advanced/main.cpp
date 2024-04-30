#include <Arduino.h>
#include <MIDI.h>
#include "ledblinker.h"
#include "Mixer128.h"
#include "ExtMemTest.h"
#include "WaveTableSynth.h"
#include "SerialFileRx.h"
#include <sf22aswt.h>

#ifndef USerial
#define USerial SerialUSB
#endif



const int SERIAL_RX_BUFFER_SIZE = 256;
bool cardInitialized = false;

void listFiles(const char *dirname);
void processSerialCommand();
void processSerialRx_FileData();

void USerialSendAck_OK(){USerial.println("ACK_OK");}
void USerialSendAck_KO(){USerial.println("ACK_KO");}

void setup()
{
    
    AudioMemory(128);
    WaveTableSynth::Init();
	//Serial.begin(115200);
    USerial.begin(115200);
    delay(500); // give host a little extra time to auto reconnect

    USerial.println("USB serial port initialized!"); // try to see if i can receive this
    //Serial1.begin(115200);
    //Serial2.begin(115200);
    if (!SD.begin(BUILTIN_SDCARD)) {
        USerial.print("SD initialization failed!\n");
    }
    else
    {
        cardInitialized = true;
    }

    USerial.println("setup end"); // try to see if i can receive this
    USerialSendAck_OK();
}

void loop()
{
    ledBlinkTask();
    if (USerial.available() > 0)
    { 
        if (SerialFileRx::inProgress == false) {

            processSerialCommand();
        }
        else
            SerialFileRx::process_FileData();
    }
    else if(SerialFileRx::inProgress) {
        long curr = millis();
        if ((curr - SerialFileRx::lastTimeRxData) > 4000) // rx timeout failsafe
        {
            SerialFileRx::EndRxFile();
            USerial.println("receive file timeout error!");
            USerialSendAck_KO();
        }
    }
    usbMIDI.read();
}


void processSerialCommand()
{
    if (USerial.available() <= 0) return;
    // Read the incoming bytes into a buffer until a newline character is received
    char serialRxBuffer[SERIAL_RX_BUFFER_SIZE];
    int bytesRead = USerial.readBytesUntil('\n', serialRxBuffer, SERIAL_RX_BUFFER_SIZE - 1);
    serialRxBuffer[bytesRead] = '\0'; // Null-terminate the string
    usbMIDI.sendSysEx(bytesRead, (uint8_t*)serialRxBuffer);
    
    if (strncmp(serialRxBuffer, "list_files:", 11) == 0)
    {
        long startTime = micros();
        if (strncmp(&serialRxBuffer[11], "/", 1) == 0)
            listFiles(&serialRxBuffer[12]);
        else
            listFiles("/");

        long endTime = micros();
        USerial.print("list files took: ");
        USerial.print((float)(endTime-startTime)/1000.0f);
        USerial.print(" ms\n");
    }
    else if (strncmp(serialRxBuffer, "read_file:", 10) == 0)
    {
        if (bytesRead <= 11) { USerial.print("read_file path parameter missing\n"); USerialSendAck_KO(); return; }
        
        long startTime = micros();
        if (SF22ASWTreader::ReadFile(&serialRxBuffer[10]) == false)
        {
            SF22ASWT::printSF2ErrorInfo();
            USerialSendAck_KO();
            return;
            // TODO. open and print a part of file contents if possible
            // using lastReadCount and position plus reading some bytes extra backwards
        }
        long endTime = micros();
        USerial.print("open file took: ");
        USerial.print((float)(endTime-startTime)/1000.0f);
        USerial.print(" ms\n");
        USerial.print("json:{'cmd':'file_loaded'}\n");
    }
    else if (strncmp(serialRxBuffer, "print_info", 10) == 0)
    {
        USerial.print("\n*** info ***\nfile size: "); USerial.print(SF22ASWT::fileSize);
        USerial.print(", sfbk size: "); USerial.print(SF22ASWTreader::sfbk->size);
        USerial.print(", info size: "); USerial.print(SF22ASWTreader::sfbk->info_size);
        USerial.print(", sdta size:"); USerial.print(SF22ASWTreader::sfbk->sdta.size);
        USerial.print(", pdta size: "); USerial.print(SF22ASWTreader::sfbk->pdta.size);
        USerial.print("\n");
        USerial.print("inst pos: "); USerial.print(SF22ASWTreader::sfbk->pdta.inst_position); 
        USerial.print(", inst count: "); USerial.println(SF22ASWTreader::sfbk->pdta.inst_count);
        USerial.print("ibag pos: "); USerial.print(SF22ASWTreader::sfbk->pdta.ibag_position); 
        USerial.print(", ibag count: "); USerial.println(SF22ASWTreader::sfbk->pdta.ibag_count);
        USerial.print("igen pos: "); USerial.print(SF22ASWTreader::sfbk->pdta.igen_position); 
        USerial.print(", igen count: "); USerial.println(SF22ASWTreader::sfbk->pdta.igen_count);
        USerial.print("shdr pos: "); USerial.print(SF22ASWTreader::sfbk->pdta.shdr_position); 
        USerial.print(", shdr count: "); USerial.println(SF22ASWTreader::sfbk->pdta.shdr_count);

        SF22ASWT::INFO info;
        File file = SD.open(SF22ASWT::filePath.c_str());
        file.seek(SF22ASWTreader::sfbk->info_position);
        SF22ASWT::readInfoBlock(file, info);
        file.close();
        USerial.println(info.ToString());
        USerialSendAck_OK();
    }
    else if (strncmp(serialRxBuffer, "list_instruments", 16) == 0)
    {
        long startTime = micros();
        if (SF22ASWTreader::lastReadWasOK == false) {
            USerial.println("file not open or last read was not ok");
            USerialSendAck_KO();
            return;
        }
        SF22ASWTreader::printInstrumentListAsJson();
        long endTime = micros();
        USerial.print("list instruments took: ");
        USerial.print((float)(endTime-startTime)/1000.0f);
        USerial.println(" ms");
    }
    else if (strncmp(serialRxBuffer, "list_presets", 12) == 0)
    {
        long startTime = micros();
        if (SF22ASWTreader::lastReadWasOK == false) {
            USerial.println("file not open or last read was not ok");
            USerialSendAck_KO();
            return;
        }
        SF22ASWTreader::printPresetListAsJson();
        long endTime = micros();
        USerial.print("list presets took: ");
        USerial.print((float)(endTime-startTime)/1000.0f);
        USerial.println(" ms");
    }
    else if (strncmp(serialRxBuffer, "load_instrument:", 16) == 0)
    {
        if (bytesRead <= 16) { USerial.print(&serialRxBuffer[16]); USerial.println("\nload_instrument index parameter missing"); USerialSendAck_KO();return; }
        char* endptr;
        uint index = std::strtoul(&serialRxBuffer[16], &endptr, 10);
        if (&serialRxBuffer[16] == endptr) { USerial.println("load_instrument index parameter don't start with digit"); USerialSendAck_KO();return; }
        else if (*endptr != '\0') { USerial.println("load_instrument index parameter non integer characters detected"); USerialSendAck_KO(); return; }

        long startTime = micros();
        
        SF22ASWT::instrument_data_temp inst_temp = {0,0,nullptr};
        if (SF22ASWTreader::load_instrument_data(index, inst_temp) == false)
        {
            SF22ASWT::printSF2ErrorInfo();
            USerialSendAck_KO();
            return;
        }
        //USerial.print("instrument load generator data complete\n");

        //USerial.print(inst_temp.ToString());  USerial.print("\n");

        USerial.print("sample count: "); USerial.println(inst_temp.sample_count);
        long endTime = micros();
        USerial.print("load instrument configuration took: ");
        USerial.print((float)(endTime-startTime)/1000.0f);
        USerial.println(" ms");

        //USerial.print("Start to load sample data from file\n");
        startTime = micros();
        
        if (SF22ASWT::ReadSampleDataFromFile(inst_temp) == false)
        {
            SF22ASWT::printSF2ErrorInfo();
            USerialSendAck_KO();
            return;
        }
        USerial.print("current instrument sample data size inclusive padding: ");
        USerial.print(SF22ASWT::totalSampleDataSizeBytes);
        USerial.println(" bytes");
        
        endTime = micros();
        USerial.print("load instrument sample data took: ");
        USerial.print((float)(endTime-startTime)/1000.0f);
        USerial.println(" ms");
        // copy the old instrument_data pointer so we can delete the used data later
        AudioSynthWavetable::instrument_data *wt_inst_old = WaveTableSynth::wt_inst;

        WaveTableSynth::wt_inst = new AudioSynthWavetable::instrument_data(SF22ASWT::converter::to_AudioSynthWavetable_instrument_data(inst_temp));
        WaveTableSynth::SetInstrument(*WaveTableSynth::wt_inst);
        // delete prev inst data if exists
        // Check if wt_inst_old is not nullptr and delete the memory it's pointing to
        if(wt_inst_old != nullptr)
        {
            USerial.println("deleting prev inst.");
            delete wt_inst_old;
            wt_inst_old = nullptr; // It's a good practice to set deleted pointers to nullptr
        }

        USerial.println("json:{'cmd':'instrument_loaded'}");
    }
    else if (strncmp(serialRxBuffer, "load_instrument_from_file:", 26) == 0)
    {
        // the index is a zeropadded 5 digit number followed by a : (for clarification)
        if (bytesRead <= 27) { USerial.print(&serialRxBuffer[26]); USerial.println("\nload_instrument_from_file index parameter missing"); USerialSendAck_KO();return; }
        char* endptr;
        uint instrumentIndex = std::strtoul(&serialRxBuffer[26], &endptr, 10);
        if (&serialRxBuffer[26] == endptr) { USerial.println("load_instrument_from_file index parameter don't start with digit"); USerialSendAck_KO();return; }
        //else if (*endptr != ':') { USerial.println("load_instrument_from_file  non integer characters detected"); USerialSendAck_KO(); return; }


        if (bytesRead <= (26+6)) { USerial.print("read_file path parameter missing\n"); USerialSendAck_KO(); return; }

        long startTime = micros();

        // print some debug info:
        USerial.print("trying to load file:"); USerial.println(&serialRxBuffer[26+6]);
        USerial.print("instrument index:"); USerial.println(instrumentIndex);

        // copy the old instrument_data pointer so we can delete the used data later
        AudioSynthWavetable::instrument_data *wt_inst_old = WaveTableSynth::wt_inst;
        
        if (SF22ASWTreader::load_instrument_from_file(&serialRxBuffer[26+6], instrumentIndex, &WaveTableSynth::wt_inst) == false)
        {
            USerial.println("load_first_instrument_from_file error!");
            USerialSendAck_KO();
            return;
        }
        
        WaveTableSynth::SetInstrument(*WaveTableSynth::wt_inst);
        // delete prev inst data if exists
        // Check if wt_inst_old is not nullptr and delete the memory it's pointing to
        if(wt_inst_old != nullptr)
        {
            USerial.println("deleting prev instrument data");
            delete[] wt_inst_old->samples;
            delete wt_inst_old;
            wt_inst_old = nullptr; // It's a good practice to set deleted pointers to nullptr
        }
        USerial.println("load_first_instrument_from_file OK");
        long endTime = micros();
        USerial.print("  took: ");
        USerial.print((float)(endTime-startTime)/1000.0f);
        USerial.println(" ms");
        USerial.println("json:{'cmd':'instrument_loaded'}");
    }
    else if (strncmp(serialRxBuffer, "transfer_file:", 14) == 0)
    {
        
        // the file size is a zeropadded 8 digit hex number followed by a :(for clarification)
        if (bytesRead <= (14+9)) { USerial.print("read_file path parameter missing\n"); USerialSendAck_KO(); return; }

        char* endptr;
        uint fileSize = std::strtoul(&serialRxBuffer[14], &endptr, 16);
        if (&serialRxBuffer[14] == endptr) { USerial.println("transfer_file file size parameter don't start with valid hex digit"); USerialSendAck_KO(); return; }
        if (*endptr != ':') { USerial.println("transfer_file : missing after file size parameter"); USerialSendAck_KO(); return; }


        SerialFileRx::StartRxFile(&serialRxBuffer[14+9], fileSize);
        
        
        // debug just send back
        USerial.print("file size:"); USerial.println(fileSize);
        USerial.print("file name:"); USerial.println(&serialRxBuffer[14+9]);

        USerial.println("json:{'cmd':'start_send_file_ack'}");
    }
    else if (strncmp(serialRxBuffer, "delete_file:", 12) == 0)
    {
        if (bytesRead <= 13) { USerial.print("delete_file path parameter missing\n"); USerialSendAck_KO(); return; }

        if (SD.remove(&serialRxBuffer[12]) == false) { USerial.print("could not delete file: "); USerial.println(&serialRxBuffer[12]); USerialSendAck_KO(); return; }
        
        USerial.print("deleted file: "); USerial.print(&serialRxBuffer[12]);
        USerial.println("json:{'cmd':'deleted_file'}");
    }
    else if (strncmp(serialRxBuffer, "exec_ext_mem_test", 17) == 0)
    {
        if (ExtMemTest::exec() == false)
        {
            USerial.println("Ext memory fail");
            USerialSendAck_KO();
        }
        USerialSendAck_OK();
    }
    else if (strncmp(serialRxBuffer, "print_all_errors", 16) == 0)
    {
        SF22ASWT::Error::Test::ExecTest();
        USerialSendAck_OK();
    }
    else if (strncmp(serialRxBuffer, "ping", 4) == 0) // used to auto detect comport
    {
        USerial.println("pong");
    }
    else
    {
        USerial.print("info - command not found:'"); USerial.print(serialRxBuffer); USerial.println("'");
        USerialSendAck_KO();
    }
}

void listFiles(const char *dirname) {
    File root = SD.open(dirname, FILE_READ);
    if (!root) {
        USerial.print("warning - cannot open directory:");
        USerial.println(dirname);
        return;
    }
    if (!root.isDirectory()) {
        USerial.print("warning - Not a directory:");
        USerial.println(dirname);
        return;
    }

    //DSerial.print("Files found in root directory:\n");
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
    USerial.println("]}");
    root.close();
}

