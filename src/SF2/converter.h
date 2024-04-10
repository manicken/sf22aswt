
#pragma once

#include <Arduino.h>
#include <Audio.h>
#include "enums.h"
#include "structures.h"
#include "helpers.h"
#include "common.h"

namespace SF2::converter
{
    // don't know if this function is really needed
    // as the code @
    // https://forum.pjrc.com/index.php?threads/wavetable-synthesis-from-sd-card.58735/
    // sets the instrument directly from a cloned sample_data structure
    AudioSynthWavetable::instrument_data to_AudioSynthWavetable_instrument_data(SF2::instrument_data &data)
    {
        const AudioSynthWavetable::sample_data *sample_data = reinterpret_cast<const AudioSynthWavetable::sample_data*>(data.samples);

        return {
            data.sample_count,
            data.sample_note_ranges,
            sample_data
        };
    }

    AudioSynthWavetable::instrument_data to_AudioSynthWavetable_instrument_data(SF2::instrument_data_temp &data)
    {
        SF2::sample_header *samples = new sample_header[data.sample_count];
        uint8_t *note_ranges = new uint8_t[data.sample_count];
        for (int i=0;i<data.sample_count;i++)
        {
            samples[i] = data.samples[i].toFinal();
            note_ranges[i] = data.sample_note_ranges[i];
        }
        note_ranges[data.sample_count-1] = 127;

        const AudioSynthWavetable::sample_data *sample_data = reinterpret_cast<const AudioSynthWavetable::sample_data*>(samples);

        return {
            data.sample_count,
            note_ranges,
            sample_data
        };
    }

    void toFinal(instrument_data_temp &idt, instrument_data &id)
    {
        id.sample_count = idt.sample_count;
        id.sample_note_ranges = idt.sample_note_ranges;
        id.samples = new sample_header[idt.sample_count];
        for (int i=0;i<idt.sample_count;i++)
        {
            id.samples[i] = idt.samples[i].toFinal();
        }
    }
/*
    sample_data toFinal(sample_data_temp &sd)
    {
        return 
        {
            (int16_t*)0, // sample data pointer, this will be reset after this function call as the data for the sample need to be loaded and handled outside this scope
            sd.LOOP, // LOOP
            sd.LENGTH_BITS, // LENGTH_BITS
            (1 << (32 - sd.LENGTH_BITS)) * WAVETABLE_CENTS_SHIFT(sd.CENTS_OFFSET) * sd.SAMPLE_RATE / WAVETABLE_NOTE_TO_FREQUENCY(sd.SAMPLE_NOTE) / AUDIO_SAMPLE_RATE_EXACT + 0.5f, // PER_HERTZ_PHASE_INCREMENT
            ((uint32_t)sd.LENGTH - 1) << (32 - sd.LENGTH_BITS), // MAX_PHASE
            ((uint32_t)sd.LOOP_END - 1) << (32 - sd.LENGTH_BITS), // LOOP_PHASE_END
            (((uint32_t)sd.LOOP_END - 1) << (32 - sd.LENGTH_BITS)) - (((uint32_t)sd.LOOP_START - 1) << (32 - sd.LENGTH_BITS)), // LOOP_PHASE_LENGTH
            uint16_t(UINT16_MAX * WAVETABLE_DECIBEL_SHIFT(sd.INIT_ATTENUATION)), // INITIAL_ATTENUATION_SCALAR
            // VOLUME ENVELOPE VALUES
            uint32_t(sd.DELAY_ENV * AudioSynthWavetable::SAMPLES_PER_MSEC / AudioSynthWavetable::ENVELOPE_PERIOD + 0.5), // DELAY_COUNT
            uint32_t(sd.ATTACK_ENV * AudioSynthWavetable::SAMPLES_PER_MSEC / AudioSynthWavetable::ENVELOPE_PERIOD + 0.5), // ATTACK_COUNT
            uint32_t(sd.HOLD_ENV * AudioSynthWavetable::SAMPLES_PER_MSEC / AudioSynthWavetable::ENVELOPE_PERIOD + 0.5), // HOLD_COUNT
            uint32_t(sd.DECAY_ENV * AudioSynthWavetable::SAMPLES_PER_MSEC / AudioSynthWavetable::ENVELOPE_PERIOD + 0.5), // DECAY_COUNT
            uint32_t(sd.RELEASE_ENV * AudioSynthWavetable::SAMPLES_PER_MSEC / AudioSynthWavetable::ENVELOPE_PERIOD + 0.5), // RELEASE_COUNT
            int32_t((1.0 - WAVETABLE_DECIBEL_SHIFT(sd.SUSTAIN_FRAC)) * AudioSynthWavetable::UNITY_GAIN), // SUSTAIN_MULT
            // VIRBRATO VALUES
            uint32_t(sd.VIB_DELAY_ENV * AudioSynthWavetable::SAMPLES_PER_MSEC / (2 * AudioSynthWavetable::LFO_PERIOD)), // VIBRATO_DELAY
            uint32_t(sd.VIB_INC_ENV * AudioSynthWavetable::LFO_PERIOD * (UINT32_MAX / AUDIO_SAMPLE_RATE_EXACT)), // VIBRATO_INCREMENT
            (WAVETABLE_CENTS_SHIFT(sd.VIB_PITCH_INIT) - 1.0) * 4, // VIBRATO_PITCH_COEFFICIENT_INITIAL
            (1.0 - WAVETABLE_CENTS_SHIFT(sd.VIB_PITCH_SCND)) * 4, // VIBRATO_COEFFICIENT_SECONDARY
            // MODULATION VALUES
            uint32_t(sd.MOD_DELAY_ENV * AudioSynthWavetable::SAMPLES_PER_MSEC / (2 * AudioSynthWavetable::LFO_PERIOD)), // MODULATION_DELAY
            uint32_t(sd.MOD_INC_ENV * AudioSynthWavetable::LFO_PERIOD * (UINT32_MAX / AUDIO_SAMPLE_RATE_EXACT)), // MODULATION_INCREMENT
            (WAVETABLE_CENTS_SHIFT(sd.MOD_PITCH_INIT) - 1.0) * 4, // MODULATION_PITCH_COEFFICIENT_INITIAL
            (1.0 - WAVETABLE_CENTS_SHIFT(sd.MOD_PITCH_SCND)) * 4, // MODULATION_PITCH_COEFFICIENT_SECOND
            int32_t(UINT16_MAX * (WAVETABLE_DECIBEL_SHIFT(sd.MOD_AMP_INIT_GAIN) - 1.0)) * 4, // MODULATION_AMPLITUDE_INITIAL_GAIN
            int32_t(UINT16_MAX * (1.0 - WAVETABLE_DECIBEL_SHIFT(sd.MOD_AMP_SCND_GAIN))) * 4, // MODULATION_AMPLITUDE_FINAL_GAIN
        };
    }
*/

}