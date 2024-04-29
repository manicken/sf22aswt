#pragma once

#include <Arduino.h>
#include <SD.h>

#include "sf22aswt_common.h"
#include "sf22aswt_structures.h"
#include "sf22aswt_enums.h"
#include "sf22aswt_helpers.h"
#include "sf22aswt_converter.h"

namespace SF22ASWT::reader_lazy::blocks
{
    /// <summary>
    /// reads data offset pointers and sizes of sample data, not the actual data
    /// as the data is read from file on demand
    /// </summary>
    /// <param name="br"></param>
    /// <returns></returns>
    bool read_sdta_block(File &file, sdta_rec_lazy &sdta)
    {
        char fourCC[4];
        while (file.available())
        {
            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR(SDTA_FOURCC_READ) //("read error - while getting sdtablock type")
            DebugPrintFOURCC(fourCC);
            if (verifyFourCC(fourCC) == false) FILE_ERROR(SDTA_FOURCC_INVALID) //("error - sdtablock type invalid")
            
            if (strncmp(fourCC, "smpl", 4) == 0)
            {
                if ((lastReadCount = file.read(&sdta.smpl.size, 4)) != 4) FILE_ERROR(SDTA_SMPL_SIZE_READ) //("read error - while reading smpl size")
                sdta.smpl.position = file.position();
                // skip sample data
                if (file.seek(sdta.smpl.size, SeekCur) == false) FILE_SEEK_ERROR(SDTA_SMPL_DATA_SKIP, sdta.smpl.size) //("seek error - while skipping smpl data")
            }
            else if (strncmp(fourCC, "sm24", 4) == 0)
            {
                if ((lastReadCount = file.read(&sdta.sm24.size, 4)) != 4) FILE_ERROR(SDTA_SM24_SIZE_READ) //("read error - while reading sm24 size")
                sdta.sm24.position = file.position();
                // skip sample data
                if (file.seek(sdta.sm24.size, SeekCur) == false) FILE_SEEK_ERROR(SDTA_SM24_DATA_SKIP, sdta.sm24.size) //("seek error - while skipping sm24 data")
            }
            else if (strncmp(fourCC, "LIST", 4) == 0)
            {
                // skip back
                if (file.seek(file.position()-4) == false) FILE_SEEK_ERROR(SDTA_BACK_SEEK, -4)
                return true;
            }
            else
            {
                // normally unknown blocks should be ignored
                uint32_t size = 0;
                if ((lastReadCount = file.read(&size, 4)) != 4) FILE_ERROR(SDTA_UNKNOWN_BLOCK_SIZE_READ) //("read error - while getting unknown sdta block size")
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(SDTA_UNKNOWN_BLOCK_DATA_SKIP, size) //("seek error - while skipping unknown sdta block")
            }
        }
        return true;
    }

