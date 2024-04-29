#pragma once

#include <Arduino.h>
#include <SD.h>

#include "sf22aswt_common.h"
#include "sf22aswt_structures.h"
#include "sf22aswt_enums.h"
#include "sf22aswt_helpers.h"
#include "sf22aswt_converter.h"
#include "sf22aswt_reader_lazy_blocks.h"
#include "sf22aswt_reader_lazy_gen_get.h"

namespace SF22ASWT::reader_lazy
{
    sfbk_rec_lazy *sfbk; // declare it as a pointer so that it's not allocated if not in use
    bool lastReadWasOK = false;

    /** reads and verifies the sf2 file,
     *  note. this is lazy read 
     *  and only the file positions for
     *  all important blocks are stored into ram
     */
    bool ReadFile(const char * filePath)
    {
        lastReadWasOK = false;
        clearErrors();
        if (sfbk == nullptr) sfbk = new sfbk_rec_lazy();

        File file = SD.open(filePath);
        if (!file) {  lastError = Error::Errors::FILE_NOT_OPEN; return false; }
        fileSize = file.size();

        char fourCC[4];

        if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR(FILE_FOURCC_READ) //("read error - while reading fileTag")
        if (verifyFourCC(fourCC) == false) FILE_ERROR(FILE_FOURCC_INVALID) //("error - invalid fileTag")
        if (strncmp(fourCC, "RIFF", 4) != 0) FILE_ERROR(FILE_FOURCC_MISMATCH) //("error - not a RIFF fileformat")
        
        if ((lastReadCount = file.read(&sfbk->size, 4)) != 4) FILE_ERROR(RIFF_SIZE_READ) //("read error - while reading RIFF size")
        if (fileSize != (sfbk->size + 8)) FILE_ERROR(RIFF_SIZE_MISMATCH) //("error - fileSize mismatch")

        if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR(RIFF_FOURCC_READ) //("read error - while reading fileformat")
        if (verifyFourCC(fourCC) == false) FILE_ERROR(RIFF_FOURCC_INVALID) //("error - invalid fileformat")
        
        if (strncmp(fourCC, "sfbk", 4) != 0) FILE_ERROR(RIFF_FOURCC_MISMATCH) //("error - not a sfbk fileformat")

        char listTag[4];
        uint32_t listSize = 0;
        while (file.available() > 0)
        {
            // every block starts with a LIST tag
            if ((lastReadCount = file.readBytes(listTag, 4)) != 4) FILE_ERROR(LIST_FOURCC_READ) //("read error - while reading listTag")
            if (verifyFourCC(listTag) == false) FILE_ERROR(LIST_FOURCC_INVALID) //("error - listTag invalid")
            if (strncmp(listTag, "LIST", 4) != 0) FILE_ERROR(LIST_FOURCC_MISMATCH) //("error - listTag is not LIST")

            
            if ((lastReadCount = file.read(&listSize, 4)) != 4) FILE_ERROR(LIST_SIZE_READ) //("read error - while getting listSize")

            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR(LISTTYPE_FOURCC_READ) //("read error - while reading listType")
            DebugPrintFOURCC(fourCC);
            DebugPrintFOURCC_size(listSize);
            if (verifyFourCC(fourCC) == false) FILE_ERROR(LISTTYPE_FOURCC_INVALID) //("error - invalid listType")
            
            
            if (strncmp(fourCC, "INFO", 4) == 0)
            {
                sfbk->info_position = file.position(); // normally don't read info chunk to save ram
                sfbk->info_size = listSize;
                if (file.seek(listSize - 4, SeekCur) == false) FILE_SEEK_ERROR(INFO_DATA_SKIP, listSize - 4) //("seek error - while skipping INFO block")
                
                //file.close(); return true; // early return debug test
            }
            else if (strncmp(fourCC, "sdta", 4) == 0)
            {
                sfbk->sdta.size = listSize;
                if (blocks::read_sdta_block(file, sfbk->sdta) == false) return false;
                //file.close(); return true; // early return debug test
            }
            else if (strncmp(fourCC, "pdta", 4) == 0)
            {
                sfbk->pdta.size = listSize;
                if (blocks::read_pdta_block(file, sfbk->pdta) == false) return false;
                //file.close(); return true; // early return debug test
            }
            else
            {
                // normally unknown blocks should be ignored
                if (file.seek(listSize - 4, SeekCur) == false) FILE_SEEK_ERROR(LIST_UNKNOWN_BLOCK_DATA_SKIP, listSize - 4) //("seek error - while skipping unknown sfbk root block")
            }
        }

        file.close();
        lastReadWasOK = true;
        SF22ASWT::filePath = filePath;
        return true;
    }

