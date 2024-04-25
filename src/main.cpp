#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>
#include "ledblinker.h"
#include "Mixer128.h"

#include "SF2/helpers.h"
#include "SF2/common.h"
#include "SF2/converter.h"
#include "SF2/error_enums.h"

#include "ExtMemTest.h"

#include <ArduinoJson.h>

const float noteFreqs[128] = {8.176, 8.662, 9.177, 9.723, 10.301, 10.913, 11.562, 12.25, 12.978, 13.75, 14.568, 15.434, 16.352, 17.324, 18.354, 19.445, 20.602, 21.827, 23.125, 24.5, 25.957, 27.5, 29.135, 30.868, 32.703, 34.648, 36.708, 38.891, 41.203, 43.654, 46.249, 48.999, 51.913, 55, 58.27, 61.735, 65.406, 69.296, 73.416, 77.782, 82.407, 87.307, 92.499, 97.999, 103.826, 110, 116.541, 123.471, 130.813, 138.591, 146.832, 155.563, 164.814, 174.614, 184.997, 195.998, 207.652, 220, 233.082, 246.942, 261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440, 466.164, 493.883, 523.251, 554.365, 587.33, 622.254, 659.255, 698.456, 739.989, 783.991, 830.609, 880, 932.328, 987.767, 1046.502, 1108.731, 1174.659, 1244.508, 1318.51, 1396.913, 1479.978, 1567.982, 1661.219, 1760, 1864.655, 1975.533, 2093.005, 2217.461, 2349.318, 2489.016, 2637.02, 2793.826, 2959.955, 3135.963, 3322.438, 3520, 3729.31, 3951.066, 4186.009, 4434.922, 4698.636, 4978.032, 5274.041, 5587.652, 5919.911, 6271.927, 6644.875, 7040, 7458.62, 7902.133, 8372.018, 8869.844, 9397.273, 9956.063, 10548.08, 11175.3, 11839.82, 12543.85};


#define USerial SerialUSB

//#include "SF2/reader.h"
//#define SF2reader SF2::reader
#include "SF2/reader_lazy.h"
#define SF2reader SF2::lazy_reader
#define VOICE_COUNT 128

AudioSynthWavetable wavetable[VOICE_COUNT];
int notes[VOICE_COUNT];
bool sustain[VOICE_COUNT];
//bool sustainActive=false;
//AudioSynthWaveform waveform;

AudioMixer128 mixer;

AudioControlSGTL5000             outputCtrl;

//AudioOutputUSB usbOut;
AudioOutputI2S i2sOut;
/*
AudioConnection ac(waveform, 0, usbOut, 0);
AudioConnection ac2(waveform, 0, usbOut, 1);

AudioConnection ac3(waveform, 0, i2sOut, 0);
AudioConnection ac4(waveform, 0, i2sOut, 1);
*/
AudioConnection voiceConnections[VOICE_COUNT];
//AudioConnection toMix_1(wavetable[0], 0, mixer, 0);
//AudioConnection toMix_2(wavetable[1], 0, mixer, 1);
//AudioConnection toMix_3(wavetable[2], 0, mixer, 2);
//AudioConnection toMix_4(wavetable[3], 0, mixer, 3);
//AudioConnection toUsb_1(mixer, 0, usbOut, 0);
//AudioConnection toUsb_2(mixer, 0, usbOut, 1);

AudioConnection toI2s_1(mixer, 0, i2sOut, 0);
AudioConnection toI2s_2(mixer, 0, i2sOut, 1);
//AudioOutputUSB usb;

// the following are gonna be used when 
// this project is converted into using
// a single serial port only
// so that usb_desc don't need to be changed
// when the client app rx theese it will close the comport
// so the programmer can access it
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
    for (int i=0;i<VOICE_COUNT;i++)
        wavetable[i].setInstrument(inst);
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
    //sustainActive = false;
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
    

    //waveform.begin(0, 50, WAVEFORM_SQUARE);
    outputCtrl.enable();
        outputCtrl.volume(1.0f);

    USerial.println("setup end"); // try to see if i can receive this
    USerialSendAck_OK();
}
long lastMs = 0;
void loop()
{
    /*if ((millis() - lastMs) > 1000)
    {
        lastMs = millis();
        SerialUSB1.print("SerialUSB1\n");
    }*/
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
        if (SF2reader::ReadFile(&serialRxBuffer[10]) == false)
        {
            SF2::printSF2ErrorInfo();
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
        USerial.print("\n*** info ***\nfile size: "); USerial.print(SF2::fileSize);
        USerial.print(", sfbk size: "); USerial.print(SF2reader::sfbk->size);
        USerial.print(", info size: "); USerial.print(SF2reader::sfbk->info_size);
        USerial.print(", sdta size:"); USerial.print(SF2reader::sfbk->sdta.size);
        USerial.print(", pdta size: "); USerial.print(SF2reader::sfbk->pdta.size);
        USerial.print("\n");
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
        USerialSendAck_OK();
    }
    else if (strncmp(serialRxBuffer, "list_instruments", 16) == 0)
    {
        long startTime = micros();
        if (SF2reader::lastReadWasOK == false) {
            USerial.println("file not open or last read was not ok");
            USerialSendAck_KO();
            return;
        }

        USerial.print("json:{\"instruments\":[");
        File file = SD.open(SF2::filePath.c_str());
        file.seek(SF2reader::sfbk->pdta.inst_position);
        SF2::inst_rec inst;
        
        for (uint32_t i = 0; i < SF2reader::sfbk->pdta.inst_count - 1; i++) // -1 the last is allways a EOI
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
        
        SF2::instrument_data_temp inst_temp = {0,0,nullptr};
        
        if (SF2reader::load_instrument(index, inst_temp) == false)
        {
            SF2::printSF2ErrorInfo();
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
        
        if (SF2::ReadSampleDataFromFile(inst_temp) == false)
        {
            SF2::printSF2ErrorInfo();
            USerialSendAck_KO();
            return;
        }
        USerial.print("current instrument sample data size inclusive padding: ");
        USerial.print(SF2::totalSampleDataSizeBytes);
        USerial.println(" bytes");
        
        endTime = micros();
        USerial.print("load instrument sample data took: ");
        USerial.print((float)(endTime-startTime)/1000.0f);
        USerial.println(" ms");

        AudioSynthWavetable::instrument_data wt_inst = SF2::converter::to_AudioSynthWavetable_instrument_data(inst_temp);
        SetInstrument(wt_inst);

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
        SF2::Error::Test::ExecTest();
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

