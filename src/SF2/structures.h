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
      String ToString()
      {
        String str;
        str.append(major);
        str.append(".");
        if (minor < 10) str.append("0");
        str.append(minor);
        return str;
      }
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

        String ToString()
        {
            String ret = "\n";
            ret += "*** Info *** ( size: ";
            ret.append(size);
            ret += " )\n\n";
            ret += "Soundfont version: " + ifil.ToString() + "\n";
            ret += "Name: " + INAM + "\n";
            ret += "SoundEngine: " + isng + "\n";
            ret += "ROM: " + irom + "\n";
            ret += "ROM ver: " + iver.ToString() + "\n";
            ret += "Date: " + ICRD + "\n";
            ret += "Credits: " + IENG + "\n";
            ret += "Product: " + IPRD + "\n";
            ret += "Copyright: " + ICOP + "\n";
            ret += "Comment: " + ICMT + "\n";
            ret += "Tools: " + ISFT + "\n";
            return ret;
        }
    };

    class smpl_rec
    {
      public:
        /** smpl data offset as from the beginning of the file */
        uint64_t position;
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
      static const uint32_t Size = 38;
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
      static const uint32_t Size = 4;
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
      static const uint32_t Size = 10;
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
      static const uint32_t Size = 4;
        SFGenerator sfGenOper;
        SF2GeneratorAmount genAmount;
    };

    class inst_rec
    {
      public:
      static const uint32_t Size = 22;
        char achInstName[20];
        uint16_t wInstBagNdx;
    };

    class shdr_rec
    {
      public:
      static const uint32_t Size = 46;
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
        uint32_t phdr_count = 0;
        /** The Preset Index list */
        bag_rec *pbag;
        uint32_t pbag_count = 0;
        /** The Preset Modulator list */
        mod_rec *pmod;
        uint32_t pmod_count = 0;
        /** The Preset Generator list */
        gen_rec *pgen;
        uint32_t pgen_count = 0;
        /** The Instrument Names and Indices */
        inst_rec *inst;
        uint32_t inst_count = 0;
        /** The Instrument Index list */
        bag_rec *ibag;
        uint32_t ibag_count = 0;
        /** The Instrument Modulator list */
        mod_rec *imod;
        uint32_t imod_count = 0;
        /** The Instrument Generator list */
        gen_rec *igen;
        uint32_t igen_count = 0;
        /** The Sample Headers */
        shdr_rec *shdr;
        uint32_t shdr_count = 0;
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
        uint32_t size = 0;
        sfbk_rec sfbk;
    };
}