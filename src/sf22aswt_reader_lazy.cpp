
#include "sf22aswt_reader_lazy.h"

namespace SF22ASWT
{
    bool ReaderLazy::ReadFile(const char * filePath)
    {
        lastReadWasOK = false;
        clearErrors();

        File file = SD.open(filePath);
        if (!file) { lastError = SF22ASWT::Error::Errors::FILE_NOT_OPEN; return false; } // extra failsafe

        fileSize = file.size();

        char fourCC[4];

        if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR(FILE_FOURCC_READ) //("read error - while reading fileTag")
        if (verifyFourCC(fourCC) == false) FILE_ERROR(FILE_FOURCC_INVALID) //("error - invalid fileTag")
        if (strncmp(fourCC, "RIFF", 4) != 0) FILE_ERROR(FILE_FOURCC_MISMATCH) //("error - not a RIFF fileformat")
        
        if ((lastReadCount = file.read(&sfbk.size, 4)) != 4) FILE_ERROR(RIFF_SIZE_READ) //("read error - while reading RIFF size")
        if (fileSize != (sfbk.size + 8)) FILE_ERROR(RIFF_SIZE_MISMATCH) //("error - fileSize mismatch")

        if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR(RIFF_FOURCC_READ) //("read error - while reading fileformat")
        if (verifyFourCC(fourCC) == false) FILE_ERROR(RIFF_FOURCC_INVALID) //("error - invalid fileformat")
        
        if (strncmp(fourCC, "sfbk", 4) != 0) FILE_ERROR(RIFF_FOURCC_MISMATCH) //("error - not a sfbk fileformat")

        uint32_t chunkSize = 0;
        while (file.available() > 0)
        {
            // every block starts with a LIST tag
            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR(LIST_FOURCC_READ) //("read error - while reading listTag")
            if (verifyFourCC(fourCC) == false) FILE_ERROR(LIST_FOURCC_INVALID) //("error - listTag invalid")
            if (strncmp(fourCC, "LIST", 4) != 0) FILE_ERROR(LIST_FOURCC_MISMATCH) //("error - listTag is not LIST")

            
            if ((lastReadCount = file.read(&chunkSize, 4)) != 4) FILE_ERROR(LIST_SIZE_READ) //("read error - while getting listSize")

            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR(LISTTYPE_FOURCC_READ) //("read error - while reading listType")
            DebugPrintFOURCC(fourCC);
            DebugPrintFOURCC_size(chunkSize);
            if (verifyFourCC(fourCC) == false) FILE_ERROR(LISTTYPE_FOURCC_INVALID) //("error - invalid listType")
            
            
            if (strncmp(fourCC, "INFO", 4) == 0)
            {
                sfbk.info_position = file.position(); // normally don't read info chunk to save ram
                sfbk.info_size = chunkSize;
                if (file.seek(chunkSize - 4, SeekCur) == false) FILE_SEEK_ERROR(INFO_DATA_SKIP, chunkSize - 4) //("seek error - while skipping INFO block")
                
                //file.close(); return true; // early return debug test
            }
            else if (strncmp(fourCC, "sdta", 4) == 0)
            {
                sfbk.sdta.size = chunkSize;
                if (read_sdta_block(file, sfbk.sdta) == false) return false;
                //file.close(); return true; // early return debug test
            }
            else if (strncmp(fourCC, "pdta", 4) == 0)
            {
                sfbk.pdta.size = chunkSize;
                if (read_pdta_block(file, sfbk.pdta) == false) return false;
                //file.close(); return true; // early return debug test
            }
            else
            {
                // normally unknown blocks should be ignored
                if (file.seek(chunkSize - 4, SeekCur) == false) FILE_SEEK_ERROR(LIST_UNKNOWN_BLOCK_DATA_SKIP, chunkSize - 4) //("seek error - while skipping unknown sfbk root block")
            }
        }

        file.close();
        lastReadWasOK = true;
        this->filePath = filePath;
        return true;
    }

