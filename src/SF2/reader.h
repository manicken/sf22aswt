#pragma once

#include <Arduino.h>
#include "enums.h"
#include "structures.h"
#include <SD.h>
#include "helpers.h"

#define USerial SerialUSB1

namespace SF2::reader
{
    sfbk_rec *sfbk;
    uint32_t fileSize;

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
  
    bool readInfoBlock(File &file, INFO &info);
    bool read_sdta_block(File &file);
    bool read_pdta_block(File &br);
    
    bool ReadFile(String filePath)
    {
        if (sfbk == nullptr) sfbk = new sfbk_rec();

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
            USerial.print(">>>"); Helpers::printRawBytes(fourCC, 4); USerial.print("<<< listsize: "); USerial.println(listSize);
            if (verifyFourCC(fourCC) == false) FILE_ERROR("error - invalid listType")
            
            
            if (strncmp(fourCC, "INFO", 4) == 0)
            {
                sfbk->info.size = listSize;
                if (readInfoBlock(file, sfbk->info) == false) return false;
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
                if (file.seek(file.position() + listSize - 4) == false) FILE_ERROR("seek error - while skipping unknown sfbk root block")
            }
        }

        file.close();
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
            if (strncmp(fourCC, "ifil", 4) == 0)
            {
                if ((lastReadCount = file.read(&dummy, 4)) != 4) FILE_ERROR("read error - ifil size dummy read")
                if ((lastReadCount = file.read(&info.ifil, 4)) != 4) FILE_ERROR("read error - while ifil read")
            }
            else if (strncmp(fourCC, "isng", 4) == 0)
            {
                if (ReadStringUsingLeadingSize(file, info.isng) == false) return false; // ReadStringUsingLeadingSize takes care of the error report
            }
            else if (strncmp(fourCC, "INAM", 4) == 0)
            {
                if (ReadStringUsingLeadingSize(file, info.INAM) == false) return false; // ReadStringUsingLeadingSize takes care of the error report
            }
            else if (strncmp(fourCC, "irom", 4) == 0)
            {
                if (ReadStringUsingLeadingSize(file, info.irom) == false) return false; // ReadStringUsingLeadingSize takes care of the error report
            }
            else if (strncmp(fourCC, "iver", 4) == 0)
            {
                if ((lastReadCount = file.read(&dummy, 4)) != 4) FILE_ERROR("read error - iver size dummy read")
                if ((lastReadCount = file.read(&info.iver, 4)) != 4) FILE_ERROR("read error - while iver read")
            }
            else if (strncmp(fourCC, "ICRD", 4) == 0)
            {
                if (ReadStringUsingLeadingSize(file, info.ICRD) == false) return false; // ReadStringUsingLeadingSize takes care of the error report
            }
            else if (strncmp(fourCC, "IENG", 4) == 0)
            {
                if (ReadStringUsingLeadingSize(file, info.IENG) == false) return false; // ReadStringUsingLeadingSize takes care of the error report
            }
            else if (strncmp(fourCC, "IPRD", 4) == 0)
            {
                if (ReadStringUsingLeadingSize(file, info.IPRD) == false) return false; // ReadStringUsingLeadingSize takes care of the error report
            }
            else if (strncmp(fourCC, "ICOP", 4) == 0)
            {
                if (ReadStringUsingLeadingSize(file, info.ICOP) == false) return false; // ReadStringUsingLeadingSize takes care of the error report
            }
            else if (strncmp(fourCC, "ICMT", 4) == 0)
            {
                if (ReadStringUsingLeadingSize(file, info.ICMT) == false) return false; // ReadStringUsingLeadingSize takes care of the error report
            }
            else if (strncmp(fourCC, "ISFT", 4) == 0)
            {
                if (ReadStringUsingLeadingSize(file, info.ISFT) == false) return false; // ReadStringUsingLeadingSize takes care of the error report
            }
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
            USerial.print(">>>"); Helpers::printRawBytes(fourCC, 4); USerial.print("<<<\n");
            if (verifyFourCC(fourCC) == false) FILE_ERROR("error - infoblock type invalid")
            
