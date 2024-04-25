# sf2Reader
Sound Font 2 reader for c++ specially designed to use together with Teensy AudioSynthWavetable<br>
<br>
note. currently it's a work in progress<br>
<br>
To use this library:<br>
copy the whole dir SF2 to where you store your main file<br>
<br>
then the usage would be:<br>
<br>
void sfExample()<br>
{<br>
  if (SF2reader::ReadFile(&serialRxBuffer[10]) == false)<br>
  {<br>
    SF2::printSF2ErrorInfo();<br>
    USerialSendAck_KO();<br>
    return;<br>
    // TODO. open and print a part of file contents if possible<br>
    // using lastReadCount and position plus reading some bytes extra backwards<br>
  }<br>
  SF2::instrument_data_temp inst_temp = {0,0,nullptr};<br>
        <br>
        if (SF2reader::load_instrument(index, inst_temp) == false)<br>
        {<br>
            SF2::printSF2ErrorInfo();<br>
            USerialSendAck_KO();<br>
            return;<br>
        }<br>
        if (SF2::ReadSampleDataFromFile(inst_temp) == false)<br>
        {<br>
            SF2::printSF2ErrorInfo();<br>
            USerialSendAck_KO();<br>
            return;<br>
        }<br>
        AudioSynthWavetable::instrument_data wt_inst = SF2::converter::to_AudioSynthWavetable_instrument_data(inst_temp);<br>
        SetInstrument(wt_inst);<br>
}<br>
setup()<br>
{<br>
  sfExample();
}<br>
<br>
loop()<br>
{<br>
<br>
}<br>
<br>

