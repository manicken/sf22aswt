#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>
#include <ArduinoJson.h>
#include "ledblinker.h"
#include "Mixer128.h"
#include "ExtMemTest.h"
#include <sf22aswt.h>

#ifndef USerial
#define USerial SerialUSB
#endif

#define VOICE_COUNT 128
AudioSynthWavetable wavetable[VOICE_COUNT];
AudioSynthWavetable::instrument_data *wt_inst = nullptr;
int notes[VOICE_COUNT];
bool sustain[VOICE_COUNT];

AudioMixer128 mixer;

AudioControlSGTL5000 outputCtrl;

AudioOutputI2S i2sOut;

AudioConnection voiceConnections[VOICE_COUNT];

AudioConnection toI2s_1(mixer, 0, i2sOut, 0);
AudioConnection toI2s_2(mixer, 0, i2sOut, 1);

void USerialSendAck_OK()
{
    USerial.println("ACK_OK");
}
void USerialSendAck_KO()
{
    USerial.println("ACK_KO");
}
void InitVoices()
{
    // TODO.better mixer for mixin all wavetable outputs
    float mixerGlobalGain = 1.0f/8.0f;//(float)(VOICE_COUNT/4);
    for (int i=0;i<VOICE_COUNT;i++)
    {
        voiceConnections[i].connect(wavetable[i], 0, mixer, i);
        notes[i] = -1; // set to free
        sustain[i] = false;
        mixer.gain(i, mixerGlobalGain);
    }
}
void autoGain()
{
    int numPlaying = 0;
    for (int i=0;i<128;i++)
    {
        if (wavetable[i].isPlaying())
            numPlaying++;
    }
    if (numPlaying == 0) numPlaying = 1;
    float newGain = 1.0f/(float)numPlaying;
    //USerial.println(newGain);
    for (int i=0;i<128;i++)
    {
        mixer.gain(i, newGain);
    }
}
void SetInstrument(const AudioSynthWavetable::instrument_data &inst)
{
    for (int i=0;i<VOICE_COUNT;i++) {
        
        wavetable[i].setInstrument(inst);
    }
}
void activateSustain()
{
    //sustainActive = true;
    for (int i=0;i<VOICE_COUNT;i++) {
        if (notes[i] != -1)
            sustain[i] = true;
    }
}

void deactivateSustain()
{
    for (int i=0;i<VOICE_COUNT;i++) {
        if (notes[i] != -1 && sustain[i] == true) {
            wavetable[i].stop();
            notes[i] = -1;
            sustain[i] = false;
        }
        
    }
    //autoGain();
}


void usbMidi_NoteOn(byte channel, byte note, byte velocity) {
    /*USerial.print("note on: ");
    USerial.print(note);
    USerial.print(", velocity: ");
    USerial.print(velocity); USerial.print("\n");*/
    //waveform.frequency(noteFreqs[note]);
    //for (int i=0;i<VOICE_COUNT;i++){
    //    if (notes[i]==-1) {
            notes[note] = 1;
            wavetable[note].playNote(note, velocity);
            //return;
        //}
    //}
    //autoGain();
    //waveform.amplitude(1.0);
}

void usbMidi_NoteOff(byte channel, byte note, byte velocity) {
    /*USerial.print("note off: ");
    USerial.print(note);
    USerial.print(", velocity: ");
    USerial.print(velocity); USerial.print("\n");*/
    //waveform.amplitude(0);
    //if (sustainActive) return;

    //for (int i=0;i<VOICE_COUNT;i++){
        if (notes[note]==1 && sustain[note] == false) {
            notes[note] = -1;
            wavetable[note].stop();
            //autoGain();
            return;
        }
    //}

}

void usbMidi_ControlChange(byte channel, byte control, byte value) {
    switch (control) { // cases 20-31,102-119 is undefined in midi spec
        case 64:
          if (value == 0)
            deactivateSustain();
          else if (value == 127)
            activateSustain();
          break;
    }
}
void usbMidi_SysEx(const uint8_t *data, uint16_t length, bool complete)
{

}



const int SERIAL_RX_BUFFER_SIZE = 256;
bool cardInitialized = false;

void listFiles(const char *dirname);
void processSerialCommand();

void setup()
{
    
    AudioMemory(1024);
    InitVoices();
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

    usbMIDI.setHandleNoteOn(usbMidi_NoteOn);
    usbMIDI.setHandleNoteOff(usbMidi_NoteOff);
    usbMIDI.setHandleControlChange(usbMidi_ControlChange);
    usbMIDI.setHandleSysEx(usbMidi_SysEx);

    outputCtrl.enable();
    outputCtrl.volume(1.0f);

    USerial.println("setup end"); // try to see if i can receive this
    USerialSendAck_OK();
}

void loop()
{
    ledBlinkTask();
    processSerialCommand();
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

        USerial.print("json:{\"instruments\":[");
        File file = SD.open(SF22ASWT::filePath.c_str());
        file.seek(SF22ASWTreader::sfbk->pdta.inst_position);
        SF22ASWT::inst_rec inst;
        
        for (uint32_t i = 0; i < SF22ASWTreader::sfbk->pdta.inst_count - 1; i++) // -1 the last is allways a EOI
        {
            file.read(&inst, 22);
            USerial.print("{\"name\":\"");
            Helpers::printRawBytesUntil(inst.achInstName, 20, '\0');
            USerial.print("\",\"ndx\":");
            USerial.print(inst.wInstBagNdx);
            USerial.print("},");
        }
        file.close();
        USerial.println("]}");
        long endTime = micros();
        USerial.print("list instruments took: ");
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
        //USerial.print("instrument load presets complete\n");

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
        AudioSynthWavetable::instrument_data *wt_inst_old = wt_inst;

        wt_inst = new AudioSynthWavetable::instrument_data(SF22ASWT::converter::to_AudioSynthWavetable_instrument_data(inst_temp));
        SetInstrument(*wt_inst);
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
        //int instrumentIndex = 0; // TODO make this a parameter
        long startTime = micros();

        // print some debug info:
        USerial.print("trying to load file:"); USerial.println(&serialRxBuffer[26+6]);
        USerial.print("instrument index:"); USerial.println(instrumentIndex);

        // copy the old instrument_data pointer so we can delete the used data later
        AudioSynthWavetable::instrument_data *wt_inst_old = wt_inst;
        
        if (SF22ASWTreader::load_instrument_from_file(&serialRxBuffer[26+6], instrumentIndex, &wt_inst) == false)
        {
            USerial.println("load_first_instrument_from_file error!");
            USerialSendAck_KO();
            return;
        }
        
        SetInstrument(*wt_inst);
        // delete prev inst data if exists
        // Check if wt_inst_old is not nullptr and delete the memory it's pointing to
        if(wt_inst_old != nullptr)
        {
            USerial.println("deleting prev inst.");
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