            if (strncmp(fourCC, "smpl", 4) == 0)
            {
                if ((lastReadCount = file.read(&sfbk->sdta.smpl.size, 4)) != 4) FILE_ERROR("read error - while reading smpl size")
                sfbk->sdta.smpl.position = file.position();
                // skip sample data
                if (file.seek( file.position() + sfbk->sdta.smpl.size) == false) FILE_ERROR("seek error - while skipping smpl data")
            }
            else if (strncmp(fourCC, "sm24", 4) == 0)
            {
                if ((lastReadCount = file.read(&sfbk->sdta.sm24.size, 4)) != 4) FILE_ERROR("read error - while reading sm24 size")
                sfbk->sdta.sm24.position = file.position();
                // skip sample data
                if (file.seek( file.position() + sfbk->sdta.sm24.size) == false) FILE_ERROR("seek error - while skipping sm24 data")
            }
            else if (strncmp(fourCC, "LIST", 4) == 0)
            {
                file.seek(file.position() - 4); // skip back
                return true;
            }
            else
            {
                // normally unknown blocks should be ignored
                uint32_t size = 0;
                if ((lastReadCount = file.read(&size, 4)) != 4) FILE_ERROR("read error - while getting unknown sdta block size")
                if (file.seek(file.position() + size) == false) FILE_ERROR("seek error - while skipping unknown sdta block")
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
            USerial.print(">>>"); Helpers::printRawBytes(fourCC, 4); USerial.print("<<<\n");
            if (verifyFourCC(fourCC) == false) FILE_ERROR("error - pdta type invalid")

            if ((lastReadCount = file.read(&size, 4)) != 4) FILE_ERROR("read error - while getting pdta block size")

            if (strncmp(fourCC, "phdr", 4) == 0)
            {
                if (size % phdr_rec::Size != 0) FILE_ERROR("error - pdta phdr block size mismatch")

                sfbk->pdta.phdr_count = size/phdr_rec::Size;
                sfbk->pdta.phdr = new phdr_rec[sfbk->pdta.phdr_count];
                for (uint32_t i = 0; i < sfbk->pdta.phdr_count; i++) {
                    if ((lastReadCount = file.read(&sfbk->pdta.phdr[i], phdr_rec::Size)) != phdr_rec::Size) FILE_ERROR("read error - while reading pdta phdr record")
                }
            }
            else if (strncmp(fourCC, "pbag", 4) == 0)
            {
                if (size % bag_rec::Size != 0) FILE_ERROR("error - pdta pbag block size mismatch")

                sfbk->pdta.pbag_count = size/bag_rec::Size;
                sfbk->pdta.pbag = new bag_rec[sfbk->pdta.pbag_count];
                for (uint32_t i = 0; i < sfbk->pdta.pbag_count; i++) {
                    if ((lastReadCount = file.read(&sfbk->pdta.pbag[i], bag_rec::Size)) != bag_rec::Size) FILE_ERROR("read error - while reading pdta pbag record")
                }
            }
            else if (strncmp(fourCC, "pmod", 4) == 0)
            {
                if (size % mod_rec::Size != 0) FILE_ERROR("error - pdta pmod block size mismatch")

                sfbk->pdta.pmod_count = size/mod_rec::Size;
                sfbk->pdta.pmod = new mod_rec[sfbk->pdta.pmod_count];
                for (uint32_t i = 0; i < sfbk->pdta.pmod_count; i++) {
                    if ((lastReadCount = file.read(&sfbk->pdta.pmod[i], mod_rec::Size)) != mod_rec::Size) FILE_ERROR("read error - while reading pdta pmod record")
                }
            }
            else if (strncmp(fourCC, "pgen", 4) == 0)
            {
                if (size % gen_rec::Size != 0) FILE_ERROR("error - pdta pgen block size mismatch")

                sfbk->pdta.pgen_count = size/gen_rec::Size;
                sfbk->pdta.pgen = new gen_rec[sfbk->pdta.pgen_count];
                for (uint32_t i = 0; i < sfbk->pdta.pgen_count; i++) {
                    if ((lastReadCount = file.read(&sfbk->pdta.pgen[i], gen_rec::Size)) != gen_rec::Size) FILE_ERROR("read error - while reading pdta pgen record")
                }
            }
            else if (strncmp(fourCC, "inst", 4) == 0)
            {
                if (size % inst_rec::Size != 0) FILE_ERROR("error - pdta inst block size mismatch")

                sfbk->pdta.inst_count = size/inst_rec::Size;
                sfbk->pdta.inst = new inst_rec[sfbk->pdta.inst_count];
                for (uint32_t i = 0; i < sfbk->pdta.inst_count; i++) {
                    if ((lastReadCount = file.read(&sfbk->pdta.inst[i], inst_rec::Size)) != inst_rec::Size) FILE_ERROR("read error - while reading pdta inst record")
                }
            }
            else if (strncmp(fourCC, "ibag", 4) == 0)
            {
                if (size % bag_rec::Size != 0) FILE_ERROR("error - pdta ibag block size mismatch")

                sfbk->pdta.ibag_count = size/bag_rec::Size;
                sfbk->pdta.ibag = new bag_rec[sfbk->pdta.ibag_count];
                for (uint32_t i = 0; i < sfbk->pdta.ibag_count; i++) {
                    if ((lastReadCount = file.read(&sfbk->pdta.ibag[i], bag_rec::Size)) != bag_rec::Size) FILE_ERROR("read error - while reading pdta ibag record")
                }
            }
            else if (strncmp(fourCC, "imod", 4) == 0)
            {
                if (size % mod_rec::Size != 0) FILE_ERROR("error - pdta imod block size mismatch")

                sfbk->pdta.imod_count = size/mod_rec::Size;
                sfbk->pdta.imod = new mod_rec[sfbk->pdta.imod_count];
                for (uint32_t i = 0; i < sfbk->pdta.imod_count; i++) {
                    if ((lastReadCount = file.read(&sfbk->pdta.imod[i], mod_rec::Size)) != mod_rec::Size) FILE_ERROR("read error - while reading pdta imod record")
                }
            }
            else if (strncmp(fourCC, "igen", 4) == 0)
            {
                if (size % gen_rec::Size != 0) FILE_ERROR("error - pdta igen block size mismatch")

                sfbk->pdta.igen_count = size/gen_rec::Size;
                sfbk->pdta.igen = new gen_rec[sfbk->pdta.igen_count];
                for (uint32_t i = 0; i < sfbk->pdta.igen_count; i++) {
                    if ((lastReadCount = file.read(&sfbk->pdta.igen[i], gen_rec::Size)) != gen_rec::Size) FILE_ERROR("read error - while reading pdta igen record")
                }
            }
            else if (strncmp(fourCC, "shdr", 4) == 0)
            {
                if (size % shdr_rec::Size != 0) FILE_ERROR("error - pdta shdr block size mismatch")

                sfbk->pdta.shdr_count = size/shdr_rec::Size;
                sfbk->pdta.shdr = new shdr_rec[sfbk->pdta.shdr_count];
                for (uint32_t i = 0; i < sfbk->pdta.shdr_count; i++) {
                    if ((lastReadCount = file.read(&sfbk->pdta.shdr[i], shdr_rec::Size)) != shdr_rec::Size) FILE_ERROR("read error - while reading pdta shdr record")
                }
            }
            else if (strncmp(fourCC, "LIST", 4) == 0) // failsafe if file don't follow standard
            {
                file.seek(file.position() - 4); // skip back
                return true;
            }
            else
            {
                // normally unknown blocks should be ignored
                if (file.seek(file.position() + size) == false) FILE_ERROR("seek error - while skipping unknown pdta block")
            }
        }
        return true;
    }
}