#pragma once

#include "ST-LIB.hpp"

#include <cstdint>

namespace RemoteBoards {

enum class HVBMS_State : uint8_t {
    // Connecting???
    Idle = 0,
    ReadyToPrecharge = 1,
    Precharging = 2,
    Energized = 3,
    Fault = 4,
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

inline HVBMS_State hvbms_sm_state = HVBMS_State::Idle;
inline PCU_State pcu_state = PCU_State::Connecting;
inline LCU_State lcu_state = LCU_State::Connecting;

inline float pcu_start_svpwm_freq_mod = 0.0f;
inline float pcu_start_svpwm_freq_com = 0.0f;
inline float pcu_start_svpwm_vref = 0.0f;
inline float pcu_start_svpwm_vmax = 0.0f;

inline float lcu_levitate_distance = 0.0f;

inline HeapOrder* FAULT_to_hvbms_order = nullptr;
inline HeapOrder* Precharge_to_hvbms_order = nullptr;
inline HeapOrder* Open_contactors_to_hvbms_order = nullptr;

inline HeapOrder* Stop_Motor_to_pcu_order = nullptr;
inline HeapOrder* Start_SVPWM_to_pcu_order = nullptr;

inline HeapOrder* Stop_to_lcu_order = nullptr;
inline HeapOrder* Levitate_to_lcu_order = nullptr;

inline void init_remote_state_receivers() {
    new HeapPacket(static_cast<uint16_t>(960), &hvbms_sm_state);
    new HeapPacket(static_cast<uint16_t>(553), &pcu_state);
    new HeapPacket(static_cast<uint16_t>(9520), &lcu_state);
}

inline void init_remote_orders() {
    FAULT_to_hvbms_order = new HeapOrder(0, +[]() {});
    Precharge_to_hvbms_order = new HeapOrder(903, +[]() {});
    Open_contactors_to_hvbms_order = new HeapOrder(901, +[]() {});

    Start_SVPWM_to_pcu_order = new HeapOrder(
        507, +[]() {},
        &pcu_start_svpwm_freq_mod,
        &pcu_start_svpwm_freq_com,
        &pcu_start_svpwm_vref,
        &pcu_start_svpwm_vmax
    );
    Stop_Motor_to_pcu_order = new HeapOrder(508, +[]() {});

    Levitate_to_lcu_order = new HeapOrder(
        9010, +[]() {},
        &lcu_levitate_distance
    );
    Stop_to_lcu_order = new HeapOrder(9000, +[]() {});
}

} // namespace RemoteBoards
