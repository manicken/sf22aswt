#pragma once

#include <Arduino.h>
//#include <SD.h>

#ifndef USerial
#define USerial Serial
#endif

namespace SerialFileRx
{
    String filePath = "";
    File *file = nullptr;
    bool inProgress = false;
    int fileSize = 0;
    #define SerialFileRx_BUFFER_SIZE (8 * CDC_RX_SIZE_480)
    char *buffer = nullptr;
    long lastTimeRxData = 0;

    // used to time file transfers
    long startTime = 0;
    long endTime = 0;

    void process_FileData();
    bool StartRxFile(const char* filePath, long size);
    void EndRxFile();

    void process_FileData()
    {
        int count = USerial.available();
        
        int bytesRead = USerial.readBytes(buffer, count);
        if (file != nullptr) file->write(buffer, count);

        fileSize -= bytesRead;
        if (fileSize <= 0) EndRxFile();    
        if (fileSize < 0) USerial.println("@ file end remaning fileSize was less than zero");

        lastTimeRxData = millis();    
    }

    bool StartRxFile(const char* _filePath, long size)
    {
        startTime = millis();
        filePath = String(_filePath);
        USerial.println("StartRxFile");
        if (SD.exists(_filePath)) // remove existing file
            SD.remove(_filePath);
        File _file = SD.open(_filePath, FILE_WRITE_BEGIN); // create file
        
        if (!_file) { USerial.println("could not open file"); return false; }
        //_file.close();
        file = new File(_file);//SD.open(_filePath, FILE_WRITE));

        fileSize = size;
        buffer = (char*)malloc(SerialFileRx_BUFFER_SIZE);
        if (buffer == nullptr) { USerial.println("could not allocate buffer"); return false; }
        inProgress = true;
        lastTimeRxData = millis();

        return true;
    }
    void EndRxFile()
    {
        USerial.println("EndRxFile");
        if (file != nullptr) file->close();
        delete file;
        file = nullptr;
        if (buffer != nullptr)
            free(buffer);
        inProgress = false;
        endTime = millis();
        USerial.print("transfer file took (in ms): ");
        USerial.println((endTime-startTime));
        USerial.println("json:{'cmd':'fileRxOK'}");
    }
}