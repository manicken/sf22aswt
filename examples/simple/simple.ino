#include <Arduino.h>
#include <sf22aswt.h>

AudioSynthWavetable wavetable;
AudioControlSGTL5000 outputCtrl;

AudioOutputI2S i2sOut;
AudioConnection ac1(wavetable, 0, i2sOut, 0);
AudioConnection ac2(wavetable, 0, i2sOut, 1);

AudioSynthWavetable::instrument_data *wt_inst;
SF22ASWTreader sf22aswt;

void usbMidi_NoteOn(byte channel, byte note, byte velocity) {
  wavetable.playNote(note, velocity);
}
void usbMidi_NoteOff(byte channel, byte note, byte velocity) {
  wavetable.stop();
}
void setup()
{
    AudioMemory(8);
    Serial.begin(115200);
    delay(500); // give host a little extra time to auto reconnect
    outputCtrl.enable();
    outputCtrl.volume(1.0f);

    usbMIDI.setHandleNoteOn(usbMidi_NoteOn);
    usbMIDI.setHandleNoteOff(usbMidi_NoteOff);

Serial.println("USB serial port initialized!"); // try to see if i can receive this
    if (!SD.begin(BUILTIN_SDCARD)) {
          Serial.print("SD initialization failed!\n");
    }

    // copy the old instrument_data pointer so we can delete the used data later
    AudioSynthWavetable::instrument_data *wt_inst_old = wt_inst;

    int instrumentIndex = 0;
    if (sf22aswt.load_instrument_from_file("gm.sf2", instrumentIndex, &wt_inst) == false)
    {
        Serial.println("load_instrument_from_file error!");
    }
    else
    {
        wavetable.setInstrument(*wt_inst);

        // delete prev inst data if exists
        // Check if aswt_id is not nullptr and delete the memory it's pointing to
        if(wt_inst_old != nullptr)
        {
            delete wt_inst_old;
            wt_inst_old = nullptr; // It's a good practice to set deleted pointers to nullptr
        }
        Serial.println("load_instrument_from_file OK!");
    }
    
}

void loop()
{
  usbMIDI.read();
}