    /**
     * this function do only load the sample preset headers for the instrument
     * to load the actual sample data the function SF22ASWT::ReadSampleDataFromFile
     * @'common.h' should be used
    */
    bool load_instrument_data(uint index, SF22ASWT::instrument_data_temp &inst)
    {
        clearErrors();

        if (index > sfbk->pdta.inst_count - 1){ 
            //lastError = "load instrument index out of range";
            lastError = Error::Errors::FUNCTION_LOAD_INST_INDEX_RANGE;
            return false;
        }

        File file = SD.open(SF22ASWT::filePath.c_str());
        uint64_t seekPos = sfbk->pdta.inst_position + inst_rec::Size*index + 20;
        if (file.seek(seekPos) == false) FILE_SEEK_ERROR(PDTA_INST_DATA_SEEK, seekPos)
        uint16_t ibag_startIndex = 0;
        uint16_t ibag_endIndex = 0;
        if ((lastReadCount = file.read(&ibag_startIndex, 2)) != 2) FILE_ERROR(PDTA_INST_DATA_READ)
        // skipping next inst name
        if (file.seek(20, SeekCur) == false) FILE_SEEK_ERROR(PDTA_INST_DATA_SKIP, 20)
        if ((lastReadCount = file.read(&ibag_endIndex, 2)) != 2) FILE_ERROR(PDTA_INST_DATA_READ)
        
        DebugPrint_Text_Var("\nibag_start index: ", ibag_startIndex);
        DebugPrintln_Text_Var(", ibag_end index: ", ibag_endIndex);
        DebugPrint("\n");
        seekPos = sfbk->pdta.ibag_position + bag_rec::Size*ibag_startIndex;
        if (file.seek(seekPos) == false) FILE_SEEK_ERROR(PDTA_IBAG_DATA_SEEK, seekPos) //seek error to ibags
        DebugPrint("igen_ndxs: ");
        uint16_t ibag_count = ibag_endIndex - ibag_startIndex; 
        uint16_t igen_ndxs[ibag_count+1]; // +1 because of the soundfont structure 
        uint16_t dummy = 0;
        for (int i=0;i<ibag_count+1;i++)
        {
            if ((lastReadCount = file.read(&igen_ndxs[i], 2)) != 2) FILE_ERROR(PDTA_IBAG_DATA_READ) //read error - while reading &igen_ndxs[i]
            if ((lastReadCount = file.read(&dummy, 2)) != 2) FILE_ERROR(PDTA_IBAG_DATA_SKIP) //read error - while reading dummy
            DebugPrint(igen_ndxs[i]);
            DebugPrint(", ");
        }
        DebugPrint("\n");
        // store gen data in bags for faster access
        bag_of_gens bags[ibag_count];

        // search to the location for the first igen record
        seekPos = sfbk->pdta.igen_position + igen_ndxs[0]*gen_rec::Size;
        if (file.seek(seekPos) == false) FILE_SEEK_ERROR(PDTA_IGEN_DATA_SEEK, seekPos) //seek error to first igen record
            
        for (int i=0;i<ibag_count;i++)
        {
            uint16_t start = igen_ndxs[i];
            uint16_t end = igen_ndxs[i+1];
            uint16_t count = end-start;
            bags[i].items = new gen_rec[count];
            bags[i].count = count;

            if ((lastReadCount = file.read(bags[i].items, gen_rec::Size*count)) != gen_rec::Size*count) FILE_ERROR(PDTA_IGEN_DATA_READ)
            
            DebugPrint("bag contents:\n");
#ifdef DEBUG
            for (int i2=0;i2<count;i2++)
            {
                DebugPrint_Text_Var("  sfGenOper:", (uint16_t)bags[i].items[i2].sfGenOper);
                DebugPrintln_Text_Var(", value:", bags[i].items[i2].genAmount.UAmount);
            }
#endif
            
        }
        DebugPrint("temp storage in bags complete\n");

        // if the first zone ends with a sampleID gen type then there is not any global zone for that instrument
        bool globalExists = (bags[0].count != 0)?(bags[0].lastItem().sfGenOper != SFGenerator::sampleID):true;

        inst.sample_count = globalExists?(ibag_count - 1):ibag_count;

        inst.sample_note_ranges = new uint8_t[inst.sample_count];
        inst.samples = new sample_header_temp[inst.sample_count];
        DebugPrint("\nsample count: "); DebugPrint(inst.sample_count);
        for (int si=0;si<inst.sample_count;si++)
        {
            shdr_rec shdr;
            DebugPrintln_Text_Var("getting sample x: ", si);
            if (gen_get::sample_header(file, *sfbk, bags, si, &shdr) == false) { 
                break; // classify the file as structually unsound
                //inst.samples[si].invalid = true;
                DebugPrintln_Text_Var("error - while getting sample header @ ", si);
                //continue;
            }
            DebugPrint("sample name: ");
#ifdef DEBUG
            Helpers::printRawBytesSanitizedUntil(shdr.achSampleName, 20, '\0');
#endif
            DebugPrintln();
            DebugPrintln("getting data:");
            inst.sample_note_ranges[si] = gen_get::key_range_end(bags, si);
            inst.samples[si].sample_start = shdr.dwStart*2 + sfbk->sdta.smpl.position;
            inst.samples[si].LOOP = gen_get::sample_repeat(bags, si, false);
            inst.samples[si].SAMPLE_NOTE = gen_get::sample_note(bags, si, shdr);
            inst.samples[si].CENTS_OFFSET = gen_get::fine_tuning(bags, si);
            inst.samples[si].LENGTH = gen_get::length(bags, si, shdr);
            inst.samples[si].LENGTH_BITS = gen_get::length_bits(inst.samples[si].LENGTH);
            inst.samples[si].SAMPLE_RATE = shdr.dwSampleRate;
            inst.samples[si].LOOP_START = gen_get::cooked_loop_start(bags, si, shdr);
            inst.samples[si].LOOP_END = gen_get::cooked_loop_end(bags, si, shdr);
            inst.samples[si].INIT_ATTENUATION = gen_get::decibel_value(bags, si, SFGenerator::initialAttenuation, 0, 0, 144) * -1;
            DebugPrintln("getting vol env");
            // VOLUME ENVELOPE VALUES
            inst.samples[si].DELAY_ENV = gen_get::timecents_value(bags, si, SFGenerator::delayVolEnv, 0, 0);
            inst.samples[si].ATTACK_ENV = gen_get::timecents_value(bags, si, SFGenerator::attackVolEnv, 1, 1);
            inst.samples[si].HOLD_ENV = gen_get::timecents_value(bags, si, SFGenerator::holdVolEnv, 0, 0);
            inst.samples[si].DECAY_ENV = gen_get::timecents_value(bags, si, SFGenerator::decayVolEnv, 1, 1);
            inst.samples[si].RELEASE_ENV = gen_get::timecents_value(bags, si, SFGenerator::releaseVolEnv, 1, 1);
            inst.samples[si].SUSTAIN_FRAC = gen_get::decibel_value(bags, si, SFGenerator::sustainVolEnv, 0, 0, 144) * -1;
            DebugPrintln("getting vib vals");
            // VIRBRATO VALUES
            inst.samples[si].VIB_DELAY_ENV = gen_get::timecents_value(bags, si, SFGenerator::delayVibLFO, 0, 0);
            inst.samples[si].VIB_INC_ENV = gen_get::hertz(bags, si, SFGenerator::freqVibLFO, 8.176, 0.1, 100);
            inst.samples[si].VIB_PITCH_INIT = gen_get::pitch_cents(bags, si, SFGenerator::vibLfoToPitch, 0, -12000, 12000);
            inst.samples[si].VIB_PITCH_SCND = inst.samples[si].VIB_PITCH_INIT * -1; //gen_get::pitch_cents(bags, si, SFGenerator::vibLfoToPitch, 0, -12000, 12000) * -1;
            DebugPrintln("getting mod vals");
            // MODULATION VALUES
            inst.samples[si].MOD_DELAY_ENV = gen_get::timecents_value(bags, si, SFGenerator::delayModLFO, 0, 0);
            inst.samples[si].MOD_INC_ENV = gen_get::hertz(bags, si, SFGenerator::freqModLFO, 8.176, 0.1, 100);
            inst.samples[si].MOD_PITCH_INIT = gen_get::pitch_cents(bags, si, SFGenerator::modLfoToPitch, 0, -12000, 12000);
            inst.samples[si].MOD_PITCH_SCND = inst.samples[si].MOD_PITCH_INIT * -1; //gen_get::pitch_cents(bags, si, SFGenerator::modLfoToPitch, 0, -12000, 12000) * -1;
            inst.samples[si].MOD_AMP_INIT_GAIN = gen_get::decibel_value(bags, si, SFGenerator::modLfoToVolume, 0, -96, 96);
            inst.samples[si].MOD_AMP_SCND_GAIN = inst.samples[si].MOD_AMP_INIT_GAIN * -1; //gen_get::decibel_value(bags, si, SFGenerator::modLfoToVolume, 0, -96, 96) * -1;
        }

        // Deallocate memory for bags_of_gens
        for (int i = 0; i < ibag_count; i++) {
            delete[] bags[i].items; // Deallocate memory for the array of pointers
        }
        
        file.close();
        return true;
    }

