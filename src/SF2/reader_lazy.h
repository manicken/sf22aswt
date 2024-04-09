#pragma once

#include <Arduino.h>
#include "enums.h"
#include "structures.h"
#include <SD.h>
#include "helpers.h"
#include "common.h"

#define USerial SerialUSB1

namespace SF2::lazy_reader
{
    sfbk_rec_lazy *sfbk;
    bool lastReadWasOK = false;

    bool read_sdta_block(File &file);
    bool read_pdta_block(File &br);
    
    /** reads and verifies the sf2 file,
     *  note. this is lazy read 
     *  and only the file positions for
     *  all important blocks are stored into ram
     */
    bool ReadFile(const char * filePath)
    {
        lastReadWasOK = false;
        if (sfbk == nullptr) sfbk = new sfbk_rec_lazy();

        File file = SD.open(filePath);
        if (!file) {  lastError = "warning - cannot open file " + String(filePath); return false; }
        fileSize = file.size();

        char fourCC[4];

        if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR("read error - while reading fileTag")
        if (verifyFourCC(fourCC) == false) FILE_ERROR("error - invalid fileTag")
        if (strncmp(fourCC, "RIFF", 4) != 0) FILE_ERROR("error - not a RIFF fileformat")
        
        if ((lastReadCount = file.read(&sfbk->size, 4)) != 4) FILE_ERROR("read error - while reading RIFF size")
        if (fileSize != (sfbk->size + 8)) FILE_ERROR("error - fileSize mismatch")

        if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR("read error - while reading fileformat")
        if (verifyFourCC(fourCC) == false) FILE_ERROR("error - invalid fileformat")
        
        if (strncmp(fourCC, "sfbk", 4) != 0) FILE_ERROR("error - not a sfbk fileformat")

        char listTag[4];
        uint32_t listSize = 0;
        while (file.available() > 0)
        {
            // every block starts with a LIST tag
            if ((lastReadCount = file.readBytes(listTag, 4)) != 4) FILE_ERROR("read error - while reading listTag")
            if (verifyFourCC(listTag) == false) FILE_ERROR("error - listTag invalid")
            if (strncmp(listTag, "LIST", 4) != 0) FILE_ERROR("error - listTag is not LIST")

            
            if ((lastReadCount = file.read(&listSize, 4)) != 4) FILE_ERROR("read error - while getting listSize")

            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR("read error - while reading listType")
            //USerial.print(">>>"); Helpers::printRawBytes(fourCC, 4); USerial.print("<<< listsize: "); USerial.print(listSize);  USerial.print("\n");
            if (verifyFourCC(fourCC) == false) FILE_ERROR("error - invalid listType")
            
            
            if (strncmp(fourCC, "INFO", 4) == 0)
            {
                sfbk->info_position = file.position();
                sfbk->info_size = listSize;
                if (file.seek(listSize - 4, SeekCur) == false) FILE_ERROR("seek error - while skipping INFO block")
                
                //file.close(); return true; // early return debug test
            }
            else if (strncmp(fourCC, "sdta", 4) == 0)
            {
                sfbk->sdta.size = listSize;
                if (read_sdta_block(file) == false) return false;
                //file.close(); return true; // early return debug test
            }
            else if (strncmp(fourCC, "pdta", 4) == 0)
            {
                sfbk->pdta.size = listSize;
                if (read_pdta_block(file) == false) return false;
                //file.close(); return true; // early return debug test
            }
            else
            {
                // normally unknown blocks should be ignored
                if (file.seek(listSize - 4, SeekCur) == false) FILE_ERROR("seek error - while skipping unknown sfbk root block")
            }
        }

        file.close();
        lastReadWasOK = true;
        SF2::filePath = filePath;
        return true;
    }

