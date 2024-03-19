#include <Arduino.h>
#include "enums.h"

#pragma once

namespace SF2
{
    

    class sfVersionTag
    {
      public:
        uint16_t major;
        uint16_t minor;
    };

    class INFO
    {
      public:
        uint32_t size; // comes from the parent LIST

        // mandatory data fields

        /** The SoundFont specification version */
        sfVersionTag ifil;
        /** The sound engine for which the SoundFont was optimized */
        String isng;
        /** The name of the SoundFont */
        String INAM;

        // optional data fields

        /** A sound data ROM to which any ROM samples refer */
        String irom;
        /** A sound data ROM revision to which any ROM samples refer */
        sfVersionTag iver;
        /** The creation date of the SoundFont, conventionally in the 'Month Day, Year' format */
        String ICRD;
        /** The author or authors of the SoundFont */
        String IENG;
        /** The product for which the SoundFont is intended */
        String IPRD;
        /** Copyright assertion string associated with the SoundFont */
        String ICOP;
        /** Any comments associated with the SoundFont */
        String ICMT;
        /** The tool used to create or edit the SoundFont */
        String ISFT;
    };

    class smpl_rec
    {
      public:
        /** smpl data offset as from the beginning of the file */
        uint32_t position;
        /** smpl data size */
        uint32_t size;
    };

    class sdta_rec
    {
      public:
        uint32_t size; // comes from parent LIST
        smpl_rec smpl;
        smpl_rec sm24;
    };

    class phdr_rec
    {
      public:
        uint32_t size; // comes from parent LIST
        char achPresetName[20];
        uint16_t wPreset;
        uint16_t wBank;
        uint16_t wPresetBagNdx;
        uint32_t dwLibrary;
        uint32_t dwGenre;
        uint32_t dwMorphology;
    };

    /**
     * used by both ibag and pbag
    */
    class bag_rec
    {
      public:
        uint16_t wGenNdx;
        uint16_t wModNdx;
    };
    
    struct SFModulator
    {
        union
        {
            uint16_t rawdata;
            struct {
                uint16_t Type : 6;
                uint16_t P : 1;
                uint16_t D : 1;
                uint16_t CC : 1;
                uint16_t Index: 7;
            };
        };
        
    };

    /**
     * used by both imod and pmod
    */
    class mod_rec
    {
      public:
        SFModulator sfModSrcOper;
        SFGenerator sfModDestOper;
        int16_t     modAmount;
        SFModulator sfModAmtSrcOper;
        SFTransform sfModTransOper;
        uint8_t     padding; // to get right size when using sizeof
    };

    struct SF2GeneratorAmount
    {
        union
        {
            struct
            {
                uint8_t LowByte;
                uint8_t HighByte;
            };
            uint16_t UAmount;
            int16_t Amount;            
        };
        
    };

    /**
     * used by both igen and pgen
    */
    class gen_rec
    {
      public:
        SFGenerator sfGenOper;
        SF2GeneratorAmount genAmount;
    };

    class inst_rec
    {
      public:
        char achInstName[20];
        uint16_t wInstBagNdx;
    };

    class shdr_rec
    {
      public:
        char achSampleName[20];
        uint32_t dwStart;
        uint32_t dwEnd;
        uint32_t dwStartloop;
        uint32_t dwEndloop;
        uint32_t dwSampleRate;
        uint8_t byOriginalKey;
        int8_t chCorrection;
        uint16_t wSampleLink;
        SFSampleLink sfSampleType;
    };

    class pdta_rec
    {
      public:
        uint32_t size; // comes from parent LIST
        /** The Preset Headers */
        phdr_rec *phdr;
        /** The Preset Index list */
        bag_rec *pbag;
        /** The Preset Modulator list */
        mod_rec *pmod;
        /** The Preset Generator list */
        gen_rec *pgen;
        /** The Instrument Names and Indices */
        inst_rec *inst;
        /** The Instrument Index list */
        bag_rec *ibag;
        /** The Instrument Modulator list */
        mod_rec *imod;
        /** The Instrument Generator list */
        gen_rec *igen;
        /** The Sample Headers */
        shdr_rec *shdr;
        pdta_rec()
        {
            phdr = new phdr_rec[0];
            pbag = new bag_rec[0];
            pmod = new mod_rec[0];
            pgen = new gen_rec[0];
            inst = new inst_rec[0];
            ibag = new bag_rec[0];
            imod = new mod_rec[0];
            igen = new gen_rec[0];
            shdr = new shdr_rec[0];            
        }
    };

    class sfbk_rec
    {
      public:
        uint32_t size;
        INFO info;
        sdta_rec sdta;
        pdta_rec pdta;
    };

    class RIFF
    {
      public:
        /** whole file size */
        long size = 0;
        sfbk_rec sfbk;
    };
}