#pragma once

#include "ST-LIB.hpp"

#include <cstdint>

namespace RemoteBoards {

enum class HVBMS_State : uint8_t {
    Connecting = 0,
    Idle = 1,
    Ready_To_Precharge = 2,
    Precharging = 3,
    Energized = 4,
    Fault = 5,
};

enum class PCU_State : uint8_t {
    Connecting = 0,
    Idle = 1,
    Accelerating = 2,
    Fault = 3,
};

enum class LCU_State : uint8_t {
    Connecting = 0,
    Idle = 1,
    Levitating = 2,
    Current_Control = 3,
    Debug = 4,
    Fault = 5,
};

inline HVBMS_State hvbms_sm_state = HVBMS_State::Connecting;
inline PCU_State pcu_state = PCU_State::Connecting;
inline LCU_State lcu_state = LCU_State::Connecting;
inline LCU_State lcu_slave_state = LCU_State::Connecting;

inline float hvbms_voltage_min = 0.0f;
inline float hvbms_voltage_max = 0.0f;
inline float hvbms_temp_min = 0.0f;
inline float hvbms_temp_max = 0.0f;
inline float hvbms_current_reading = 0.0f;
inline float hvbms_voltage_reading = 0.0f;
inline float hvbms_batteries_voltage = 0.0f;

inline float propulsion_target_speed = 0.0f;
inline float propulsion_max_current = 0.0f;
inline float levitation_target_height = 0.0f;

inline float pcu_start_svpwm_vref = 0.0f;
inline float pcu_start_svpwm_vmax = 0.0f;

inline void reset_control_params() {
    propulsion_target_speed = 0.0f;
    propulsion_max_current = 0.0f;
    levitation_target_height = 0.0f;
}

inline HeapOrder* FAULT_to_hvbms_order = nullptr;
inline HeapOrder* Precharge_to_hvbms_order = nullptr;
inline HeapOrder* Open_contactors_to_hvbms_order = nullptr;

inline HeapOrder* Stop_Motor_to_pcu_order = nullptr;
inline HeapOrder* Start_SVPWM_to_pcu_order = nullptr;

inline HeapOrder* Stop_to_lcu_order = nullptr;
inline HeapOrder* Levitate_to_lcu_order = nullptr;

inline void init_remote_state_receivers() {
    new HeapPacket(static_cast<uint16_t>(950),
        &hvbms_voltage_min,
        &hvbms_voltage_max,
        &hvbms_temp_min,
        &hvbms_temp_max,
        &hvbms_current_reading,
        &hvbms_voltage_reading,
        &hvbms_batteries_voltage,
        &hvbms_sm_state
    );
    new HeapPacket(static_cast<uint16_t>(553), &pcu_state);
    new HeapPacket(static_cast<uint16_t>(9520), &lcu_state, &lcu_slave_state);
}

inline void init_remote_orders() {
    FAULT_to_hvbms_order = new HeapOrder(0, +[]() {});
    Precharge_to_hvbms_order = new HeapOrder(903, +[]() {});
    Open_contactors_to_hvbms_order = new HeapOrder(901, +[]() {});

    Start_SVPWM_to_pcu_order = new HeapOrder(
        507, +[]() {},
        &propulsion_target_speed,
        &propulsion_max_current,
        &pcu_start_svpwm_vref,
        &pcu_start_svpwm_vmax
    );
    Stop_Motor_to_pcu_order = new HeapOrder(508, +[]() {});

    Levitate_to_lcu_order = new HeapOrder(
        9010, +[]() {},
        &levitation_target_height
    );
    Stop_to_lcu_order = new HeapOrder(9000, +[]() {});
}

} // namespace RemoteBoards