    /// <summary>
    /// reads data offset pointers and sizes of sample data, not the actual data
    /// as the data is read from file on demand
    /// </summary>
    /// <param name="br"></param>
    /// <returns></returns>
    bool read_sdta_block(File &file)
    {
        char fourCC[4];
        while (file.available())
        {
            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR("read error - while getting sdtablock type")
            //USerial.print(">>>"); Helpers::printRawBytes(fourCC, 4); USerial.print("<<<\n");
            if (verifyFourCC(fourCC) == false) FILE_ERROR("error - sdtablock type invalid")
            
            if (strncmp(fourCC, "smpl", 4) == 0)
            {
                if ((lastReadCount = file.read(&sfbk->sdta.smpl.size, 4)) != 4) FILE_ERROR("read error - while reading smpl size")
                sfbk->sdta.smpl.position = file.position();
                // skip sample data
                if (file.seek(sfbk->sdta.smpl.size, SeekCur) == false) FILE_ERROR("seek error - while skipping smpl data")
            }
            else if (strncmp(fourCC, "sm24", 4) == 0)
            {
                if ((lastReadCount = file.read(&sfbk->sdta.sm24.size, 4)) != 4) FILE_ERROR("read error - while reading sm24 size")
                sfbk->sdta.sm24.position = file.position();
                // skip sample data
                if (file.seek(sfbk->sdta.sm24.size, SeekCur) == false) FILE_ERROR("seek error - while skipping sm24 data")
            }
            else if (strncmp(fourCC, "LIST", 4) == 0)
            {
                file.seek(-4, SeekCur); // skip back
                return true;
            }
            else
            {
                // normally unknown blocks should be ignored
                uint32_t size = 0;
                if ((lastReadCount = file.read(&size, 4)) != 4) FILE_ERROR("read error - while getting unknown sdta block size")
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping unknown sdta block")
            }
        }
        return true;
    }