    bool ReaderLazy::printInstrumentListAsJson(Stream &stream)
    {
        clearErrors();
        if (lastReadWasOK == false) { lastError = SF22ASWT::Error::Errors::FILE_NOT_OPEN; return false; }
        File file = SD.open(filePath.c_str());
        if (!file) { lastError = SF22ASWT::Error::Errors::FILE_NOT_OPEN; return false; } // extra failsafe

        if (file.seek(sfbk.pdta.inst_position) == false) FILE_ERROR(PDTA_INST_DATA_SEEK)

        SF22ASWT::inst_rec inst;
        stream.print("json:{\"instruments\":[");
        for (uint32_t i = 0; i < sfbk.pdta.inst_count - 1; i++) // -1 the last is allways a EOI
        {
            file.read(&inst, SF22ASWT::inst_rec::Size);
            stream.print("{\"name\":\"");
            Helpers::printRawBytesUntil(stream, inst.achInstName, 20, '\0');
            stream.print("\",\"ndx\":");
            stream.print(inst.wInstBagNdx);
            stream.print("},");
        }
        file.close();
        stream.println("]}");
        return true;
    }

    bool ReaderLazy::printPresetListAsJson(Stream &stream)
    {
        clearErrors();
        if (lastReadWasOK == false) { lastError = SF22ASWT::Error::Errors::FILE_NOT_OPEN; return false; }
        File file = SD.open(filePath.c_str());
        if (!file) { lastError = SF22ASWT::Error::Errors::FILE_NOT_OPEN; return false; } // extra failsafe

        if (file.seek(sfbk.pdta.phdr_position) == false) FILE_ERROR(PDTA_PHDR_DATA_SEEK)
        
        SF22ASWT::phdr_rec phdr;

        stream.print("json:{\"presets\":[");

        for (uint32_t i = 0; i < sfbk.pdta.phdr_count - 1; i++) // -1 the last is allways a EOP
        {
            file.read(&phdr, SF22ASWT::phdr_rec::Size);
            stream.print("{\"name\":\"");
            Helpers::printRawBytesUntil(stream, phdr.achPresetName, 20, '\0');
            stream.print("\",\"bank\":");
            stream.print(phdr.wBank);
            stream.print(",\"preset\":");
            stream.print(phdr.wPreset);
            stream.print(",\"bagNdx\":");
            stream.print(phdr.wPresetBagNdx);
            stream.print("},");
        }
        file.close();
        stream.println("]}");
        return true;
    }

