#pragma once

#include <Arduino.h>
#include <SD.h>
#include "enums.h"
#include "structures.h"
#include "helpers.h"

#define USerial SerialUSB1

namespace SF2
{
    uint32_t fileSize;
    String filePath;

    String lastError; // TODO: change string lastError into a enum
    uint64_t lastErrorPosition;
    size_t lastReadCount = 0; // used to track errors
    #define FILE_ERROR(msg) {lastError=msg; lastErrorPosition = file.position() - lastReadCount; file.close(); return false;}

    bool ReadStringUsingLeadingSize(File &file, String& string)
    {
        uint32_t size = 0;
        if ((lastReadCount = file.read(&size, 4)) != 4) FILE_ERROR("read error - while getting infoblock string size")
        //DSerial.printf("string size:%ld",size);
        char bytes[size];
        if ((lastReadCount = file.readBytes(bytes, size)) != size) FILE_ERROR("read error - while reading infoblock string")
        
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
        //SerialUSB1.print("\navailable >>>"); SerialUSB1.println(file.available()); SerialUSB1.print("<<<\n");
        char fourCC[4];
        while (file.available() > 0)
        {
            //DSerial.printf("\n  file position: %ld\n", file.position());
            
            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR("read error - while getting infoblock type")
            USerial.print(">>>"); Helpers::printRawBytes(fourCC, 4); USerial.print("<<<\n");
            if (verifyFourCC(fourCC) == false) FILE_ERROR("error - infoblock type invalid")

            uint32_t dummy = 0;
            if (strncmp(fourCC, "ifil", 4) == 0) {
                if ((lastReadCount = file.read(&dummy, 4)) != 4) FILE_ERROR("read error - ifil size dummy read")
                if ((lastReadCount = file.read(&info.ifil, 4)) != 4) FILE_ERROR("read error - while ifil read")
            }
            else if (strncmp(fourCC, "isng", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.isng) == false) return false; }// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "INAM", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.INAM) == false) return false; }// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "irom", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.irom) == false) return false; }// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "iver", 4) == 0) {
                if ((lastReadCount = file.read(&dummy, 4)) != 4) FILE_ERROR("read error - iver size dummy read")
                if ((lastReadCount = file.read(&info.iver, 4)) != 4) FILE_ERROR("read error - while iver read")
            }
            else if (strncmp(fourCC, "ICRD", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.ICRD) == false) return false; }// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "IENG", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.IENG) == false) return false; }// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "IPRD", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.IPRD) == false) return false; }// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "ICOP", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.ICOP) == false) return false; }// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "ICMT", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.ICMT) == false) return false; }// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "ISFT", 4) == 0) { if (ReadStringUsingLeadingSize(file, info.ISFT) == false) return false; }// ReadStringUsingLeadingSize takes care of the error report
            else if (strncmp(fourCC, "LIST", 4) == 0)
            {
                file.seek(file.position() - 4); // skip back
                return true;
            }
            else
            {
                // normally unknown blocks should be ignored
                if ((lastReadCount = file.read(&dummy, 4)) != 4) FILE_ERROR("read error - while getting unknown INFO block size")
                if (file.seek(file.position() + dummy) == false) FILE_ERROR("seek error - while skipping unknown INFO block")
            }
        }
        return true;
    }
};