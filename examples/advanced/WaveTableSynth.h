#pragma once

#include <Arduino.h>
#include <Audio.h>
#include "Mixer128.h"

namespace WaveTableSynth
{

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

    void Init()
    {
        InitVoices();
        usbMIDI.setHandleNoteOn(usbMidi_NoteOn);
        usbMIDI.setHandleNoteOff(usbMidi_NoteOff);
        usbMIDI.setHandleControlChange(usbMidi_ControlChange);
        usbMIDI.setHandleSysEx(usbMidi_SysEx);

        outputCtrl.enable();
        outputCtrl.volume(1.0f);
    }
}