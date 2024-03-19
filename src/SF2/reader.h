#include <Arduino.h>
#include "enums.h"
#include "structures.h"
#include <SD.h>

namespace SF2
{
    RIFF sfFile;
    String lastError;
    uint64_t lastErrorPosition;

    class SoundFontReader
    {
      public:
        File &file;
        bool autoCloseFileOnError = false;
        SoundFontReader(File file, bool autoCloseFileOnError):file(file),autoCloseFileOnError(autoCloseFileOnError) { }

        template<typename T>
        bool Read(T *val)
        {
            bool ret = file.read(val, sizeof(T)) == sizeof(T);
            if (ret == false && autoCloseFileOnError == true) file.close(); // auto close on error
            return ret;
        }

        bool ReadUInt32(uint32_t *val)
        {
            bool ret = file.read(val, sizeof(uint32_t)) == sizeof(uint32_t);
            if (ret == false && autoCloseFileOnError == true) file.close(); // auto close on error
            return ret;
        }

        
    };
  
    bool readInfoBlock(File file);

    bool ReadFile(String filePath)
    {
        File file = SD.open(filePath.c_str());
        if (!file) {  lastError = "warning - cannot open file " + filePath; return false; }
        sfFile.size = file.size();
        //SoundFontReader sfr(file, true);
        String fileTag = file.readString(4);
        if (fileTag != "RIFF") { file.close(); lastError = "warning - not a RIFF fileformat: " + fileTag; lastErrorPosition = file.position(); return false;}
        uint32_t size = 0;

        if (file.read(&sfFile.sfbk.size, 4) != 4) { lastError = "read error - while reading RIFF size"; lastErrorPosition = file.position(); return false; }

        String formatTag = file.readString(4);
        if (formatTag != "sfbk") { file.close(); lastError = "warning - not a sfbk fileformat: " + fileTag; lastErrorPosition = file.position(); return false;}

        String listType = "sfbk";
        while (file.available() > 0)
        {
            // every block starts with a LIST tag
            String listTag = file.readString(4);
            if (listTag != "LIST")
            {
                file.close();
                lastError = "LIST item not found after " + listType;
                lastErrorPosition = file.position();
                return false;
            }
            uint32_t listSize;
            if (file.read(&listSize, 4) != 4) { file.close(); lastError = "read error - while getting listSize"; lastErrorPosition = file.position(); return false; }

            listType = file.readString(4);
            SerialUSB1.println(listType);
            if (listType == "INFO")
            {
                sfFile.sfbk.info.size = listSize;
                SerialUSB1.println(file.available());
                readInfoBlock(file);// == false) {  lastError = "read error - while getting info block"; lastErrorPosition = file.position(); file.close(); return false; }
                
                file.close(); return true; // early return debug test
            }
            else if (listType == "sdta")
            {
                sfFile.sfbk.sdta.size = listSize;
                //if (read_sdta_block(br/*, fs.Position + listSize + 4*/) == false) return false;

                //return true; // early return debug test
            }
            else if (listType == "pdta")
            {
                sfFile.sfbk.pdta.size = listSize;
                //if (read_pdta_block(br/*, fs.Position + listSize + 4*/) == false) return false;
            }
        }

        file.close();
        return true;
    }

    String ReadStringUsingLeadingSize(File file)
    {
        uint32_t size;
        file.read(&size, 4);
        SerialUSB1.printf("string size:%ld",size);
        char bytes[size];
        file.readBytes(bytes, size);
        //String str = file.readString(size);
        return "dummy";
    }

    bool readInfoBlock(File file)
    {
        //INFO info = sfFile.sfbk.info; // simplify the usage
        //SerialUSB1.print("\navailable >>>"); SerialUSB1.println(file.available()); SerialUSB1.print("<<<\n");
        while (file.available() > 0)
        {
            
            char type[5];
            file.readBytes(type, 4);
            type[4] = 0;
            //String type = file.readString(4);
            SerialUSB1.print("\n>>>");
            for (int i=0;i<4;i++)
            {
                if (type[i] < 32 || type[i] > 126)
                {
                    SerialUSB1.print("[");
                    SerialUSB1.print(type[i],HEX);
                    SerialUSB1.print("]");
                }
                else
                    SerialUSB1.write(type[i]);
            } 
            SerialUSB1.print("<<<\n");
            
            //Debug.rtxt.AppendLine("type: " + type);
            uint32_t dummy = 0;
            if (strcmp(type, "ifil")){ file.read(&dummy, 4); file.read(&sfFile.sfbk.info.ifil, 4); }// file.read(&sfFile.sfbk.info.ifil.minor, 2);}
            else if (strcmp(type, "isng")) sfFile.sfbk.info.isng = ReadStringUsingLeadingSize(file);
            else if (strcmp(type, "INAM")) sfFile.sfbk.info.INAM = ReadStringUsingLeadingSize(file);
            else if (strcmp(type, "irom")) sfFile.sfbk.info.irom = ReadStringUsingLeadingSize(file);
            else if (strcmp(type, "iver")){ file.read(&dummy, 4); file.read(&sfFile.sfbk.info.ifil, 4); }// file.read(&sfFile.sfbk.info.ifil.minor, 2);}
            else if (strcmp(type, "ICRD")) sfFile.sfbk.info.ICRD = ReadStringUsingLeadingSize(file);
            else if (strcmp(type, "IENG")) sfFile.sfbk.info.IENG = ReadStringUsingLeadingSize(file);
            else if (strcmp(type, "IPRD")) sfFile.sfbk.info.IPRD = ReadStringUsingLeadingSize(file);
            else if (strcmp(type, "ICOP")) sfFile.sfbk.info.ICOP = ReadStringUsingLeadingSize(file);
            else if (strcmp(type, "ICMT")) sfFile.sfbk.info.ICMT = ReadStringUsingLeadingSize(file);
            else if (strcmp(type, "ISFT")) sfFile.sfbk.info.ISFT = ReadStringUsingLeadingSize(file);
            else if (strcmp(type, "LIST")) {
                file.seek(file.position() - 4); // skip back
                return true;
            }
        }
        return true;
    }
}