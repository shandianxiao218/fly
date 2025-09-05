#include "obstruction.h"

// TODO: 考虑遮挡物的材质对信号衰减的影响
int obstruction_calculate(const AircraftGeometry* geometry,
                         const SatellitePosition* satellite_pos,
                         const AircraftState* aircraft_state,
                         const ObstructionParams* params,
                         ObstructionResult* result) {
    // Placeholder implementation
    if (result) {
        result->is_obstructed = 0;
    }
    return 1;
}
