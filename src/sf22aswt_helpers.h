#pragma once

#include <Arduino.h>

namespace Helpers
{
    void printRawBytes(Stream &str, const char* bytes, size_t length)
    {
        for (size_t i=0;i<length;i++)
        {
            if (bytes[i] < 32 || bytes[i] > 126)
            {
                str.print("[");
                str.print(bytes[i],HEX);
                str.print("]");
            }
            else
                str.write(bytes[i]);
        }
    }
    void printRawBytesSanitized(Stream &str, const char* bytes, size_t length)
    {
        for (size_t i=0;i<length;i++)
        {
            if (bytes[i] < 32 || bytes[i] > 126)
                str.write(' ');
            else
                str.write(bytes[i]);
        }
    }
    void printRawBytesSanitizedUntil(Stream &str, const char* bytes, size_t length, char untilchar)
    {
        for (size_t i=0;i<length;i++)
        {
            if (bytes[i] == untilchar) return;
            if (bytes[i] < 32 || bytes[i] > 126)
                str.write(' ');
            else
                str.write(bytes[i]);
        }
    }
    void printRawBytesUntil(Stream &str, const char* bytes, size_t length, char untilchar)
    {
        for (size_t i=0;i<length;i++)
        {
            if (bytes[i] == untilchar) return;
            if (bytes[i] < 32 || bytes[i] > 126)
            {
                str.print("[");
                str.print(bytes[i],HEX);
                str.print("]");
            }
            else
                str.write(bytes[i]);
        }
    }
}