    bool ReaderLazy::load_instrument_data(uint index, SF22ASWT::instrument_data_temp &inst)
    {
        clearErrors();
        if (lastReadWasOK == false) { lastError = SF22ASWT::Error::Errors::FILE_NOT_OPEN; return false; }
        File file = SD.open(filePath.c_str());
        if (!file) { lastError = SF22ASWT::Error::Errors::FILE_NOT_OPEN; return false; } // extra failsafe

        if (index > sfbk.pdta.inst_count - 1){ 
            //lastError = "load instrument index out of range";
            lastError = SF22ASWT::Error::Errors::FUNCTION_LOAD_INST_INDEX_RANGE;
            return false;
        }

        uint32_t seekPos = sfbk.pdta.inst_position + inst_rec::Size*index + 20;
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
        
        uint16_t ibag_count = ibag_endIndex - ibag_startIndex; 
        
        // store gen data in bags for faster access
        bag_of_gens bags[ibag_count];

        if (fillBagsOfGens(file, bags, ibag_startIndex, ibag_count) == false) {
            return false;
        }
        
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
            if (get_sample_header(file, sfbk, bags, si, &shdr) == false) { 
                break; // classify the file as structually unsound
                //inst.samples[si].invalid = true;
                DebugPrintln_Text_Var("error - while getting sample header @ ", si);
                //continue;
            }
            DebugPrint("sample name: ");
#ifdef SF22ASWT_DEBUG
            Helpers::printRawBytesSanitizedUntil(USerial, shdr.achSampleName, 20, '\0');
#endif
            DebugPrintln();
            DebugPrintln("getting data:");
            inst.sample_note_ranges[si] = get_key_range_end(bags, si);
            inst.samples[si].sample_start = shdr.dwStart*2 + sfbk.sdta.smpl.position;
            inst.samples[si].LOOP = get_sample_repeat(bags, si, false);
            inst.samples[si].SAMPLE_NOTE = get_sample_note(bags, si, shdr);
            inst.samples[si].CENTS_OFFSET = get_fine_tuning(bags, si);
            inst.samples[si].LENGTH = get_length(bags, si, shdr);
            inst.samples[si].LENGTH_BITS = get_length_bits(inst.samples[si].LENGTH);
            inst.samples[si].SAMPLE_RATE = shdr.dwSampleRate;
            inst.samples[si].LOOP_START = get_cooked_loop_start(bags, si, shdr);
            inst.samples[si].LOOP_END = get_cooked_loop_end(bags, si, shdr);
            inst.samples[si].INIT_ATTENUATION = get_decibel_value(bags, si, SFGenerator::initialAttenuation, 0, 0, 144) * -1;
            DebugPrintln("getting vol env");
            // VOLUME ENVELOPE VALUES
            inst.samples[si].DELAY_ENV = get_timecents_value(bags, si, SFGenerator::delayVolEnv, 0, 0);
            inst.samples[si].ATTACK_ENV = get_timecents_value(bags, si, SFGenerator::attackVolEnv, 1, 1);
            inst.samples[si].HOLD_ENV = get_timecents_value(bags, si, SFGenerator::holdVolEnv, 0, 0);
            inst.samples[si].DECAY_ENV = get_timecents_value(bags, si, SFGenerator::decayVolEnv, 1, 1);
            inst.samples[si].RELEASE_ENV = get_timecents_value(bags, si, SFGenerator::releaseVolEnv, 1, 1);
            inst.samples[si].SUSTAIN_FRAC = get_decibel_value(bags, si, SFGenerator::sustainVolEnv, 0, 0, 144) * -1;
            DebugPrintln("getting vib vals");
            // VIRBRATO VALUES
            inst.samples[si].VIB_DELAY_ENV = get_timecents_value(bags, si, SFGenerator::delayVibLFO, 0, 0);
            inst.samples[si].VIB_INC_ENV = get_hertz(bags, si, SFGenerator::freqVibLFO, 8.176, 0.1, 100);
            inst.samples[si].VIB_PITCH_INIT = get_pitch_cents(bags, si, SFGenerator::vibLfoToPitch, 0, -12000, 12000);
            inst.samples[si].VIB_PITCH_SCND = inst.samples[si].VIB_PITCH_INIT * -1; //pitch_cents(bags, si, SFGenerator::vibLfoToPitch, 0, -12000, 12000) * -1;
            DebugPrintln("getting mod vals");
            // MODULATION VALUES
            inst.samples[si].MOD_DELAY_ENV = get_timecents_value(bags, si, SFGenerator::delayModLFO, 0, 0);
            inst.samples[si].MOD_INC_ENV = get_hertz(bags, si, SFGenerator::freqModLFO, 8.176, 0.1, 100);
            inst.samples[si].MOD_PITCH_INIT = get_pitch_cents(bags, si, SFGenerator::modLfoToPitch, 0, -12000, 12000);
            inst.samples[si].MOD_PITCH_SCND = inst.samples[si].MOD_PITCH_INIT * -1; //pitch_cents(bags, si, SFGenerator::modLfoToPitch, 0, -12000, 12000) * -1;
            inst.samples[si].MOD_AMP_INIT_GAIN = get_decibel_value(bags, si, SFGenerator::modLfoToVolume, 0, -96, 96);
            inst.samples[si].MOD_AMP_SCND_GAIN = inst.samples[si].MOD_AMP_INIT_GAIN * -1; //decibel_value(bags, si, SFGenerator::modLfoToVolume, 0, -96, 96) * -1;
        }
        
