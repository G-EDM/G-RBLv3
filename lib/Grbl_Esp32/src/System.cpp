#include "Grbl.h"
#include "Config.h"

// Declare system global variable structure
DRAM_ATTR int32_t       sys_position[N_AXIS];        // Real-time machine (aka home) position vector in steps.
int32_t                sys_probe_position[N_AXIS];  // Last probe position in machine coordinates and steps.
int32_t                sys_probe_position_final[N_AXIS];
volatile bool          sys_probed_axis[N_AXIS];


DRAM_ATTR int64_t idle_timer = esp_timer_get_time();

IRAM_ATTR void system_flag_wco_change(){}
/** this is not accurate used only for rough calculations! **/
IRAM_ATTR int system_convert_mm_to_steps(float mm, uint8_t idx) {
    int steps = int( mm * g_axis[idx]->steps_per_mm.get() );
    return steps;
}
IRAM_ATTR float system_convert_axis_steps_to_mpos(int32_t steps, uint8_t idx) {
    float pos = (float)steps / g_axis[idx]->steps_per_mm.get();
    return pos;
}
// Returns machine position of axis 'idx'. Must be sent a 'step' array.
// NOTE: If motor steps and machine position are not in the same coordinate frame, this function
//   serves as a central place to compute the transformation.
IRAM_ATTR void system_convert_array_steps_to_mpos(float* position, int32_t* steps) {
    float motors[N_AXIS];
    for (int idx = 0; idx < N_AXIS; idx++) {
        motors[idx] = (float)steps[idx] / g_axis[idx]->steps_per_mm.get();
    }
    memcpy(position, motors, N_AXIS * sizeof(motors[0]));
}
IRAM_ATTR float* system_get_mpos() {
    DRAM_ATTR static float position[N_AXIS];
    system_convert_array_steps_to_mpos(position, sys_position);
    return position;
};
IRAM_ATTR float system_get_mpos_for_axis( int axis ) {
    return (float)sys_position[axis] / g_axis[axis]->steps_per_mm.get();
};
void IRAM_ATTR mpos_to_wpos(float* position) {
    float* wco    = get_wco();
    auto n_axis = N_AXIS;
    for (int idx = 0; idx < n_axis; idx++) {
        position[idx] -= wco[idx];
    }
}
float* get_wco() {
    DRAM_ATTR static float wco[N_AXIS];
    auto n_axis = N_AXIS;
    for (int idx = 0; idx < n_axis; idx++) {
        // Apply work coordinate offsets and tool length offset to current position.
        wco[idx] = gc_state.coord_system[idx] + gc_state.coord_offset[idx];
        if (idx == TOOL_LENGTH_OFFSET_AXIS) {
            wco[idx] += gc_state.tool_length_offset;
        }
    }
    return wco;
}