    bool read_pdta_block(File &file)
    {
        char fourCC[4];
        uint32_t size = 0;
        while (file.available())
        {
            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR("read error - while getting pdta block type")
            //USerial.print(">>>"); Helpers::printRawBytes(fourCC, 4); USerial.print("<<<\n");
            if (verifyFourCC(fourCC) == false) FILE_ERROR("error - pdta type invalid")

            // store result for easier pinpoint of error handing
            bool sizeReadFail = ((lastReadCount = file.read(&size, 4)) != 4);

            if (strncmp(fourCC, "phdr", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR("read error - while getting pdta phdr block size")
                if (size % phdr_rec::Size != 0) FILE_ERROR("error - pdta phdr block size mismatch")

                sfbk->pdta.phdr_count = size/phdr_rec::Size;
                sfbk->pdta.phdr_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping phdr block")
            }
            else if (strncmp(fourCC, "pbag", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR("read error - while getting pdta pbag block size")
                if (size % bag_rec::Size != 0) FILE_ERROR("error - pdta pbag block size mismatch")

                sfbk->pdta.pbag_count = size/bag_rec::Size;
                sfbk->pdta.pbag_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping pbag block")
            }
            else if (strncmp(fourCC, "pmod", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR("read error - while getting pdta pmod block size")
                if (size % mod_rec::Size != 0) FILE_ERROR("error - pdta pmod block size mismatch")

                sfbk->pdta.pmod_count = size/mod_rec::Size;
                sfbk->pdta.pmod_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping pmod block")
            }
            else if (strncmp(fourCC, "pgen", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR("read error - while getting pdta pgen block size")
                if (size % gen_rec::Size != 0) FILE_ERROR("error - pdta pgen block size mismatch")

                sfbk->pdta.pgen_count = size/gen_rec::Size;
                sfbk->pdta.pgen_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping pgen block")
            }
            else if (strncmp(fourCC, "inst", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR("read error - while getting pdta inst block size")
                if (size % inst_rec::Size != 0) FILE_ERROR("error - pdta inst block size mismatch")

                sfbk->pdta.inst_count = size/inst_rec::Size;
                sfbk->pdta.inst_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping inst block")
            }
            else if (strncmp(fourCC, "ibag", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR("read error - while getting pdta ibag block size")
                if (size % bag_rec::Size != 0) FILE_ERROR("error - pdta ibag block size mismatch")

                sfbk->pdta.ibag_count = size/bag_rec::Size;
                sfbk->pdta.ibag_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping ibag block")
            }
            else if (strncmp(fourCC, "imod", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR("read error - while getting pdta imod block size")
                if (size % mod_rec::Size != 0) FILE_ERROR("error - pdta imod block size mismatch")

                sfbk->pdta.imod_count = size/mod_rec::Size;
                sfbk->pdta.imod_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping imod block")
            }
            else if (strncmp(fourCC, "igen", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR("read error - while getting pdta igen block size")
                if (size % gen_rec::Size != 0) FILE_ERROR("error - pdta igen block size mismatch")

                sfbk->pdta.igen_count = size/gen_rec::Size;
                sfbk->pdta.igen_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping igen block")
            }
            else if (strncmp(fourCC, "shdr", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR("read error - while getting pdta shdr block size")
                if (size % shdr_rec::Size != 0) FILE_ERROR("error - pdta shdr block size mismatch")

                sfbk->pdta.shdr_count = size/shdr_rec::Size;
                sfbk->pdta.shdr_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping shdr block")
            }
            else if (strncmp(fourCC, "LIST", 4) == 0) // failsafe if file don't follow standard
            {
                
                file.seek(-8, SeekCur); // skip back
                return true;
            }
            else
            {
                // normally unknown blocks should be ignored
                if (sizeReadFail) FILE_ERROR("read error - while getting unknown block size")
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping unknown pdta block")
            }
        }
        return true;
    }

    bool get_gen_parameter_value(bag_of_gens* bags, int sampleIndex, SFGenerator genType, SF2GeneratorAmount *amount)
    {
        uint16_t sampleGenCount = bags[sampleIndex+1].count;
        for (int i=0;i<sampleGenCount;i++)
        {
            if (bags[sampleIndex+1].items[i].sfGenOper == genType) {
                *amount = bags[sampleIndex+1].items[i].genAmount;
                return true;
            }
        }
        // try again with global bag
        uint16_t globalGenCount = bags[0].count;
        for (int i = 0; i < globalGenCount; i++)
        {
            if (bags[0].items[i].sfGenOper == genType) {
                *amount = bags[0].items[i].genAmount;
                return true;
            }
        }
        return false;
    }

    float get_decibel_value(bag_of_gens* bags, int sampleIndex, SFGenerator genType, float DEFAULT, float MIN, float MAX)
    {
        SF2GeneratorAmount genval;
        float val = get_gen_parameter_value(bags, sampleIndex, genType, &genval)?genval.centibels(): DEFAULT;
        return (val > MAX) ? MAX : ((val < MIN) ? MIN : val);
    }
    float get_timecents_value(bag_of_gens* bags, int sampleIndex, SFGenerator genType, float DEFAULT, float MIN)
    {
        SF2GeneratorAmount genval;
        float val = get_gen_parameter_value(bags, sampleIndex, genType, &genval)?genval.cents()*1000.0f: DEFAULT;
        return (val > MIN) ? val : MIN;
    }
    float get_hertz(bag_of_gens* bags, int sampleIndex, SFGenerator genType, float DEFAULT, float MIN, float MAX)
    {
        SF2GeneratorAmount genval;
        float val = get_gen_parameter_value(bags, sampleIndex, genType, &genval)?genval.absolute_cents(): DEFAULT;
        return (val > MAX) ? MAX : ((val < MIN) ? MIN : val);
    }
    int get_pitch_cents(bag_of_gens* bags, int sampleIndex, SFGenerator genType, int DEFAULT, int MIN, int MAX)
    {
        SF2GeneratorAmount genval;
        int val = get_gen_parameter_value(bags, sampleIndex, genType, &genval)?genval.Amount: DEFAULT;
        return (val > MAX) ? MAX : ((val < MIN) ? MIN : val);
    }
    int get_cooked_loop_start(bag_of_gens* bags, int sampleIndex, shdr_rec &shdr)
    {
        int result = (int)(shdr.dwStartloop - shdr.dwStart);
        SF2GeneratorAmount genval;
        result += get_gen_parameter_value(bags, sampleIndex, SFGenerator::startloopAddrsOffset, &genval)?genval.Amount:0;
        result += get_gen_parameter_value(bags, sampleIndex, SFGenerator::startloopAddrsCoarseOffset, &genval)?genval.coarse_offset():0;
        return result;
    }
    int get_cooked_loop_end(bag_of_gens* bags, int sampleIndex, shdr_rec &shdr)
    {
        int result = (int)(shdr.dwEndloop - shdr.dwStart);
        SF2GeneratorAmount genval;
        result += get_gen_parameter_value(bags, sampleIndex, SFGenerator::endloopAddrsOffset, &genval)?genval.Amount:0;
        result += get_gen_parameter_value(bags, sampleIndex, SFGenerator::endloopAddrsCoarseOffset, &genval)?genval.coarse_offset():0;
        return result;
    }
    int get_sample_note(bag_of_gens* bags, int sampleIndex, shdr_rec &shdr)
    {
        SF2GeneratorAmount genval;
        return get_gen_parameter_value(bags, sampleIndex, SFGenerator::overridingRootKey, &genval)?genval.UAmount:((shdr.byOriginalKey <= 127)?shdr.byOriginalKey:60);
    }
    int get_fine_tuning(bag_of_gens* bags, int sampleIndex)
    {
        SF2GeneratorAmount genval;
        return get_gen_parameter_value(bags, sampleIndex, SFGenerator::fineTune, &genval)?genval.Amount:0;
    }
    bool get_sample_header(File &file, bag_of_gens* bags, int sampleIndex, shdr_rec *shdr)
    {
        SF2GeneratorAmount genval;
        if (get_gen_parameter_value(bags, sampleIndex, SFGenerator::sampleID, &genval) == false) return false;
        if (file.seek(sfbk->pdta.shdr_position + genval.UAmount*shdr_rec::Size) == false) return false;
        if (file.read(shdr, shdr_rec::Size) != shdr_rec::Size) return false;
        return true;
    }
    bool get_sample_repeat(bag_of_gens* bags, int sampleIndex, bool defaultValue)
    {
        SF2GeneratorAmount genVal;
        if (get_gen_parameter_value(bags, sampleIndex, SFGenerator::sampleModes, &genVal) == false){ /*USerial.print("could not get samplemode\n");*/ return defaultValue; }
        
        return (genVal.sample_mode() == SFSampleMode::kLoopContinuously);// || (val.sample_mode == SampleMode.kLoopEndsByKeyDepression);
    }
    int get_length(bag_of_gens* bags, int sampleIndex, shdr_rec &shdr)
    {
        int length = (int)(shdr.dwEnd - shdr.dwStart);
        int cooked_loop_end = get_cooked_loop_end(bags, sampleIndex, shdr);
        if (get_sample_repeat(bags, sampleIndex, false) && cooked_loop_end < length)
        {
            return cooked_loop_end + 1;
        }
        return length;
    }
    int get_key_range_end(bag_of_gens* bags, int sampleIndex)
    {
        SF2GeneratorAmount genval;
        return get_gen_parameter_value(bags, sampleIndex, SFGenerator::keyRange, &genval)?genval.rangeHigh():127;
    }
    int get_length_bits(int len)
    {
        int length_bits = 0;
        while (len != 0)
        {
            length_bits += 1;
            len = len >> 1;
        }
        return length_bits;
    }

    bool load_instrument(uint index, SF2::instrument_data_temp &inst)
    {
        if (index > sfbk->pdta.inst_count - 1){ lastError = "load instrument index out of range"; return false; }

        File file = SD.open(SF2::filePath.c_str());
        file.seek(sfbk->pdta.inst_position + inst_rec::Size*index + 20);
        uint16_t ibag_startIndex = 0;
        uint16_t ibag_endIndex = 0;
        file.read(&ibag_startIndex, 2);
        file.seek(20, SeekCur);
        file.read(&ibag_endIndex, 2);
        uint16_t ibag_count = ibag_endIndex - ibag_startIndex; 
        uint16_t igen_ndxs[ibag_count+1]; // +1 because of the soundfont structure 
        uint16_t dummy = 0;
        USerial.print("\nibag_start index: "); USerial.print(ibag_startIndex);
        USerial.print(", ibag_end index: "); USerial.print(ibag_endIndex);  USerial.print("\n");
        USerial.print("\n");
        if (file.seek(sfbk->pdta.ibag_position + bag_rec::Size*ibag_startIndex) == false) {USerial.print("seek error to ibags\n"); file.close(); return false;}
        USerial.print("seek complete\n");
        for (int i=0;i<ibag_count+1;i++)
        {
            if (file.read(&igen_ndxs[i], 2) != 2) {USerial.print("read error - while reading &igen_ndxs[i]\n"); file.close(); return false;}
            if (file.read(&dummy, 2) != 2) {USerial.print("read error - while reading dummy\n"); file.close(); return false;}; // imod not used
            USerial.print(igen_ndxs[i]);
            USerial.print(", ");
        }
        USerial.print("\n");
        // store gen data in bags for faster access
        bag_of_gens bags[ibag_count];
        for (int i=0;i<ibag_count;i++)
        {
            uint16_t start = igen_ndxs[i];
            uint16_t end = igen_ndxs[i+1];
            uint16_t count = end-start;
            bags[i].items = new gen_rec[count];
            bags[i].count = count;
            if (file.seek(sfbk->pdta.igen_position + start*gen_rec::Size) == false) {USerial.print("seek error to igen\n"); file.close(); return false;};
            for (int i2=0;i2<count;i2++)
            {
                file.read(&bags[i].items[i2], gen_rec::Size);
            }
        }
        USerial.print("temp storage in bags complete\n");
        inst.sample_count = ibag_count - 1;
        inst.sample_note_ranges = new uint8_t[inst.sample_count];
        inst.samples = new sample_header_temp[inst.sample_count];
        for (int si=0;si<inst.sample_count;si++)
        {
            shdr_rec shdr;
            //USerial.print("getting sample x: ");USerial.print(si); USerial.print("\n");
            if (get_sample_header(file, bags, si, &shdr) == false) { 
                inst.samples[si].invalid = true;
                USerial.print("error - while getting sample header @ "); USerial.print(si); USerial.print("\n");
                continue;
            }
            //USerial.print("getting data\n");
            inst.sample_note_ranges[si] = get_key_range_end(bags, si);
            inst.samples[si].invalid = false; // used later as a failsafe when getting data
            inst.samples[si].sample_start = shdr.dwStart;
            inst.samples[si].LOOP = get_sample_repeat(bags, si, false);
            inst.samples[si].SAMPLE_NOTE = get_sample_note(bags, si, shdr);
            inst.samples[si].CENTS_OFFSET = get_fine_tuning(bags, si);
            inst.samples[si].LENGTH = get_length(bags, si, shdr);
            inst.samples[si].LENGTH_BITS = get_length_bits(inst.samples[si].LENGTH);
            inst.samples[si].SAMPLE_RATE = shdr.dwSampleRate;
            inst.samples[si].LOOP_START = get_cooked_loop_start(bags, si, shdr);
            inst.samples[si].LOOP_END = get_cooked_loop_end(bags, si, shdr);
            inst.samples[si].INIT_ATTENUATION = get_decibel_value(bags, si, SFGenerator::initialAttenuation, 0, 0, 144) * -1;
            //USerial.print("getting vol env\n");
            // VOLUME ENVELOPE VALUES
            inst.samples[si].DELAY_ENV = get_timecents_value(bags, si, SFGenerator::delayVolEnv, 0, 0);
            inst.samples[si].ATTACK_ENV = get_timecents_value(bags, si, SFGenerator::attackVolEnv, 1, 1);
            inst.samples[si].HOLD_ENV = get_timecents_value(bags, si, SFGenerator::holdVolEnv, 0, 0);
            inst.samples[si].DECAY_ENV = get_timecents_value(bags, si, SFGenerator::decayVolEnv, 1, 1);
            inst.samples[si].RELEASE_ENV = get_timecents_value(bags, si, SFGenerator::releaseVolEnv, 1, 1);
            inst.samples[si].SUSTAIN_FRAC = get_decibel_value(bags, si, SFGenerator::sustainVolEnv, 0, 0, 144) * -1;
            //USerial.print("getting vib vals\n");
            // VIRBRATO VALUES
            inst.samples[si].VIB_DELAY_ENV = get_timecents_value(bags, si, SFGenerator::delayVibLFO, 0, 0);
            inst.samples[si].VIB_INC_ENV = get_hertz(bags, si, SFGenerator::freqVibLFO, 8.176, 0.1, 100);
            inst.samples[si].VIB_PITCH_INIT = get_pitch_cents(bags, si, SFGenerator::vibLfoToPitch, 0, -12000, 12000);
            inst.samples[si].VIB_PITCH_SCND = inst.samples[si].VIB_PITCH_INIT * -1; //get_pitch_cents(bags, si, SFGenerator::vibLfoToPitch, 0, -12000, 12000) * -1;
            //USerial.print("getting mod vals\n");
            // MODULATION VALUES
            inst.samples[si].MOD_DELAY_ENV = get_timecents_value(bags, si, SFGenerator::delayModLFO, 0, 0);
            inst.samples[si].MOD_INC_ENV = get_hertz(bags, si, SFGenerator::freqModLFO, 8.176, 0.1, 100);
            inst.samples[si].MOD_PITCH_INIT = get_pitch_cents(bags, si, SFGenerator::modLfoToPitch, 0, -12000, 12000);
            inst.samples[si].MOD_PITCH_SCND = inst.samples[si].MOD_PITCH_INIT * -1; //get_pitch_cents(bags, si, SFGenerator::modLfoToPitch, 0, -12000, 12000) * -1;
            inst.samples[si].MOD_AMP_INIT_GAIN = get_decibel_value(bags, si, SFGenerator::modLfoToVolume, 0, -96, 96);
            inst.samples[si].MOD_AMP_SCND_GAIN = inst.samples[si].MOD_AMP_INIT_GAIN * -1; //get_decibel_value(bags, si, SFGenerator::modLfoToVolume, 0, -96, 96) * -1;
        }

        // Deallocate memory for bags_of_gens
        for (int i = 0; i < ibag_count; i++) {
            delete[] bags[i].items; // Deallocate memory for the array of pointers
        }
        USerial.print("instrument load complete\n");
        file.close();
        return true;
    }

    void FreePrevSampleData()
    {
        //USerial.print("try to free prev loaded sampledata\n");
        for (int i = 0;i<sample_count;i++)
        {
            if (samples[i].data != nullptr) {
                //USerial.print("freeing " + String(i) + " @ " + String((uint64_t)samples[i].data + "\n"));
                extmem_free(samples[i].data);
            }
        }
        //USerial.print("[OK]\n");
        delete[] samples;
    }

    bool ReadSampleDataFromFile(instrument_data_temp &inst)
    {
        File file = SD.open(SF2::filePath.c_str());
        if (!file) {USerial.print("cannot open file\n"); return false;}
        if (samples != nullptr) {
            FreePrevSampleData();
        } 
        samples = new sample_data[inst.sample_count];
        sample_count = inst.sample_count;
        int totalSize = 0;
        for (int si=0;si<inst.sample_count;si++)
        {
            //USerial.print("reading sample: "); USerial.print(si); USerial.print("\n");
            int length_32 = (int)std::ceil((double)inst.samples[si].LENGTH / 2.0f);
            int pad_length = (length_32 % 128 == 0) ? 0 : (128 - length_32 % 128);
            int ary_length = length_32 + pad_length;
            totalSize+=ary_length;
            samples[si].data = nullptr; // clear the pointer so free above won't fail if prev. load was unsuccessful
        }
        USerial.print("\ntotal size in bytes of current instrument samples inclusive padding: "); USerial.print(totalSize*4);  USerial.print("\n");
        int allocatedSize = 0;
        for (int si=0;si<inst.sample_count;si++)
        {
            //USerial.print("reading sample: "); USerial.print(si);  USerial.print("\n");
            int length_32 = (int)std::ceil((double)inst.samples[si].LENGTH / 2.0f);
            int length_8 = length_32*4;
            int pad_length = (length_32 % 128 == 0) ? 0 : (128 - length_32 % 128);
            int ary_length = length_32 + pad_length;
            
            samples[si].data = (uint32_t*)extmem_malloc(ary_length*4);
            if (samples[si].data == nullptr) {
                lastError = "@ sample " + String(si) + "Not enough memory to allocate additional " + String(ary_length*4) + " bytes, allocated " + String(allocatedSize*4) + " of " + String(totalSize*4) + " bytes";
                file.close();
                FreePrevSampleData();
                return false;
            }

            if (file.seek(sfbk->sdta.smpl.position + inst.samples[si].sample_start*2) == false) {
                lastError = "@ sample " +  String(si) + " could not seek to data location in file";
                file.close();
                FreePrevSampleData();
                return false;
            }
            if (lastReadCount = file.readBytes((char*)samples[si].data, length_8) != length_8) {
                lastError = "@ sample " +  String(si) + " could not read sample data from file, wanted:" + length_8 + " but could only read " + lastReadCount;
                file.close();
                FreePrevSampleData();
                return false;
            }
            for (int i = length_32; i < ary_length;i++)
            {
                samples[si].data[i] = 0x00000000;
            }
            inst.samples[si].sample = (int16_t*)samples[si].data;
            allocatedSize+=ary_length;
        }
        
        file.close();
        return true;
    }
}