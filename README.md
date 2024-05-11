# SF22ASWT (Soundfont2 to AudioSynthWavetable)
Sound Font 2 reader for c++ specially designed to use together with Teensy AudioSynthWavetable<br>
<br>
note. currently it's a work in progress<br>
and future structural changes can occur<br>
<br>
To use this library:<br>
download this repository and<br>
copy the contents of src (all files starting with sf22aswt) to your 'sketch folder' or where you store your main.cpp file<br>
<br>
It's also available at the official Arduino Library with the name sf22aswt
<br>
## Example code<br><br>

[Simple](https://github.com/manicken/sf22aswt/tree/main/examples/simple)  use this as a starting point for your own projects, note that it uses a simplified loading function Load_instrument_from_file which can be found @ sf22aswt_reader_lazy.cpp that function can be used as a starting point for more advanced use cases.<br>
<br>
[Advanced](https://github.com/manicken/sf22aswt/tree/main/examples/advanced)  this is mostly intended to use together with [sf22aswtTester](https://github.com/manicken/sf22aswtTester) which uses serial commands to control the loading of files and instruments, it also have file upload functionality<br>


## Release Notes

### 0.1.0

Initial Release

### 0.1.1

* adds autocheck to use extmem if available
* early check if sample data can fit into ram

### 0.1.2

* moved Errors (enum list) into root namespace SF22ASWT, to make usage shorted
* fix for: Forgot that I could not use FILE_ERROR when !file (not file check) after File open, as it tries to use the file object.
* renamed DEBUG to SF22ASWT_DEBUG to avoid name collisions.
* made the usage of lastErrorStr so that it's only available when SF22ASWT_DEBUG is defined.

### 0.1.3

* hide functions/variables that should not be accessed
* created getters for needed hidden variables.

### 0.1.4

* changed SF22ASWT_SAMPLES_MAX_INTERNAL_RAM_USAGE(define) into SF22ASWT::Samples_Max_Internal_RAM_Cap (variable)
  so that it can easier be changed by the user

### 0.1.5

* add new example: multi_instrument

* add new function: ReaderLazy::Load_instrument
  which can be used for writing easier code, see use case in multi_instrument example

* added optional errPrintStream parameter to both ReaderLazy::Load_instrument_from_file and ReaderLazy::Load_instrument
  which can be used to print the errors to something else than standard Serial output

* add CloneInto function to ReaderLazy class, use case see multi_instrument example
