#ifndef EQUALIZERAPP_PARAMETEREVENT_H
#define EQUALIZERAPP_PARAMETEREVENT_H

#include "ParameterID.h"
#include "ParameterValue.h"

/**
 * Represents a single parameter change request from the UI or automation.
 */
struct ParameterEvent {
    ParameterID id;
    ParameterValue value;
};

#endif //EQUALIZERAPP_PARAMETEREVENT_H
