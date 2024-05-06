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
        bool printInstrumentListAsJson(Stream &stream);
        bool printPresetListAsJson(Stream &stream);
        /**
         * this function do only load the sample preset headers for the instrument (soundfont igen data)
         * to load the actual sample data the function <instance name>::ReadSampleDataFromFile should be used
        */
        bool load_instrument_data(uint index, SF22ASWT::instrument_data_temp &inst);
        /**
         * this is mostly intended as a demo or to quickly use this library
        */
        bool load_instrument_from_file(const char * filePath, int instrumentIndex, AudioSynthWavetable::instrument_data **aswt_id);
        bool PrintInfoBlock(Print &stream);

  private:
        bool read_pdta_block(File &file, pdta_rec_lazy &pdta);
        bool fillBagsOfGens(File &file, bag_of_gens* bags, int ibag_startIndex, int ibag_count);
        
    };

// this is just a test
class MemoryStream : public Print {
  private:
    char *buffer;
    size_t bufferSize;
    size_t bufferIndex;

  public:
    MemoryStream(size_t size) {
      buffer = new char[size];
      bufferSize = size;
      bufferIndex = 0;
    }

    ~MemoryStream() {
      delete[] buffer;
    }

    virtual size_t write(uint8_t c) {
      if (bufferIndex < bufferSize - 1) {
        buffer[bufferIndex++] = c;
        buffer[bufferIndex] = '\0'; // Null-terminate the string
        return 1; // Successful write
      } else {
        return 0; // Buffer overflow
      }
    }

    virtual size_t write(const uint8_t *buffer, size_t size) {
      size_t bytesWritten = 0;
      for (size_t i = 0; i < size; ++i) {
        if (write(buffer[i]) == 1) {
          bytesWritten++;
        } else {
          break; // Stop writing on buffer overflow
        }
      }
      return bytesWritten;
    }

    const char *getData() {
      return buffer;
    }

    size_t getSize() {
      return bufferIndex;
    }

    void clear() {
      bufferIndex = 0;
      buffer[0] = '\0';
    }

        
  };
};