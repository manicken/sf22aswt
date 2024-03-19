#include <Arduino.h>
#include "enums.h"
#include "structures.h"
#include <SD.h>

namespace SF2
{
    RIFF sfFile;

    void ReadFile(String filePath)
    {
        File file = SD.open(filePath.c_str());
        String fileTag = file.readString(4);
        if (fileTag != "RIFF") { file.close(); SerialUSB1.println("{'log':'not a RIFF file'}"); return;}
        uint32_t size = 0;
        // TODO have this as a function
        if (file.readBytes((char *)&size, sizeof(uint32_t)) != sizeof(uint32_t))
        {
            SerialUSB1.println("{'log':'error while reading size'}");
            file.close();
            return;
        }
        SerialUSB1.print("{'log':'size:");
        SerialUSB1.print(size);
        SerialUSB1.println("'}");
        file.close();
    }
}