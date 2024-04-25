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
&nbsp;&nbsp;if (SF2reader::ReadFile(&serialRxBuffer[10]) == false)<br>
&nbsp;&nbsp;{<br>
&nbsp;&nbsp;&nbsp;&nbsp;SF2::printSF2ErrorInfo();<br>
&nbsp;&nbsp;&nbsp;&nbsp;USerialSendAck_KO();<br>
&nbsp;&nbsp;&nbsp;&nbsp;return;<br>
&nbsp;&nbsp;}<br>
&nbsp;&nbsp;SF2::instrument_data_temp inst_temp = {0,0,nullptr};<br>
<br>
&nbsp;&nbsp;if (SF2reader::load_instrument(index, inst_temp) == false)<br>
&nbsp;&nbsp;{<br>
&nbsp;&nbsp;&nbsp;&nbsp;SF2::printSF2ErrorInfo();<br>
&nbsp;&nbsp;&nbsp;&nbsp;USerialSendAck_KO();<br>
&nbsp;&nbsp;&nbsp;&nbsp;return;<br>
&nbsp;&nbsp;}<br>
&nbsp;&nbsp;if (SF2::ReadSampleDataFromFile(inst_temp) == false)<br>
&nbsp;&nbsp;{<br>
&nbsp;&nbsp;&nbsp;&nbsp;SF2::printSF2ErrorInfo();<br>
&nbsp;&nbsp;&nbsp;&nbsp;USerialSendAck_KO();<br>
&nbsp;&nbsp;&nbsp;&nbsp;return;<br>
&nbsp;&nbsp;}<br>
&nbsp;&nbsp;AudioSynthWavetable::instrument_data wt_inst = SF2::converter::to_AudioSynthWavetable_instrument_data(inst_temp);<br>
&nbsp;&nbsp;SetInstrument(wt_inst);<br>
}<br>
setup()<br>
{<br>
&nbsp;&nbsp;sfExample();
}<br>
<br>
loop()<br>
{<br>
<br>
}<br>
<br>

