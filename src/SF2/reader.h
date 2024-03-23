#include <Arduino.h>
#include "enums.h"
#include "structures.h"
#include <SD.h>

#define USerial SerialUSB1

namespace SF2
{
    RIFF sfFile;
    String lastError; // TODO: change string lastError into a enum
    uint64_t lastErrorPosition;
    size_t lastReadCount = 0; // used to track errors
    #define FILE_ERROR(msg) {lastError=msg; lastErrorPosition = file.position() - lastReadCount; file.close(); return false;}

    void printRawBytes(const char* bytes, size_t length)
    {
        for (int i=0;i<length;i++)
        {
            if (bytes[i] < 32 || bytes[i] > 126)
            {
                USerial.print("[");
                USerial.print(bytes[i],HEX);
                USerial.print("]");
            }
            else
                USerial.write(bytes[i]);
        }
    }

    bool ReadStringUsingLeadingSize(File file, String& string)
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
  
    bool readInfoBlock(File file);
    bool read_sdta_block(File file);
    bool read_pdta_block(File br);

    
    bool ReadFile(String filePath)
    {
        USerial.println(sizeof(phdr_rec));

        File file = SD.open(filePath.c_str());
        if (!file) {  lastError = "warning - cannot open file " + filePath; return false; }
        sfFile.size = file.size();

        char fourCC[4];

        if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR("read error - while reading fileTag")
        if (verifyFourCC(fourCC) == false) FILE_ERROR("error - invalid fileTag")
        if (strncmp(fourCC, "RIFF", 4) != 0) FILE_ERROR("error - not a RIFF fileformat")
        
        if ((lastReadCount = file.read(&sfFile.sfbk.size, 4)) != 4) FILE_ERROR("read error - while reading RIFF size")
        if (sfFile.size != (sfFile.sfbk.size + 8)) FILE_ERROR("error - fileSize mismatch")

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
            USerial.print(">>>"); printRawBytes(fourCC, 4); USerial.print("<<< listsize: "); USerial.println(listSize);
            if (verifyFourCC(fourCC) == false) FILE_ERROR("error - invalid listType")
            
            
            if (strncmp(fourCC, "INFO", 4) == 0)
            {
                sfFile.sfbk.info.size = listSize;
                if (readInfoBlock(file) == false) return false;
                //file.close(); return true; // early return debug test
            }
            else if (strncmp(fourCC, "sdta", 4) == 0)
            {
                sfFile.sfbk.sdta.size = listSize;
                if (read_sdta_block(file) == false) return false;
                //file.close(); return true; // early return debug test
            }
            else if (strncmp(fourCC, "pdta", 4) == 0)
            {
                sfFile.sfbk.pdta.size = listSize;
                if (read_pdta_block(file) == false) return false;
                //file.close(); return true; // early return debug test
            }
            else
            {
                FILE_ERROR("error - unknown listType")
            }
        }

