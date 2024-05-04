#pragma once

#include <Arduino.h>
#include <SD.h>
#include "sf22aswt_enums.h"
#include "sf22aswt_structures.h"
#include "sf22aswt_helpers.h"
#include "sf22aswt_common.h"
#include "sf22aswt_converter.h"

#ifdef DEBUG
  #define USerial SerialUSB1
  #define DebugPrint(args) USerial.print(args);
  #define DebugPrintln(args) USerial.println(args);
  #define DebugPrint_Text_Var(text, var) USerial.print(text); USerial.print(var);
  #define DebugPrintln_Text_Var(text, var) USerial.print(text); USerial.println(var);
  #define DebugPrintFOURCC(fourCC) USerial.print(">>>"); Helpers::printRawBytes(USerial, fourCC, 4); USerial.println("<<<");
  #define DebugPrintFOURCC_size(size) USerial.print("size: "); USerial.print(size);  USerial.print("\n");
#else
  #define DebugPrint(args)
  #define DebugPrintln(args)
  #define DebugPrint_Text_Var(text, var)
  #define DebugPrintln_Text_Var(text, var)
  #define DebugPrintFOURCC(fourCC)
  #define DebugPrintFOURCC_size(size)
#endif

namespace SF22ASWT
{
    class Reader : public SF22ASWT::common
    {
      public:
        sfbk_rec sfbk;

        bool ReadFile(String filePath);
        bool read_pdta_block(File &br);
    };
}