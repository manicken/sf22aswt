#pragma once

#include <Arduino.h>
#include <SD.h>
#include "enums.h"
#include "error_enums.h"
#include "structures.h"
#include "helpers.h"

#define USerial SerialUSB1

namespace SF2
{
    uint32_t fileSize;
    String filePath;


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
    #define FILE_ERROR_APPEND_SUB(ROOT_TYPE, SUB_TYPE) lastError = (Error::Errors)((uint16_t)lastError & (uint16_t)SF2::Error::ROOT_TYPE::SUB_TYPE);
    // TODO make all samples load into a single array for easier allocation / deallocation
    // also maybe have it as a own contained memory pool
    sample_data *samples;
    int sample_count = 0;

    bool ReadStringUsingLeadingSize(File &file, String& string)
    {
        uint32_t size = 0;
        if ((lastReadCount = file.read(&size, 4)) != 4) FILE_ERROR(INFO_STRING_SIZE_READ) //FILE_ERROR("read error - while getting infoblock string size")
        //DSerial.printf("string size:%ld",size);
        char bytes[size];
        if ((lastReadCount = file.readBytes(bytes, size)) != size) FILE_ERROR(INFO_STRING_DATA_READ) //FILE_ERROR("read error - while reading infoblock string")
        
        // TODO. sanitize bytes
        string = String(bytes);
        //printRawBytes(str.c_str(), size);

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
        //USerial.print("\navailable >>>"); USerial.print(file.available()); USerial.print("\n"); USerial.print("<<<\n");
        char fourCC[4];
        while (file.available() > 0)
        {
            //USerial.printf("\n  file position: %ld\n", file.position());
            
            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR(INFO_FOURCC_READ) //FILE_ERROR("read error - while getting infoblock type")
            //USerial.print(">>>"); Helpers::printRawBytes(fourCC, 4); USerial.print("<<<\n");
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
                file.seek(file.position() - 4); // skip back
                return true;
            }
            else
            {
                // normally unknown blocks should be ignored
                if ((lastReadCount = file.read(&dummy, 4)) != 4) FILE_ERROR(INFO_UNKNOWN_BLOCK_SIZE_READ) //FILE_ERROR("read error - while getting unknown INFO block size")
                if (file.seek(file.position() + dummy) == false) FILE_ERROR(INFO_UNKNOWN_BLOCK_DATA_SKIP) //FILE_ERROR("seek error - while skipping unknown INFO block")
            }
        }
        return true;
    }
};