#pragma once

#include <Arduino.h>
#define USerial SerialUSB1
namespace Helpers
{
    void printRawBytes(const char* bytes, size_t length)
    {
        for (size_t i=0;i<length;i++)
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
    void printRawBytesSanitized(const char* bytes, size_t length)
    {
        for (size_t i=0;i<length;i++)
        {
            if (bytes[i] < 32 || bytes[i] > 126)
                USerial.write(' ');
            else
                USerial.write(bytes[i]);
        }
    }
    void printRawBytesSanitizedUntil(const char* bytes, size_t length, char untilchar)
    {
        for (size_t i=0;i<length;i++)
        {
            if (bytes[i] == untilchar) return;
            if (bytes[i] < 32 || bytes[i] > 126)
                USerial.write(' ');
            else
                USerial.write(bytes[i]);
        }
    }
    void printRawBytesUntil(const char* bytes, size_t length, char untilchar)
    {
        for (size_t i=0;i<length;i++)
        {
            if (bytes[i] == untilchar) return;
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
}