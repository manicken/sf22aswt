#include <Arduino.h>

namespace SF2
{
    class RIFF
    {
      public:
        /** whole file size */
        long size = 0;

    };

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
            phdr = new
        }
    };

    class phdr_rec
    {
      public:
        uint32_t size; // comes from parent LIST

    };

    /**
     * used by both ibag and pbag
    */
    class bag_rec
    {
      public:

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

    };

    class inst_rec
    {
      public:

    };

    class shdr_rec
    {
      public:

    };

    class sfbk_rec
    {
      public:
        uint32_t size;
        INFO info;
        sdta_rec sdta;
        pdta_rec pdta;
    };


}