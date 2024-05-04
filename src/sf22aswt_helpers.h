#pragma once

#include <Arduino.h>



namespace Helpers
{
    void printRawBytes(Print &printStream, const char* bytes, size_t length);
    void printRawBytesSanitized(Print &printStream, const char* bytes, size_t length);
    void printRawBytesSanitizedUntil(Print &printStream, const char* bytes, size_t length, char untilchar);
    void printRawBytesUntil(Print &printStream, const char* bytes, size_t length, char untilchar);
}