#pragma once

#include <Arduino.h>
#include <SD.h>
#include "enums.h"
#include "error_enums.h"
#include "structures.h"
#include "helpers.h"

#ifndef USerial
#define USerial SerialUSB1
#endif

#define USE_EXTMEM

#ifdef USE_EXTMEM
  #define SAMPLEDATA_MALLOC extmem_malloc
  #define SAMPLEDATA_FREE   extmem_free
#else
  #define SAMPLEDATA_MALLOC malloc
  #define SAMPLEDATA_FREE   free
#endif


#ifdef DEBUG
#ifndef USerial
  #define USerial SerialUSB1
#endif
  #define DebugPrint(args) USerial.print(args);
  #define DebugPrintln(args) USerial.println(args);
  #define DebugPrint_Text_Var(text, var) USerial.print(text); USerial.print(var);
  #define DebugPrintln_Text_Var(text, var) USerial.print(text); USerial.println(var);
  #define DebugPrintFOURCC(fourCC) USerial.print(">>>"); Helpers::printRawBytes(fourCC, 4); USerial.println("<<<");
  #define DebugPrintFOURCC_size(size) USerial.print("size: "); USerial.print(size);  USerial.print("\n");
#else
  #define DebugPrint(args)
  #define DebugPrintln(args)
  #define DebugPrint_Text_Var(text, var)
  #define DebugPrintln_Text_Var(text, var)
  #define DebugPrintFOURCC(fourCC)
  #define DebugPrintFOURCC_size(size)
#endif

namespace SF2
{
    uint32_t fileSize;
    String filePath;

    // TODO make all samples load into a single array for easier allocation / deallocation
    // also maybe have it as a own contained memory pool
    sample_data *samples;
    int sample_count = 0;
    int totalSampleDataSizeBytes = 0;


    String lastErrorStr; // used to provide additional info if needed
    Error::Errors lastError = Error::Errors::NONE;
    uint64_t lastErrorPosition;
    size_t lastReadCount = 0; // used to track errors
    void clearErrors()
    {
        lastErrorStr = "";
        lastError = Error::Errors::NONE;
        lastErrorPosition = 0;
        lastReadCount = 0;
    }
    //#define FILE_ERROR(msg) {lastError=msg; lastErrorPosition = file.position() - lastReadCount; file.close(); return false;}
    #define FILE_ERROR(ERROR_TYPE) {lastError=Error::Errors::ERROR_TYPE; lastErrorPosition = file.position() - lastReadCount; file.close(); return false;}
    #define FILE_SEEK_ERROR(ERROR_TYPE, SEEK_POS) {lastError=Error::Errors::ERROR_TYPE; lastErrorPosition = file.position(); lastReadCount = SEEK_POS; file.close(); return false; }
    #define FILE_ERROR_APPEND_SUB(ROOT_TYPE, SUB_TYPE) lastError = (Error::Errors)((uint16_t)lastError & (uint16_t)SF2::Error::ROOT_TYPE::SUB_TYPE);
    
    void printSF2ErrorInfo()
    {
        SF2::Error::printError(SF2::lastError); USerial.print("\n");
        USerial.println(SF2::lastErrorStr);
        USerial.print(" @ position: ");
        USerial.print(SF2::lastErrorPosition);
        USerial.print(", lastReadCount: ");
        USerial.print(SF2::lastReadCount); USerial.print("\n");
    }

    bool ReadStringUsingLeadingSize(File &file, String& string)
    {
        uint32_t size = 0;
        if ((lastReadCount = file.read(&size, 4)) != 4) FILE_ERROR(INFO_STRING_SIZE_READ) //FILE_ERROR("read error - while getting infoblock string size")
        char bytes[size];
        if ((lastReadCount = file.readBytes(bytes, size)) != size) FILE_ERROR(INFO_STRING_DATA_READ) //FILE_ERROR("read error - while reading infoblock string")
        
        // TODO. sanitize bytes
        string = String(bytes);

        return true;
    }
    bool verifyFourCC(const char* fourCC)
    {
        for (int i=0;i<4;i++)
            if (fourCC[i] < 32 || fourCC[i] > 126) return false;
        return true;
    }