    bool read_pdta_block(File &file, pdta_rec_lazy &pdta)
    {
        char fourCC[4];
        uint32_t size = 0;
        while (file.available())
        {
            if ((lastReadCount = file.readBytes(fourCC, 4)) != 4) FILE_ERROR(PDTA_FOURCC_READ) //("read error - while getting pdta block type")
            DebugPrintFOURCC(fourCC);
            if (verifyFourCC(fourCC) == false) FILE_ERROR(PDTA_FOURCC_INVALID) //("error - pdta type invalid")

            // store result for easier pinpoint of error handing
            bool sizeReadFail = ((lastReadCount = file.read(&size, 4)) != 4);

            if (strncmp(fourCC, "phdr", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_PHDR_SIZE_READ) //("read error - while getting pdta phdr block size")
                if (size % phdr_rec::Size != 0) FILE_ERROR(PDTA_PHDR_SIZE_MISMATCH) //("error - pdta phdr block size mismatch")

                pdta.phdr_count = size/phdr_rec::Size;
                pdta.phdr_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_PHDR_DATA_SKIP, size) //("seek error - while skipping phdr block")
            }
            else if (strncmp(fourCC, "pbag", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_PBAG_SIZE_READ) //("read error - while getting pdta pbag block size")
                if (size % bag_rec::Size != 0) FILE_ERROR(PDTA_PBAG_SIZE_MISMATCH) //("error - pdta pbag block size mismatch")

                pdta.pbag_count = size/bag_rec::Size;
                pdta.pbag_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_PBAG_DATA_SKIP, size) //("seek error - while skipping pbag block")
            }
            else if (strncmp(fourCC, "pmod", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_PMOD_SIZE_READ) //("read error - while getting pdta pmod block size")
                if (size % mod_rec::Size != 0) FILE_ERROR(PDTA_PMOD_SIZE_MISMATCH) //("error - pdta pmod block size mismatch")

                pdta.pmod_count = size/mod_rec::Size;
                pdta.pmod_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_PMOD_DATA_SKIP, size) //("seek error - while skipping pmod block")
            }
            else if (strncmp(fourCC, "pgen", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_PGEN_SIZE_READ) //("read error - while getting pdta pgen block size")
                if (size % gen_rec::Size != 0) FILE_ERROR(PDTA_PGEN_SIZE_MISMATCH) //("error - pdta pgen block size mismatch")

                pdta.pgen_count = size/gen_rec::Size;
                pdta.pgen_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_PGEN_DATA_SKIP, size) //("seek error - while skipping pgen block")
            }
            else if (strncmp(fourCC, "inst", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_INST_SIZE_READ) //("read error - while getting pdta inst block size")
                if (size % inst_rec::Size != 0) FILE_ERROR(PDTA_INST_SIZE_MISMATCH) //("error - pdta inst block size mismatch")

                pdta.inst_count = size/inst_rec::Size;
                pdta.inst_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_INST_DATA_SKIP, size) //("seek error - while skipping inst block")
            }
            else if (strncmp(fourCC, "ibag", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_IBAG_SIZE_READ) //("read error - while getting pdta ibag block size")
                if (size % bag_rec::Size != 0) FILE_ERROR(PDTA_IBAG_SIZE_MISMATCH) //("error - pdta ibag block size mismatch")

                pdta.ibag_count = size/bag_rec::Size;
                pdta.ibag_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_IBAG_DATA_SKIP, size) //("seek error - while skipping ibag block")
            }
            else if (strncmp(fourCC, "imod", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_IMOD_SIZE_READ) //("read error - while getting pdta imod block size")
                if (size % mod_rec::Size != 0) FILE_ERROR(PDTA_IMOD_SIZE_MISMATCH) //("error - pdta imod block size mismatch")

                pdta.imod_count = size/mod_rec::Size;
                pdta.imod_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_IMOD_DATA_SKIP, size) //("seek error - while skipping imod block")
            }
            else if (strncmp(fourCC, "igen", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_IGEN_SIZE_READ) //("read error - while getting pdta igen block size")
                if (size % gen_rec::Size != 0) FILE_ERROR(PDTA_IGEN_SIZE_MISMATCH) //("error - pdta igen block size mismatch")

                pdta.igen_count = size/gen_rec::Size;
                pdta.igen_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_IGEN_DATA_SKIP, size) //("seek error - while skipping igen block")
            }
            else if (strncmp(fourCC, "shdr", 4) == 0)
            {
                if (sizeReadFail) FILE_ERROR(PDTA_SHDR_SIZE_READ) //("read error - while getting pdta shdr block size")
                if (size % shdr_rec::Size != 0) FILE_ERROR(PDTA_SHDR_SIZE_MISMATCH) //("error - pdta shdr block size mismatch")

                pdta.shdr_count = size/shdr_rec::Size;
                pdta.shdr_position = file.position();
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_SHDR_DATA_SKIP, size) //("seek error - while skipping shdr block")
            }
            else if (strncmp(fourCC, "LIST", 4) == 0) // failsafe if file don't follow standard
            {
                // skip back
                if (file.seek(file.position()-8) == false) FILE_SEEK_ERROR(PDTA_BACK_SEEK, -8) 
                return true;
            }
            else
            {
                // normally unknown blocks should be ignored
                if (sizeReadFail) FILE_ERROR(PDTA_UNKNOWN_BLOCK_SIZE_READ) //("read error - while getting unknown block size")
                if (file.seek(size, SeekCur) == false) FILE_SEEK_ERROR(PDTA_UNKNOWN_BLOCK_DATA_SKIP, size) //("seek error - while skipping unknown pdta block")
            }
        }
        return true;
    }
}