        file.close();
        return true;
    }

    bool ReaderLazy::fillBagsOfGens(File &file, bag_of_gens* bags, int ibag_startIndex, int ibag_count)
    {
        uint32_t seekPos = sfbk.pdta.ibag_position + bag_rec::Size*ibag_startIndex;
        if (file.seek(seekPos) == false) FILE_SEEK_ERROR(PDTA_IBAG_DATA_SEEK, seekPos) //seek error to ibags
        DebugPrint("igen_ndxs: ");
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
        // search to the location for the first igen record
        seekPos = sfbk.pdta.igen_position + igen_ndxs[0]*gen_rec::Size;
        if (file.seek(seekPos) == false) FILE_SEEK_ERROR(PDTA_IGEN_DATA_SEEK, seekPos) //seek error to first igen record
            
        for (int i=0;i<ibag_count;i++)
        {
            uint16_t start = igen_ndxs[i];
            uint16_t end = igen_ndxs[i+1];
            uint16_t count = end-start;
            bags[i].items = new gen_rec[count];
            bags[i].count = count;

            if ((lastReadCount = file.read(bags[i].items, gen_rec::Size*count)) != gen_rec::Size*count) FILE_ERROR(PDTA_IGEN_DATA_READ)
#ifdef SF22ASWT_DEBUG
            DebugPrintBagContents(bags[i]);
#endif
        }
        DebugPrint("temp storage in bags complete\n");
        return true;
    }

    bool ReaderLazy::load_instrument_from_file(const char * filePath, int instrumentIndex, AudioSynthWavetable::instrument_data **aswt_id)
    {
        
        if (ReadFile(filePath) == false)
        {
            USerial.println("Read file error:");
            printSF2ErrorInfo();
            return false;
        }
        SF22ASWT::instrument_data_temp inst_temp = {0,0,nullptr};

        if (load_instrument_data(instrumentIndex, inst_temp) == false)
        {
            USerial.println("load_instrument_data error:");
            printSF2ErrorInfo();
            return false;
        }
        if (ReadSampleDataFromFile(inst_temp) == false)
        {
            USerial.println("ReadSampleDataFromFile error:");
            printSF2ErrorInfo();
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

    bool ReaderLazy::PrintInfoBlock(Print &stream)
    {
        clearErrors();
        if (lastReadWasOK == false) { lastError = SF22ASWT::Error::Errors::FILE_NOT_OPEN; return false; }
        File file = SD.open(filePath.c_str());
        if (!file) { lastError = SF22ASWT::Error::Errors::FILE_NOT_OPEN; return false; } // extra failsafe

        SF22ASWT::INFO info;
        if (file.seek(sfbk.info_position) == false) FILE_ERROR(INFO_DATA_SEEK)
        if (readInfoBlock(file, info) == false) return false; // readInfoBlock have allready closed the file
        
        file.close();
        info.size = sfbk.info_size;
        info.Print(stream);
        return true;
    }

    bool ReaderLazy::read_pdta_block(File &file, pdta_rec_lazy &pdta)
    {
        char fourCC[4];
        uint32_t size = 0;
        while (file.available())
        {
            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR(PDTA_FOURCC_READ) //("read error - while getting pdta block type")
            DebugPrintFOURCC(fourCC);
            if (verifyFourCC(fourCC) == false) FILE_ERROR(PDTA_FOURCC_INVALID) //("error - pdta type invalid")

            // store result for easier pinpoint of error handing
            bool sizeReadFail = ((lastReadCount = file.read(&size, 4)) != 4);

            if (strncmp(fourCC, "phdr", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_PHDR_SIZE_READ) //("read error - while getting pdta phdr block size")
                if (size % phdr_rec::Size != 0) FILE_ERROR(PDTA_PHDR_SIZE_MISMATCH) //("error - pdta phdr block size mismatch")

                pdta.phdr_count = size/phdr_rec::Size;
                pdta.phdr_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_PHDR_DATA_SKIP, size) //("seek error - while skipping phdr block")
            }
            else if (strncmp(fourCC, "pbag", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_PBAG_SIZE_READ) //("read error - while getting pdta pbag block size")
                if (size % bag_rec::Size != 0) FILE_ERROR(PDTA_PBAG_SIZE_MISMATCH) //("error - pdta pbag block size mismatch")

                pdta.pbag_count = size/bag_rec::Size;
                pdta.pbag_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_PBAG_DATA_SKIP, size) //("seek error - while skipping pbag block")
            }
            else if (strncmp(fourCC, "pmod", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_PMOD_SIZE_READ) //("read error - while getting pdta pmod block size")
                if (size % mod_rec::Size != 0) FILE_ERROR(PDTA_PMOD_SIZE_MISMATCH) //("error - pdta pmod block size mismatch")

                pdta.pmod_count = size/mod_rec::Size;
                pdta.pmod_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_PMOD_DATA_SKIP, size) //("seek error - while skipping pmod block")
            }
            else if (strncmp(fourCC, "pgen", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_PGEN_SIZE_READ) //("read error - while getting pdta pgen block size")
                if (size % gen_rec::Size != 0) FILE_ERROR(PDTA_PGEN_SIZE_MISMATCH) //("error - pdta pgen block size mismatch")

                pdta.pgen_count = size/gen_rec::Size;
                pdta.pgen_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_PGEN_DATA_SKIP, size) //("seek error - while skipping pgen block")
            }
            else if (strncmp(fourCC, "inst", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_INST_SIZE_READ) //("read error - while getting pdta inst block size")
                if (size % inst_rec::Size != 0) FILE_ERROR(PDTA_INST_SIZE_MISMATCH) //("error - pdta inst block size mismatch")

                pdta.inst_count = size/inst_rec::Size;
                pdta.inst_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_INST_DATA_SKIP, size) //("seek error - while skipping inst block")
            }
            else if (strncmp(fourCC, "ibag", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_IBAG_SIZE_READ) //("read error - while getting pdta ibag block size")
                if (size % bag_rec::Size != 0) FILE_ERROR(PDTA_IBAG_SIZE_MISMATCH) //("error - pdta ibag block size mismatch")

                pdta.ibag_count = size/bag_rec::Size;
                pdta.ibag_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_IBAG_DATA_SKIP, size) //("seek error - while skipping ibag block")
            }
            else if (strncmp(fourCC, "imod", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_IMOD_SIZE_READ) //("read error - while getting pdta imod block size")
                if (size % mod_rec::Size != 0) FILE_ERROR(PDTA_IMOD_SIZE_MISMATCH) //("error - pdta imod block size mismatch")

                pdta.imod_count = size/mod_rec::Size;
                pdta.imod_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_IMOD_DATA_SKIP, size) //("seek error - while skipping imod block")
            }
            else if (strncmp(fourCC, "igen", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_IGEN_SIZE_READ) //("read error - while getting pdta igen block size")
                if (size % gen_rec::Size != 0) FILE_ERROR(PDTA_IGEN_SIZE_MISMATCH) //("error - pdta igen block size mismatch")

                pdta.igen_count = size/gen_rec::Size;
                pdta.igen_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_IGEN_DATA_SKIP, size) //("seek error - while skipping igen block")
            }
            else if (strncmp(fourCC, "shdr", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_SHDR_SIZE_READ) //("read error - while getting pdta shdr block size")
                if (size % shdr_rec::Size != 0) FILE_ERROR(PDTA_SHDR_SIZE_MISMATCH) //("error - pdta shdr block size mismatch")

                pdta.shdr_count = size/shdr_rec::Size;
                pdta.shdr_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_SHDR_DATA_SKIP, size) //("seek error - while skipping shdr block")
            }
            else if (strncmp(fourCC, "LIST", 4) == 0) // failsafe if file don't follow standard
            {
                // skip back
                if (file.seek(file.position()-8) == false) FILE_SEEK_ERROR(PDTA_BACK_SEEK, -8) 
                return true;
            }
            else
            {
                // normally unknown blocks should be ignored
                if (sizeReadFail) FILE_ERROR(PDTA_UNKNOWN_BLOCK_SIZE_READ) //("read error - while getting unknown block size")
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_UNKNOWN_BLOCK_DATA_SKIP, size) //("seek error - while skipping unknown pdta block")
            }
        }
        return true;
    }
}