        file.close();
        return true;
    }

    bool readInfoBlock(File file)
    {
        //SerialUSB1.print("\navailable >>>"); SerialUSB1.println(file.available()); SerialUSB1.print("<<<\n");
        char fourCC[4];
        while (file.available() > 0)
        {
            //DSerial.printf("\n  file position: %ld\n", file.position());
            
            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR("read error - while getting infoblock type")
            USerial.print(">>>"); printRawBytes(fourCC, 4); USerial.print("<<<\n");
            if (verifyFourCC(fourCC) == false) FILE_ERROR("error - infoblock type invalid")

            uint32_t dummy = 0;
            if (strncmp(fourCC, "ifil", 4) == 0)
            {
                if ((lastReadCount = file.read(&dummy, 4)) != 4) FILE_ERROR("read error - ifil size dummy read")
                if ((lastReadCount = file.read(&sfFile.sfbk.info.ifil, 4)) != 4) FILE_ERROR("read error - while ifil read")
            }
            else if (strncmp(fourCC, "isng", 4) == 0) { if (ReadStringUsingLeadingSize(file, sfFile.sfbk.info.isng) == false) return false; }
            else if (strncmp(fourCC, "INAM", 4) == 0) { if (ReadStringUsingLeadingSize(file, sfFile.sfbk.info.INAM) == false) return false; }
            else if (strncmp(fourCC, "irom", 4) == 0) { if (ReadStringUsingLeadingSize(file, sfFile.sfbk.info.irom) == false) return false; }
            else if (strncmp(fourCC, "iver", 4) == 0)
            {
                if ((lastReadCount = file.read(&dummy, 4)) != 4) FILE_ERROR("read error - iver size dummy read")
                if ((lastReadCount = file.read(&sfFile.sfbk.info.iver, 4)) != 4) FILE_ERROR("read error - while iver read")
            }
            else if (strncmp(fourCC, "ICRD", 4) == 0) { if (ReadStringUsingLeadingSize(file, sfFile.sfbk.info.ICRD) == false) return false; }
            else if (strncmp(fourCC, "IENG", 4) == 0) { if (ReadStringUsingLeadingSize(file, sfFile.sfbk.info.IENG) == false) return false; }
            else if (strncmp(fourCC, "IPRD", 4) == 0) { if (ReadStringUsingLeadingSize(file, sfFile.sfbk.info.IPRD) == false) return false; }
            else if (strncmp(fourCC, "ICOP", 4) == 0) { if (ReadStringUsingLeadingSize(file, sfFile.sfbk.info.ICOP) == false) return false; }
            else if (strncmp(fourCC, "ICMT", 4) == 0) { if (ReadStringUsingLeadingSize(file, sfFile.sfbk.info.ICMT) == false) return false; }
            else if (strncmp(fourCC, "ISFT", 4) == 0) { if (ReadStringUsingLeadingSize(file, sfFile.sfbk.info.ISFT) == false) return false; }
            else if (strncmp(fourCC, "LIST", 4) == 0) {
                file.seek(file.position() - 4); // skip back
                return true;
            }
            else
            {
                FILE_ERROR("error - unknown info block type")
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
    bool read_sdta_block(File file)
    {
        char fourCC[4];
        while (file.available())
        {
            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR("read error - while getting infoblock type")
            USerial.print(">>>"); printRawBytes(fourCC, 4); USerial.print("<<<\n");
            if (verifyFourCC(fourCC) == false) FILE_ERROR("error - infoblock type invalid")
            
            if (strncmp(fourCC, "smpl", 4) == 0)
            {
                if ((lastReadCount = file.read(&sfFile.sfbk.sdta.smpl.size, 4)) != 4) FILE_ERROR("read error - while reading smpl size")
                sfFile.sfbk.sdta.smpl.position = file.position();
                // skip sample data
                if (file.seek( file.position() + sfFile.sfbk.sdta.smpl.size) == false) FILE_ERROR("seek error - while skipping smpl data")
            }
            else if (strncmp(fourCC, "sm24", 4) == 0)
            {
                if ((lastReadCount = file.read(&sfFile.sfbk.sdta.sm24.size, 4)) != 4) FILE_ERROR("read error - while reading sm24 size")
                sfFile.sfbk.sdta.sm24.position = file.position();
                // skip sample data
                if (file.seek( file.position() + sfFile.sfbk.sdta.sm24.size) == false) FILE_ERROR("seek error - while skipping sm24 data")
            }
            else if (strncmp(fourCC, "LIST", 4) == 0)
            {
                file.seek(file.position() - 4); // skip back
                return true;
            }
            else
            {
                FILE_ERROR("error - unknown sdta block type")
            }
        }
        return true;
    }

    bool read_pdta_block(File file)
    {
        char fourCC[4];
        uint32_t size = 0;
        while (file.available())
        {
            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR("read error - while getting pdta block type")
            USerial.print(">>>"); printRawBytes(fourCC, 4); USerial.print("<<<\n");
            if (verifyFourCC(fourCC) == false) FILE_ERROR("error - pdta type invalid")

            if ((lastReadCount = file.read(&size, 4)) != 4) FILE_ERROR("read error - while getting pdta block size")

            if (strncmp(fourCC, "phdr", 4) == 0)
            {
                if (size % phdr_rec::Size != 0) FILE_ERROR("error - pdta phdr block size mismatch")

                sfFile.sfbk.pdta.phdr_count = size/phdr_rec::Size;
                sfFile.sfbk.pdta.phdr = new phdr_rec[sfFile.sfbk.pdta.phdr_count];
                for (uint32_t i = 0; i < sfFile.sfbk.pdta.phdr_count; i++) {
                    if ((lastReadCount = file.read(&sfFile.sfbk.pdta.phdr[i], phdr_rec::Size)) != phdr_rec::Size) FILE_ERROR("read error - while reading pdta phdr record")
                }
            }
            else if (strncmp(fourCC, "pbag", 4) == 0)
            {
                if (size % bag_rec::Size != 0) FILE_ERROR("error - pdta pbag block size mismatch")

                sfFile.sfbk.pdta.pbag_count = size/bag_rec::Size;
                sfFile.sfbk.pdta.pbag = new bag_rec[sfFile.sfbk.pdta.pbag_count];
                for (uint32_t i = 0; i < sfFile.sfbk.pdta.pbag_count; i++) {
                    if ((lastReadCount = file.read(&sfFile.sfbk.pdta.pbag[i], bag_rec::Size)) != bag_rec::Size) FILE_ERROR("read error - while reading pdta pbag record")
                }
            }
            else if (strncmp(fourCC, "pmod", 4) == 0)
            {
                if (size % mod_rec::Size != 0) FILE_ERROR("error - pdta pmod block size mismatch")

                sfFile.sfbk.pdta.pmod_count = size/mod_rec::Size;
                sfFile.sfbk.pdta.pmod = new mod_rec[sfFile.sfbk.pdta.pmod_count];
                for (uint32_t i = 0; i < sfFile.sfbk.pdta.pmod_count; i++) {
                    if ((lastReadCount = file.read(&sfFile.sfbk.pdta.pmod[i], mod_rec::Size)) != mod_rec::Size) FILE_ERROR("read error - while reading pdta pmod record")
                }
            }
            else if (strncmp(fourCC, "pgen", 4) == 0)
            {
                if (size % gen_rec::Size != 0) FILE_ERROR("error - pdta pgen block size mismatch")

                sfFile.sfbk.pdta.pgen_count = size/gen_rec::Size;
                sfFile.sfbk.pdta.pgen = new gen_rec[sfFile.sfbk.pdta.pgen_count];
                for (uint32_t i = 0; i < sfFile.sfbk.pdta.pgen_count; i++) {
                    if ((lastReadCount = file.read(&sfFile.sfbk.pdta.pgen[i], gen_rec::Size)) != gen_rec::Size) FILE_ERROR("read error - while reading pdta pgen record")
                }
            }
            else if (strncmp(fourCC, "inst", 4) == 0)
            {
                if (size % inst_rec::Size != 0) FILE_ERROR("error - pdta inst block size mismatch")

                sfFile.sfbk.pdta.inst_count = size/inst_rec::Size;
                sfFile.sfbk.pdta.inst = new inst_rec[sfFile.sfbk.pdta.inst_count];
                for (uint32_t i = 0; i < sfFile.sfbk.pdta.inst_count; i++) {
                    if ((lastReadCount = file.read(&sfFile.sfbk.pdta.inst[i], inst_rec::Size)) != inst_rec::Size) FILE_ERROR("read error - while reading pdta inst record")
                }
            }
            else if (strncmp(fourCC, "ibag", 4) == 0)
            {
                if (size % bag_rec::Size != 0) FILE_ERROR("error - pdta ibag block size mismatch")

                sfFile.sfbk.pdta.ibag_count = size/bag_rec::Size;
                sfFile.sfbk.pdta.ibag = new bag_rec[sfFile.sfbk.pdta.ibag_count];
                for (uint32_t i = 0; i < sfFile.sfbk.pdta.ibag_count; i++) {
                    if ((lastReadCount = file.read(&sfFile.sfbk.pdta.ibag[i], bag_rec::Size)) != bag_rec::Size) FILE_ERROR("read error - while reading pdta ibag record")
                }
            }
            else if (strncmp(fourCC, "imod", 4) == 0)
            {
                if (size % mod_rec::Size != 0) FILE_ERROR("error - pdta imod block size mismatch")

                sfFile.sfbk.pdta.imod_count = size/mod_rec::Size;
                sfFile.sfbk.pdta.imod = new mod_rec[sfFile.sfbk.pdta.imod_count];
                for (uint32_t i = 0; i < sfFile.sfbk.pdta.imod_count; i++) {
                    if ((lastReadCount = file.read(&sfFile.sfbk.pdta.imod[i], mod_rec::Size)) != mod_rec::Size) FILE_ERROR("read error - while reading pdta imod record")
                }
            }
            else if (strncmp(fourCC, "igen", 4) == 0)
            {
                if (size % gen_rec::Size != 0) FILE_ERROR("error - pdta igen block size mismatch")

                sfFile.sfbk.pdta.igen_count = size/gen_rec::Size;
                sfFile.sfbk.pdta.igen = new gen_rec[sfFile.sfbk.pdta.igen_count];
                for (uint32_t i = 0; i < sfFile.sfbk.pdta.igen_count; i++) {
                    if ((lastReadCount = file.read(&sfFile.sfbk.pdta.igen[i], gen_rec::Size)) != gen_rec::Size) FILE_ERROR("read error - while reading pdta igen record")
                }
            }
            else if (strncmp(fourCC, "shdr", 4) == 0)
            {
                if (size % shdr_rec::Size != 0) FILE_ERROR("error - pdta shdr block size mismatch")

                sfFile.sfbk.pdta.shdr_count = size/shdr_rec::Size;
                sfFile.sfbk.pdta.shdr = new shdr_rec[sfFile.sfbk.pdta.shdr_count];
                for (uint32_t i = 0; i < sfFile.sfbk.pdta.shdr_count; i++) {
                    if ((lastReadCount = file.read(&sfFile.sfbk.pdta.shdr[i], shdr_rec::Size)) != shdr_rec::Size) FILE_ERROR("read error - while reading pdta shdr record")
                }
            }
            else if (strncmp(fourCC, "LIST", 4) == 0) // failsafe if file don't follow standard
            {
                file.seek(file.position() - 4); // skip back
                return true;
            }
            else
            {
                FILE_ERROR("error - unknown pdta block type")
            }
        }
        return true;
    }
}