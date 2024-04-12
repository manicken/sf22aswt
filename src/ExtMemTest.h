#pragma once

#include <Arduino.h>

#define USerial SerialUSB

namespace ExtMemTest
{
    extern "C" uint8_t external_psram_size;

    bool memory_ok = false;
    uint32_t *memory_begin, *memory_end;

    bool check_fixed_pattern(uint32_t pattern);
    bool check_lfsr_pattern(uint32_t seed);

    bool exec()
    {
        uint8_t size = external_psram_size;
        USerial.printf("\nEXTMEM Memory Test, %d Mbyte\n", size);
        if (size == 0) return false;
        const float clocks[4] = {396.0f, 720.0f, 664.62f, 528.0f};
        const float frequency = clocks[(CCM_CBCMR >> 8) & 3] / (float)(((CCM_CBCMR >> 29) & 7) + 1);
        USerial.printf(" CCM_CBCMR=%08X (%.1f MHz)\n", CCM_CBCMR, frequency);
        memory_begin = (uint32_t *)(0x70000000);
        memory_end = (uint32_t *)(0x70000000 + size * 1048576);
        elapsedMillis msec = 0;
        if (!check_fixed_pattern(0x5A698421)) return false; // 1
        if (!check_lfsr_pattern(2976674124ul)) return false; // 2 
        if (!check_lfsr_pattern(1438200953ul)) return false; // 3
        if (!check_lfsr_pattern(3413783263ul)) return false; // 4
        if (!check_lfsr_pattern(1900517911ul)) return false; // 5
        if (!check_lfsr_pattern(1227909400ul)) return false; // 6
        if (!check_lfsr_pattern(276562754ul)) return false; // 7
        if (!check_lfsr_pattern(146878114ul)) return false; // 8
        if (!check_lfsr_pattern(615545407ul)) return false; // 9
        if (!check_lfsr_pattern(110497896ul)) return false; // 10
        if (!check_lfsr_pattern(74539250ul)) return false; // 11
        if (!check_lfsr_pattern(4197336575ul)) return false; // 12
        if (!check_lfsr_pattern(2280382233ul)) return false; // 13
        if (!check_lfsr_pattern(542894183ul)) return false; // 14
        if (!check_lfsr_pattern(3978544245ul)) return false; // 15
        if (!check_lfsr_pattern(2315909796ul)) return false; // 16
        if (!check_lfsr_pattern(3736286001ul)) return false; // 17
        if (!check_lfsr_pattern(2876690683ul)) return false; // 18
        if (!check_lfsr_pattern(215559886ul)) return false; // 19
        if (!check_lfsr_pattern(539179291ul)) return false; // 20
        if (!check_lfsr_pattern(537678650ul)) return false; // 21
        if (!check_lfsr_pattern(4001405270ul)) return false; // 22
        if (!check_lfsr_pattern(2169216599ul)) return false; // 23
        if (!check_lfsr_pattern(4036891097ul)) return false; // 24
        if (!check_lfsr_pattern(1535452389ul)) return false; // 25
        if (!check_lfsr_pattern(2959727213ul)) return false; // 26
        if (!check_lfsr_pattern(4219363395ul)) return false; // 27
        if (!check_lfsr_pattern(1036929753ul)) return false; // 28
        if (!check_lfsr_pattern(2125248865ul)) return false; // 29
        if (!check_lfsr_pattern(3177905864ul)) return false; // 30
        if (!check_lfsr_pattern(2399307098ul)) return false; // 31
        if (!check_lfsr_pattern(3847634607ul)) return false; // 32
        if (!check_lfsr_pattern(27467969ul)) return false; // 33
        if (!check_lfsr_pattern(520563506ul)) return false; // 34
        if (!check_lfsr_pattern(381313790ul)) return false; // 35
        if (!check_lfsr_pattern(4174769276ul)) return false; // 36
        if (!check_lfsr_pattern(3932189449ul)) return false; // 37
        if (!check_lfsr_pattern(4079717394ul)) return false; // 38
        if (!check_lfsr_pattern(868357076ul)) return false; // 39
        if (!check_lfsr_pattern(2474062993ul)) return false; // 40
        if (!check_lfsr_pattern(1502682190ul)) return false; // 41
        if (!check_lfsr_pattern(2471230478ul)) return false; // 42
        if (!check_lfsr_pattern(85016565ul)) return false; // 43
        if (!check_lfsr_pattern(1427530695ul)) return false; // 44
        if (!check_lfsr_pattern(1100533073ul)) return false; // 45
        if (!check_fixed_pattern(0x55555555)) return false; // 46
        if (!check_fixed_pattern(0x33333333)) return false; // 47
        if (!check_fixed_pattern(0x0F0F0F0F)) return false; // 48
        if (!check_fixed_pattern(0x00FF00FF)) return false; // 49
        if (!check_fixed_pattern(0x0000FFFF)) return false; // 50
        if (!check_fixed_pattern(0xAAAAAAAA)) return false; // 51
        if (!check_fixed_pattern(0xCCCCCCCC)) return false; // 52
        if (!check_fixed_pattern(0xF0F0F0F0)) return false; // 53
        if (!check_fixed_pattern(0xFF00FF00)) return false; // 54
        if (!check_fixed_pattern(0xFFFF0000)) return false; // 55
        if (!check_fixed_pattern(0xFFFFFFFF)) return false; // 56
        if (!check_fixed_pattern(0x00000000)) return false; // 57
        USerial.printf(" test ran for %.2f seconds\n", (float)msec / 1000.0f);
        USerial.print("All memory tests passed :-)\n");
        memory_ok = true;
    }

    bool fail_message(volatile uint32_t *location, uint32_t actual, uint32_t expected)
    {
        USerial.printf(" Error at %08X, read %08X but expected %08X\n",
            (uint32_t)location, actual, expected);
        return false;
    }

    // fill the entire RAM with a fixed pattern, then check it
    bool check_fixed_pattern(uint32_t pattern)
    {
        volatile uint32_t *p;
        USerial.printf("testing with fixed pattern %08X\n", pattern);
        for (p = memory_begin; p < memory_end; p++) {
            *p = pattern;
        }
        arm_dcache_flush_delete((void *)memory_begin,
            (uint32_t)memory_end - (uint32_t)memory_begin);
        for (p = memory_begin; p < memory_end; p++) {
            uint32_t actual = *p;
            if (actual != pattern) return fail_message(p, actual, pattern);
        }
        return true;
    }

    // fill the entire RAM with a pseudo-random sequence, then check it
    bool check_lfsr_pattern(uint32_t seed)
    {
        volatile uint32_t *p;
        uint32_t reg;

        USerial.printf("testing with pseudo-random sequence, seed=%u\n", seed);
        reg = seed;
        for (p = memory_begin; p < memory_end; p++) {
            *p = reg;
            for (int i=0; i < 3; i++) {
                // https://en.wikipedia.org/wiki/Xorshift
                reg ^= reg << 13;
                reg ^= reg >> 17;
                reg ^= reg << 5;
            }
        }
        arm_dcache_flush_delete((void *)memory_begin,
            (uint32_t)memory_end - (uint32_t)memory_begin);
        reg = seed;
        for (p = memory_begin; p < memory_end; p++) {
            uint32_t actual = *p;
            if (actual != reg) return fail_message(p, actual, reg);
            //USerial.printf(" reg=%08X\n", reg);
            for (int i=0; i < 3; i++) {
                reg ^= reg << 13;
                reg ^= reg >> 17;
                reg ^= reg << 5;
            }
        }
        return true;
    }
}