    bool readInfoBlock(File &file, INFO &info)
    {
        char fourCC[4];
        while (file.available() > 0)
        {
            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR(INFO_FOURCC_READ) //FILE_ERROR("read error - while getting infoblock type")
            DebugPrintFOURCC(fourCC);
            if (verifyFourCC(fourCC) == false) FILE_ERROR(INFO_FOURCC_INVALID) //FILE_ERROR("error - infoblock type invalid")

            uint32_t dummy = 0;
            if (strncmp(fourCC, "ifil", 4) == 0) {
                if ((lastReadCount = file.read(&dummy, 4)) != 4) FILE_ERROR(INFO_IFIL_SIZE_READ) //FILE_ERROR("read error - ifil size dummy read")
                if ((lastReadCount = file.read(&info.ifil, 4)) != 4) FILE_ERROR(INFO_IFIL_DATA_READ) //FILE_ERROR("read error - while ifil read")
            }
            else if (strncmp(fourCC, "isng", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.isng) == false){ FILE_ERROR_APPEND_SUB(INFO, ISNG) return false; }}// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "INAM", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.INAM) == false){ FILE_ERROR_APPEND_SUB(INFO, INAM) return false; }}// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "irom", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.irom) == false){ FILE_ERROR_APPEND_SUB(INFO, IROM) return false; }}// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "iver", 4) == 0) {
                if ((lastReadCount = file.read(&dummy, 4)) != 4) FILE_ERROR(INFO_IVER_SIZE_READ) //FILE_ERROR("read error - iver size dummy read")
                if ((lastReadCount = file.read(&info.iver, 4)) != 4) FILE_ERROR(INFO_IVER_DATA_READ) //FILE_ERROR("read error - while iver read")
            }
            else if (strncmp(fourCC, "ICRD", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.ICRD) == false){ FILE_ERROR_APPEND_SUB(INFO, ICRD) return false; }}// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "IENG", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.IENG) == false){ FILE_ERROR_APPEND_SUB(INFO, IENG) return false; }}// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "IPRD", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.IPRD) == false){ FILE_ERROR_APPEND_SUB(INFO, IPRD) return false; }}// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "ICOP", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.ICOP) == false){ FILE_ERROR_APPEND_SUB(INFO, ICOP) return false; }}// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "ICMT", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.ICMT) == false){ FILE_ERROR_APPEND_SUB(INFO, ICMT) return false; }}// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "ISFT", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.ISFT) == false){ FILE_ERROR_APPEND_SUB(INFO, ISFT) return false; }}// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "LIST", 4) == 0)
            {
                 // skip back
                if (file.seek(-4, SeekCur) == false) FILE_SEEK_ERROR(INFO_BACK_SEEK, -4)
                return true;
            }
            else
            {
                // normally unknown blocks should be ignored
                if ((lastReadCount = file.read(&dummy, 4)) != 4) FILE_ERROR(INFO_UNKNOWN_BLOCK_SIZE_READ) //FILE_ERROR("read error - while getting unknown INFO block size")
                if (file.seek(file.position() + dummy) == false) FILE_SEEK_ERROR(INFO_UNKNOWN_BLOCK_DATA_SKIP, file.position() + dummy) //FILE_ERROR("seek error - while skipping unknown INFO block")
            }
        }
        return true;
    }

    void FreePrevSampleData()
    {
        DebugPrintln("try to free prev loaded sampledata");
        for (int i = 0;i<sample_count;i++)
        {
            if (samples[i].data != nullptr) {
                DebugPrintln("freeing " + String(i) + " @ " + String((uint64_t)samples[i].data));
                SAMPLEDATA_FREE(samples[i].data);
            }
        }
        DebugPrintln("[OK]");
        delete[] samples;
    }

    bool ReadSampleDataFromFile(instrument_data_temp &inst)
    {
        clearErrors();

        File file = SD.open(SF2::filePath.c_str());
        if (!file) {lastError = Error::Errors::FILE_NOT_OPEN; return false;}
        if (samples != nullptr) {
            FreePrevSampleData();
        } 
        samples = new sample_data[inst.sample_count];
        sample_count = inst.sample_count;
        totalSampleDataSizeBytes = 0;
        for (int si=0;si<inst.sample_count;si++)
        {
            int length_32 = (int)std::ceil((double)inst.samples[si].LENGTH / 2.0f);
            int pad_length = (length_32 % 128 == 0) ? 0 : (128 - length_32 % 128);
            int ary_length = length_32 + pad_length;
            totalSampleDataSizeBytes+=ary_length*4;
            samples[si].data = nullptr; // clear the pointer so free above won't fail if prev. load was unsuccessful
        }
        int allocatedSize = 0;
        for (int si=0;si<inst.sample_count;si++)
        {
            DebugPrintln_Text_Var("reading sample: ", si);
            int length_32 = (int)std::ceil((double)inst.samples[si].LENGTH / 2.0f);
            int length_8 = length_32*4;
            int pad_length = (length_32 % 128 == 0) ? 0 : (128 - length_32 % 128);
            int ary_length = length_32 + pad_length;
            
            samples[si].data = (uint32_t*)SAMPLEDATA_MALLOC(ary_length*4);
            if (samples[si].data == nullptr) {
                lastError = Error::Errors::RAM_DATA_MALLOC;
                lastErrorStr = "@ sample " + String(si) + " could not allocate additional " + String(ary_length*4) + " bytes, allocated " + String(allocatedSize*4) + " of " + String(totalSampleDataSizeBytes) + " bytes";
                file.close();
                FreePrevSampleData();
                return false;
            }

            if (file.seek(inst.samples[si].sample_start) == false) {
                //lastError = "@ sample " +  String(si) + " could not seek to data location in file";
                lastError = Error::Errors::SDTA_SMPL_DATA_SEEK;
                lastErrorPosition = file.position();
                lastReadCount = inst.samples[si].sample_start;
                file.close();
                FreePrevSampleData();
                return false;
            }
            if ((lastReadCount = file.readBytes((char*)samples[si].data, length_8)) != length_8) {
                //lastError = "@ sample " +  String(si) + " could not read sample data from file, wanted:" + length_8 + " but could only read " + lastReadCount;
                lastError = Error::Errors::SDTA_SMPL_DATA_READ;
                lastErrorPosition = inst.samples[si].sample_start;
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
};