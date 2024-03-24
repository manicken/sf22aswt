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
    bool ReadFile(String filePath)
    {
        lastReadWasOK = false;
        if (sfbk == nullptr) sfbk = new sfbk_rec_lazy();

        File file = SD.open(filePath.c_str());
        if (!file) {  lastError = "warning - cannot open file " + filePath; return false; }
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
            //USerial.print(">>>"); Helpers::printRawBytes(fourCC, 4); USerial.print("<<< listsize: "); USerial.println(listSize);
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
            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR("read error - while getting infoblock type")
            //USerial.print(">>>"); Helpers::printRawBytes(fourCC, 4); USerial.print("<<<\n");
            if (verifyFourCC(fourCC) == false) FILE_ERROR("error - infoblock type invalid")
            
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

            if ((lastReadCount = file.read(&size, 4)) != 4) FILE_ERROR("read error - while getting pdta block size")

            if (strncmp(fourCC, "phdr", 4) == 0)
            {
                if (size % phdr_rec::Size != 0) FILE_ERROR("error - pdta phdr block size mismatch")

                sfbk->pdta.phdr_count = size/phdr_rec::Size;
                sfbk->pdta.phdr_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping phdr block")
            }
            else if (strncmp(fourCC, "pbag", 4) == 0)
            {
                if (size % bag_rec::Size != 0) FILE_ERROR("error - pdta pbag block size mismatch")

                sfbk->pdta.pbag_count = size/bag_rec::Size;
                sfbk->pdta.pbag_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping pbag block")
            }
            else if (strncmp(fourCC, "pmod", 4) == 0)
            {
                if (size % mod_rec::Size != 0) FILE_ERROR("error - pdta pmod block size mismatch")

                sfbk->pdta.pmod_count = size/mod_rec::Size;
                sfbk->pdta.pmod_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping pmod block")
            }
            else if (strncmp(fourCC, "pgen", 4) == 0)
            {
                if (size % gen_rec::Size != 0) FILE_ERROR("error - pdta pgen block size mismatch")

                sfbk->pdta.pgen_count = size/gen_rec::Size;
                sfbk->pdta.pgen_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping pgen block")
            }
            else if (strncmp(fourCC, "inst", 4) == 0)
            {
                if (size % inst_rec::Size != 0) FILE_ERROR("error - pdta inst block size mismatch")

                sfbk->pdta.inst_count = size/inst_rec::Size;
                sfbk->pdta.inst_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping inst block")
            }
            else if (strncmp(fourCC, "ibag", 4) == 0)
            {
                if (size % bag_rec::Size != 0) FILE_ERROR("error - pdta ibag block size mismatch")

                sfbk->pdta.ibag_count = size/bag_rec::Size;
                sfbk->pdta.ibag_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping ibag block")
            }
            else if (strncmp(fourCC, "imod", 4) == 0)
            {
                if (size % mod_rec::Size != 0) FILE_ERROR("error - pdta imod block size mismatch")

                sfbk->pdta.imod_count = size/mod_rec::Size;
                sfbk->pdta.imod_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping imod block")
            }
            else if (strncmp(fourCC, "igen", 4) == 0)
            {
                if (size % gen_rec::Size != 0) FILE_ERROR("error - pdta igen block size mismatch")

                sfbk->pdta.igen_count = size/gen_rec::Size;
                sfbk->pdta.igen_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping igen block")
            }
            else if (strncmp(fourCC, "shdr", 4) == 0)
            {
                if (size % shdr_rec::Size != 0) FILE_ERROR("error - pdta shdr block size mismatch")

                sfbk->pdta.shdr_count = size/shdr_rec::Size;
                sfbk->pdta.shdr_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_ERROR("seek error - while skipping shdr block")
            }
            else if (strncmp(fourCC, "LIST", 4) == 0) // failsafe if file don't follow standard
            {
                file.seek(-4, SeekCur); // skip back
                return true;
            }
            else
            {
                // normally unknown blocks should be ignored
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
        uint16_t globalGenCount = bags[sampleIndex+1].count;
        for (int i = 0; i < globalGenCount; i++)
        {
            if (bags[0].items[i].sfGenOper == genType) {
                *amount = bags[0].items[i].genAmount;
                return true;
            }
        }
        return false;
    }

    double get_decibel_value(bag_of_gens* bags, int sampleIndex, SFGenerator genType, double DEFAULT, double MIN, double MAX)
    {
        SF2GeneratorAmount genval;
        double val = get_gen_parameter_value(bags, sampleIndex, genType, &genval)?genval.centibels(): DEFAULT;
        return (val > MAX) ? MAX : ((val < MIN) ? MIN : val);
    }
    double get_timecents_value(bag_of_gens* bags, int sampleIndex, SFGenerator genType, double DEFAULT, double MIN)
    {
        SF2GeneratorAmount genval;
        double val = get_gen_parameter_value(bags, sampleIndex, genType, &genval)?genval.cents()*1000.0f: DEFAULT;
        return (val > MIN) ? val : MIN;
    }
    double get_hertz(bag_of_gens* bags, int sampleIndex, SFGenerator genType, double DEFAULT, double MIN, double MAX)
    {
        SF2GeneratorAmount genval;
        double val = get_gen_parameter_value(bags, sampleIndex, genType, &genval)?genval.absolute_cents(): DEFAULT;
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
        return get_gen_parameter_value(bags, sampleIndex, SFGenerator::overridingRootKey, &genval)?genval.Amount:0;
    }
    bool get_sample_header(File &file, bag_of_gens* bags, int sampleIndex, shdr_rec *shdr)
    {
        SF2GeneratorAmount genval;
        if (get_gen_parameter_value(bags, sampleIndex, SFGenerator::sampleID, &genval) == false) return false;
        if (file.seek(sfbk->pdta.shdr_position + genval.UAmount*shdr_rec::Size) == false) return false;
        if (file.read(&shdr, shdr_rec::Size) != shdr_rec::Size) return false;
        return true;
    }
    bool get_sample_repeat(bag_of_gens* bags, int sampleIndex, bool defaultValue)
    {
        SF2GeneratorAmount genVal;
        if (get_gen_parameter_value(bags, sampleIndex, SFGenerator::sampleModes, &genVal) == false) return defaultValue;
        
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

    bool load_instrument(uint index, instrument_data &inst)
    {
        if (index > sfbk->pdta.inst_count - 1) return false;

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
        USerial.print("ibag_start index: "); USerial.print(ibag_startIndex);
        USerial.print(", ibag_end index: "); USerial.println(ibag_endIndex);
        USerial.println(" ");
        file.seek(sfbk->pdta.ibag_position + bag_rec::Size*ibag_startIndex);
        for (int i=0;i<ibag_count+1;i++)
        {
            file.read(&igen_ndxs[i], 2);
            file.read(&dummy, 2); // imod not used
            USerial.print(igen_ndxs[i]);
            USerial.print(", ");
        }
        // store gen data in bags for faster access
        bag_of_gens bags[ibag_count];
        for (int i=0;i<ibag_count;i++)
        {
            uint16_t start = igen_ndxs[i];
            uint16_t end = igen_ndxs[i+1];
            uint16_t count = end-start;
            bags[i].items = new gen_rec[count];
            bags[i].count = count;
            file.seek(sfbk->pdta.igen_position + start*gen_rec::Size);
            for (int i2=0;i2<count;i2++)
            {
                file.read(&bags[i].items[i2], gen_rec::Size);
            }
        }

        inst.sample_count = ibag_count - 1;
        inst.samples = new sample_data[ibag_count - 1];
        for (int i=0;i<inst.sample_count;i++)
        {
            shdr_rec shdr;
            if (get_sample_header(file, bags, i, &shdr) == false) continue;
            
        }

        // Deallocate memory for bags_of_gens
        for (int i = 0; i < ibag_count; i++) {
            delete[] bags[i].items; // Deallocate memory for the array of pointers
        }
        delete[] bags;

        file.close();
        return true;
    }
}