#ifndef EQUALIZERAPP_PARAMETERID_H
#define EQUALIZERAPP_PARAMETERID_H

#include <cstdint>

/**
 * Strongly typed enumeration for every controllable parameter in the engine.
 * Using explicit underlying type for JNI compatibility and deterministic sizing.
 */
enum class ParameterID : uint32_t {
    Bypass = 0,
    PreampGain,
    HighPassFreq,
    StereoWidth,
    Crossfeed,
    OutputVolume,

    // Parametric EQ Bands (12 bands supported)
    // Structured as: BandIndex * 10 + Type/Freq/Gain/Q
    Band1Type = 100, Band1Freq, Band1Gain, Band1Q,
    Band2Type = 110, Band2Freq, Band2Gain, Band2Q,
    Band3Type = 120, Band3Freq, Band3Gain, Band3Q,
    Band4Type = 130, Band4Freq, Band4Gain, Band4Q,
    Band5Type = 140, Band5Freq, Band5Gain, Band5Q,
    Band6Type = 150, Band6Freq, Band6Gain, Band6Q,
    Band7Type = 160, Band7Freq, Band7Gain, Band7Q,
    Band8Type = 170, Band8Freq, Band8Gain, Band8Q,
    Band9Type = 180, Band9Freq, Band9Gain, Band9Q,
    Band10Type = 190, Band10Freq, Band10Gain, Band10Q,
    Band11Type = 200, Band11Freq, Band11Gain, Band11Q,
    Band12Type = 210, Band12Freq, Band12Gain, Band12Q
};

/**
 * Total number of addressable parameters.
 * Used for fixed-size lookup tables to ensure deterministic O(1) access.
 */
constexpr uint32_t kMaxParameterID = 256;

#endif //EQUALIZERAPP_PARAMETERID_H
