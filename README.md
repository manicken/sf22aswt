# sf2Reader
Sound Font 2 reader for c++ specially designed to use together with Teensy AudioSynthWavetable<br>
<br>
note. currently it's a work in progress<br>
<br>
To use this library:<br>
copy the whole dir SF2 to where you store your main file<br>
<br>
then the usage would be:<br>
```

#include <Arduino.h>
#include "SF2/reader_lazy.h"

AudioSynthWavetable wavetable;
AudioControlSGTL5000 outputCtrl;

AudioOutputI2S i2sOut;
AudioConnection ac1(wavetable, 0, i2sOut, 0);
AudioConnection ac2(wavetable, 0, i2sOut, 1);

AudioSynthWavetable::instrument_data *wt_inst;

setup()
{
    AudioMemory(8);
    outputCtrl.enable();
    outputCtrl.volume(1.0f);

    // copy the old instrument_data pointer so we can delete the used data later
    AudioSynthWavetable::instrument_data *wt_inst_old = wt_inst;

    int instrumentIndex = 0;
    if (SF2::lazy_reader::load_instrument_from_file("filename.sf2", instrumentIndex, &wt_inst) == false)
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

loop()
{

}
```

