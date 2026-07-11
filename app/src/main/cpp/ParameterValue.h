#ifndef EQUALIZERAPP_PARAMETERVALUE_H
#define EQUALIZERAPP_PARAMETERVALUE_H

#include <cstdint>

/**
 * A production-grade variant container for parameter payloads.
 * Supports scalars, booleans, and indices.
 * Designed for 8-byte alignment to ensure atomic-like copies where applicable.
 */
struct ParameterValue {
    enum class Type : uint32_t {
        Float = 0,
        Int,
        Bool,
        ID      // For resource handles (IRs, HRTF profiles)
    };

    union {
        float f;
        int32_t i;
        bool b;
        uint32_t u;
    } val;

    Type type;

    // Helper constructors for convenience (Non-RT path)
    static ParameterValue fromFloat(float f) {
        ParameterValue p; p.type = Type::Float; p.val.f = f; return p;
    }
    static ParameterValue fromInt(int32_t i) {
        ParameterValue p; p.type = Type::Int; p.val.i = i; return p;
    }
    static ParameterValue fromBool(bool b) {
        ParameterValue p; p.type = Type::Bool; p.val.b = b; return p;
    }
};

#endif //EQUALIZERAPP_PARAMETERVALUE_H
