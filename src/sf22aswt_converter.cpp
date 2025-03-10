#include "sf22aswt_converter.h"

namespace SF22ASWT::converter
{
    CODE_LOCATION AudioSynthWavetable::instrument_data to_AudioSynthWavetable_instrument_data(SF22ASWT::instrument_data_temp &data)
    {
        // must use a second struct to contain the data
        // as AudioSynthWavetable::sample_data members are const
        SF22ASWT::sample_header *samples = new SF22ASWT::sample_header[data.sample_count+1];
        uint8_t *note_ranges = new uint8_t[data.sample_count+1];
        for (int i=0;i<data.sample_count;i++)
        {
            samples[i] = toFinal(data.samples[i]);//.toFinal();
            note_ranges[i] = data.sample_note_ranges[i];
        }
        // have one dummy empty sample so that AudioSynthWavetable don't crash while try to play notes outside of scope
        samples[data.sample_count] = {};
        note_ranges[data.sample_count] = 127; // set to last note possible

        const AudioSynthWavetable::sample_data *sample_data = reinterpret_cast<const AudioSynthWavetable::sample_data*>(samples);
        data.sample_count++;
        return {
            data.sample_count,
            note_ranges,
            sample_data
        };
    }

    CODE_LOCATION SF22ASWT::sample_header toFinal(SF22ASWT::sample_header_temp &sd)
    {
        return 
        {
            (int16_t*)sd.sample,
            sd.LOOP, // LOOP
            sd.LENGTH_BITS, // LENGTH_BITS
            (float)((1 << (32 - sd.LENGTH_BITS)) * WAVETABLE_CENTS_SHIFT(sd.CENTS_OFFSET) * sd.SAMPLE_RATE / WAVETABLE_NOTE_TO_FREQUENCY(sd.SAMPLE_NOTE) / AUDIO_SAMPLE_RATE_EXACT + 0.5f), // PER_HERTZ_PHASE_INCREMENT
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
            (float)((WAVETABLE_CENTS_SHIFT(sd.VIB_PITCH_INIT) - 1.0) * 4), // VIBRATO_PITCH_COEFFICIENT_INITIAL
            (float)((1.0 - WAVETABLE_CENTS_SHIFT(sd.VIB_PITCH_SCND)) * 4), // VIBRATO_COEFFICIENT_SECONDARY
            // MODULATION VALUES
            uint32_t(sd.MOD_DELAY_ENV * AudioSynthWavetable::SAMPLES_PER_MSEC / (2 * AudioSynthWavetable::LFO_PERIOD)), // MODULATION_DELAY
            uint32_t(sd.MOD_INC_ENV * AudioSynthWavetable::LFO_PERIOD * (UINT32_MAX / AUDIO_SAMPLE_RATE_EXACT)), // MODULATION_INCREMENT
            (float)((WAVETABLE_CENTS_SHIFT(sd.MOD_PITCH_INIT) - 1.0) * 4), // MODULATION_PITCH_COEFFICIENT_INITIAL
            (float)((1.0 - WAVETABLE_CENTS_SHIFT(sd.MOD_PITCH_SCND)) * 4), // MODULATION_PITCH_COEFFICIENT_SECOND
            int32_t(UINT16_MAX * (WAVETABLE_DECIBEL_SHIFT(sd.MOD_AMP_INIT_GAIN) - 1.0)) * 4, // MODULATION_AMPLITUDE_INITIAL_GAIN
            int32_t(UINT16_MAX * (1.0 - WAVETABLE_DECIBEL_SHIFT(sd.MOD_AMP_SCND_GAIN))) * 4, // MODULATION_AMPLITUDE_FINAL_GAIN
        };
    }
}