    /**
     * this is mostly intended as a demo or to quickly use this library
    */
    bool load_instrument_from_file(const char * filePath, int instrumentIndex, AudioSynthWavetable::instrument_data **aswt_id)
    {
        
        if (ReadFile(filePath) == false)
        {
            USerial.println("Read file error:");
            SF22ASWT::printSF2ErrorInfo();
            return false;
        }
        SF22ASWT::instrument_data_temp inst_temp = {0,0,nullptr};

        if (load_instrument_data(instrumentIndex, inst_temp) == false)
        {
            USerial.println("load_instrument_data error:");
            SF22ASWT::printSF2ErrorInfo();
            return false;
        }
        if (SF22ASWT::ReadSampleDataFromFile(inst_temp) == false)
        {
            USerial.println("ReadSampleDataFromFile error:");
            SF22ASWT::printSF2ErrorInfo();
            return false;
        }
        AudioSynthWavetable::instrument_data* new_inst = new AudioSynthWavetable::instrument_data(SF22ASWT::converter::to_AudioSynthWavetable_instrument_data(inst_temp));
        if (new_inst == nullptr) // failsafe
        {
            USerial.println("convert to AudioSynthWavetable::instrument_data error!");
            return false;
        }
        
        *aswt_id = new_inst;
        return true;
    }
}