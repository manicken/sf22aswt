#pragma once

#include <Arduino.h>
#include <SD.h>

#include "sf22aswt_reader_base.h"
#include "sf22aswt_structures.h"
#include "sf22aswt_enums.h"
#include "sf22aswt_helpers.h"
#include "sf22aswt_converter.h"

namespace SF22ASWT
{
    class ReaderLazy : public SF22ASWT::ReaderBase
    {
      public:
        sfbk_rec_lazy sfbk;

        /** reads and verifies the sf2 file,
         *  note. this is lazy read 
         *  and only the file data position for
         *  all used blocks are stored into ram
         */
        bool ReadFile(const char * filePath);
        bool PrintInstrumentListAsJson(Print &printStream);
        bool PrintPresetListAsJson(Print &printStream);
        /**
         * this function do only load the sample preset headers for the instrument (soundfont igen data)
         * to load the actual sample data the function <instance name>::ReadSampleDataFromFile should be used
        */
        bool Load_instrument_data(uint index, SF22ASWT::instrument_data_temp &inst);
        /**
         * this is mostly intended as a demo or to quickly use this library
        */
        bool Load_instrument_from_file(const char * filePath, int instrumentIndex, AudioSynthWavetable::instrument_data **aswt_id);
        bool PrintInfoBlock(Print &printStream);

  private:
        bool read_pdta_block(File &file, pdta_rec_lazy &pdta);
        bool fillBagsOfGens(File &file, bag_of_gens* bags, int ibag_startIndex, int ibag_count);
        
    };


};