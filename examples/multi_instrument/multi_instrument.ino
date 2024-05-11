#include <Arduino.h>
#include <Audio.h>
#include <sf22aswt.h>

#define USerial Serial

SF22ASWTreader sf22aswt_reader1;
SF22ASWTreader sf22aswt_reader2;
SF22ASWTreader sf22aswt_reader3;

AudioSynthWavetable::instrument_data* sf22aswt_reader1_inst;
AudioSynthWavetable::instrument_data* sf22aswt_reader2_inst;
AudioSynthWavetable::instrument_data* sf22aswt_reader3_inst;

AudioSynthWavetable wavetable1;
AudioSynthWavetable wavetable2;
AudioSynthWavetable wavetable3;
AudioMixer4 mixer;

AudioControlSGTL5000 outputCtrl;

AudioOutputI2S i2sOut;
AudioConnection ac1(wavetable1, 0, mixer, 0);
AudioConnection ac2(wavetable2, 0, mixer, 1);
AudioConnection ac3(wavetable2, 0, mixer, 2);
AudioConnection ac4(mixer, 0, i2sOut, 0);
AudioConnection ac5(mixer, 0, i2sOut, 1);

void usbMidi_NoteOn(byte channel, byte note, byte velocity) {
    if (channel == 0)
        wavetable1.playNote(note, velocity);
    else if (channel == 1)
        wavetable2.playNote(note, velocity);
    else if (channel == 2)
        wavetable3.playNote(note, velocity);
}
void usbMidi_NoteOff(byte channel, byte note, byte velocity) {
    if (channel == 0)
        wavetable1.stop();
    else if (channel == 1)
        wavetable2.stop();
    else if (channel == 2)
        wavetable3.stop();
}

void LoadInstruments()
{
    if (sf22aswt_reader1.ReadFile("gm.sf2") == false)
    {
        USerial.println("Fail to load soundfont file gm.sf2");
        return;
    }
    // this copies all file pointers so that 
    // file read don't to be done on
    // each instance of SF22ASWTreader
    sf22aswt_reader1.CloneInto(sf22aswt_reader2);
    sf22aswt_reader1.CloneInto(sf22aswt_reader3);

    // here we can load three different instruments:

    // As a best practice, it's important to highlight that I utilize wt_inst_old
    // to retain the pointer to the old instrument data.
    // to be able to delete it's data later
    // This precaution is necessary as the wavetable might still rely on the old data. 
    // By doing so, we mitigate the risk of potential crashes.
    // we could off course disable interrupts at this point but as 
    // the Load_instrument_from_open_file takes some time to execute it's best not to
    // note that this practice is only needed if the instruments are to be changed at runtime
    
    
    // instrument 0
    
    AudioSynthWavetable::instrument_data *wt_inst_old = sf22aswt_reader1_inst;
    if (sf22aswt_reader1.Load_instrument(0, sf22aswt_reader1_inst) == true)
    {
        wavetable1.setInstrument(*sf22aswt_reader1_inst);
        if(wt_inst_old != nullptr)
        {
            delete wt_inst_old;
            wt_inst_old = nullptr; // It's a good practice to set deleted pointers to nullptr
        }
    }
    else
    {
        USerial.println("Fail to load instrument 0");
    }

    // instrument 1
    wt_inst_old = sf22aswt_reader2_inst;
    if (sf22aswt_reader2.Load_instrument(1, sf22aswt_reader2_inst) == true)
    {
        wavetable1.setInstrument(*sf22aswt_reader2_inst);
        if(wt_inst_old != nullptr)
        {
            delete wt_inst_old;
            wt_inst_old = nullptr; // It's a good practice to set deleted pointers to nullptr
        }
    }
    else
    {
        USerial.println("Fail to load instrument 1");
    }

    // instrument 2
    wt_inst_old = sf22aswt_reader3_inst;
    if (sf22aswt_reader3.Load_instrument(2, sf22aswt_reader3_inst) == true)
    {
        wavetable1.setInstrument(*sf22aswt_reader3_inst);
        if(wt_inst_old != nullptr)
        {
            delete wt_inst_old;
            wt_inst_old = nullptr; // It's a good practice to set deleted pointers to nullptr
        }
    }
    else
    {
        USerial.println("Fail to load instrument 2");
    }
}

void setup()
{
    AudioMemory(8);
    USerial.begin(115200);
    delay(500); // give host a little extra time to auto reconnect
    outputCtrl.enable();
    outputCtrl.volume(1.0f);

    usbMIDI.setHandleNoteOn(usbMidi_NoteOn);
    usbMIDI.setHandleNoteOff(usbMidi_NoteOff);

    USerial.println("USB serial port initialized!"); // try to see if i can receive this
    if (!SD.begin(BUILTIN_SDCARD)) {
          USerial.print("SD initialization failed!\n");
    }

    LoadInstruments();
}

void loop()
{
    usbMIDI.read();
}