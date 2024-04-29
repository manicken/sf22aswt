#pragma once

#include <Arduino.h>
#include <SD.h>

#include "sf22aswt_common.h"
#include "sf22aswt_structures.h"
#include "sf22aswt_enums.h"
#include "sf22aswt_helpers.h"
#include "sf22aswt_converter.h"

namespace SF22ASWT::reader_lazy::gen_get
{
    bool parameter_value(bag_of_gens* bags, int sampleIndex, SFGenerator genType, SF2GeneratorAmount *amount)
    {
        bool globalExists = (bags[0].count != 0)?(bags[0].lastItem().sfGenOper != SFGenerator::sampleID):true;
        int bagIndex = globalExists?(sampleIndex+1):sampleIndex;

        uint16_t sampleGenCount = bags[bagIndex].count;
        for (int i=0;i<sampleGenCount;i++)
        {
            if (bags[bagIndex].items[i].sfGenOper == genType) {
                *amount = bags[bagIndex].items[i].genAmount;
                return true;
            }
        }
        if (globalExists == false) return false;
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
    float decibel_value(bag_of_gens* bags, int sampleIndex, SFGenerator genType, float DEFAULT, float MIN, float MAX)
    {
        SF2GeneratorAmount genval;
        float val = parameter_value(bags, sampleIndex, genType, &genval)?genval.centibels(): DEFAULT;
        return (val > MAX) ? MAX : ((val < MIN) ? MIN : val);
    }
    float timecents_value(bag_of_gens* bags, int sampleIndex, SFGenerator genType, float DEFAULT, float MIN)
    {
        SF2GeneratorAmount genval;
        float val = parameter_value(bags, sampleIndex, genType, &genval)?genval.cents()*1000.0f: DEFAULT;
        return (val > MIN) ? val : MIN;
    }
    float hertz(bag_of_gens* bags, int sampleIndex, SFGenerator genType, float DEFAULT, float MIN, float MAX)
    {
        SF2GeneratorAmount genval;
        float val = parameter_value(bags, sampleIndex, genType, &genval)?genval.absolute_cents(): DEFAULT;
        return (val > MAX) ? MAX : ((val < MIN) ? MIN : val);
    }
    int pitch_cents(bag_of_gens* bags, int sampleIndex, SFGenerator genType, int DEFAULT, int MIN, int MAX)
    {
        SF2GeneratorAmount genval;
        int val = parameter_value(bags, sampleIndex, genType, &genval)?genval.Amount: DEFAULT;
        return (val > MAX) ? MAX : ((val < MIN) ? MIN : val);
    }
    int cooked_loop_start(bag_of_gens* bags, int sampleIndex, shdr_rec &shdr)
    {
        int result = (int)(shdr.dwStartloop - shdr.dwStart);
        SF2GeneratorAmount genval;
        result += parameter_value(bags, sampleIndex, SFGenerator::startloopAddrsOffset, &genval)?genval.Amount:0;
        result += parameter_value(bags, sampleIndex, SFGenerator::startloopAddrsCoarseOffset, &genval)?genval.coarse_offset():0;
        return result;
    }
    int cooked_loop_end(bag_of_gens* bags, int sampleIndex, shdr_rec &shdr)
    {
        int result = (int)(shdr.dwEndloop - shdr.dwStart);
        SF2GeneratorAmount genval;
        result += parameter_value(bags, sampleIndex, SFGenerator::endloopAddrsOffset, &genval)?genval.Amount:0;
        result += parameter_value(bags, sampleIndex, SFGenerator::endloopAddrsCoarseOffset, &genval)?genval.coarse_offset():0;
        return result;
    }
    int sample_note(bag_of_gens* bags, int sampleIndex, shdr_rec &shdr)
    {
        SF2GeneratorAmount genval;
        return parameter_value(bags, sampleIndex, SFGenerator::overridingRootKey, &genval)?genval.UAmount:((shdr.byOriginalKey <= 127)?shdr.byOriginalKey:60);
    }
    int fine_tuning(bag_of_gens* bags, int sampleIndex)
    {
        SF2GeneratorAmount genval;
        return parameter_value(bags, sampleIndex, SFGenerator::fineTune, &genval)?genval.Amount:0;
    }
    bool sample_header(File &file, sfbk_rec_lazy &sfbk, bag_of_gens* bags, int sampleIndex, shdr_rec *shdr)
    {
        SF2GeneratorAmount genval;
        if (parameter_value(bags, sampleIndex, SFGenerator::sampleID, &genval) == false) return false;
        uint64_t seekPos = sfbk.pdta.shdr_position + genval.UAmount*shdr_rec::Size;
        if (file.seek(seekPos) == false) FILE_SEEK_ERROR(PDTA_SHDR_DATA_SEEK, seekPos)
        if (file.read(shdr, shdr_rec::Size) != shdr_rec::Size) FILE_ERROR(PDTA_SHDR_DATA_READ)
        return true;
    }
    bool sample_repeat(bag_of_gens* bags, int sampleIndex, bool defaultValue)
    {
        SF2GeneratorAmount genVal;
        if (parameter_value(bags, sampleIndex, SFGenerator::sampleModes, &genVal) == false){ DebugPrintln("could not get samplemode"); return defaultValue; }
        
        return (genVal.sample_mode() == SFSampleMode::kLoopContinuously);// || (val.sample_mode == SampleMode.kLoopEndsByKeyDepression);
    }
    int length(bag_of_gens* bags, int sampleIndex, shdr_rec &shdr)
    {
        int length = (int)(shdr.dwEnd - shdr.dwStart);
        int cooked_loop_end_val = cooked_loop_end(bags, sampleIndex, shdr);
        if (sample_repeat(bags, sampleIndex, false) && cooked_loop_end_val < length)
        {
            return cooked_loop_end_val + 1;
        }
        return length;
    }
    int key_range_end(bag_of_gens* bags, int sampleIndex)
    {
        SF2GeneratorAmount genval;
        return parameter_value(bags, sampleIndex, SFGenerator::keyRange, &genval)?genval.rangeHigh():127;
    }
    int length_bits(int len)
    {
        int length_bits = 0;
        while (len != 0)
        {
            length_bits += 1;
            len = len >> 1;
        }
        return length_bits;
    }
}