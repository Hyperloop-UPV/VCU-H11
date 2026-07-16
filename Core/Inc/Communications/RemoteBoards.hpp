#pragma once

#include "ST-LIB.hpp"

#include <cstdint>

#ifdef ADJ_COMMIT_VALIDATION
#include <cstdlib>

extern "C" const char ADJ_COMMIT_HASH[16];
#endif

#ifdef CUSTOM_KEEPALIVE
void keepalive_timeout_trigger();
#endif

namespace RemoteBoards {

#ifdef ENABLE_HVBMS
enum class HVBMS_State : uint8_t {
    Connecting = 0,
    Idle = 1,
    Ready_To_Precharge = 2,
    Precharging = 3,
    Energized = 4,
    Fault = 5,
};
#endif

#ifdef ENABLE_PCU
enum class PCU_State : uint8_t {
    Connecting = 0,
    Idle = 1,
    Accelerating = 2,
    Fault = 3,
};
#endif

#ifdef ENABLE_LCU
enum class LCU_State : uint8_t {
    Connecting = 0,
    Idle = 1,
    Levitating = 2,
    Current_Control = 3,
    Debug = 4,
    Fault = 5,
};
#endif

#ifdef ENABLE_HVBMS
inline HVBMS_State hvbms_sm_state = HVBMS_State::Connecting;
#endif
#ifdef ENABLE_PCU
inline PCU_State pcu_state = PCU_State::Connecting;
#endif
#ifdef ENABLE_LCU
inline LCU_State lcu_state = LCU_State::Connecting;
inline LCU_State lcu_slave_state = LCU_State::Connecting;
#endif

#ifdef ENABLE_HVBMS
inline float hvbms_voltage_min = 0.0f;
inline float hvbms_voltage_max = 0.0f;
inline float hvbms_temp_min = 0.0f;
inline float hvbms_temp_max = 0.0f;
inline float hvbms_current_reading = 0.0f;
inline float hvbms_voltage_reading = 0.0f;
inline float hvbms_batteries_voltage = 0.0f;
#endif

inline float propulsion_current_reference = 0.0f;
inline float levitation_target_height = 0.0f;

#ifdef ENABLE_PCU
inline float pcu_start_svpwm_vref = 0.0f;
inline float pcu_start_svpwm_vmax = 0.0f;
#endif

inline void reset_control_params() {
    propulsion_current_reference= 0.0f;
    levitation_target_height = 0.0f;
}

#ifdef ENABLE_HVBMS
inline HeapOrder* Precharge_to_hvbms_order = nullptr;
inline HeapOrder* Open_contactors_to_hvbms_order = nullptr;
inline HeapOrder* read_bcm_faults = nullptr;
#endif

#ifdef ENABLE_PCU
inline HeapOrder* Stop_Motor_to_pcu_order = nullptr;
inline HeapOrder* Start_SVPWM_to_pcu_order = nullptr;
#endif

#ifdef ENABLE_LCU
inline HeapOrder* Stop_to_lcu_order = nullptr;
inline HeapOrder* Levitate_to_lcu_order = nullptr;
#endif

#ifdef CUSTOM_KEEPALIVE
inline uint16_t keepalive_timeout_id = Scheduler::INVALID_ID;
inline HeapOrder* keepalive_order = nullptr;
#endif

#ifdef ADJ_COMMIT_VALIDATION
inline uint64_t adj_commit_hash_value = 0;
inline uint64_t adj_hash_on_wire = 0;
inline bool adj_hash_verified = false;
inline bool adj_hashes_sent = false;
inline HeapOrder* adj_hash_order = nullptr;
#endif

inline void init_remote_state_receivers() {
#ifdef ENABLE_HVBMS
    new HeapPacket(
        static_cast<uint16_t>(950),
        &hvbms_voltage_min,
        &hvbms_voltage_max,
        &hvbms_temp_min,
        &hvbms_temp_max,
        &hvbms_current_reading,
        &hvbms_voltage_reading,
        &hvbms_batteries_voltage,
        &hvbms_sm_state
    );
#endif
#ifdef ENABLE_PCU
    new HeapPacket(static_cast<uint16_t>(553), &pcu_state);
#endif
#ifdef ENABLE_LCU
    new HeapPacket(static_cast<uint16_t>(9520), &lcu_state, &lcu_slave_state);
#endif
}

inline void init_remote_orders() {
#ifdef ENABLE_HVBMS
    Precharge_to_hvbms_order = new HeapOrder(
        903,
        +[]() {}
    );
    Open_contactors_to_hvbms_order = new HeapOrder(
        901,
        +[]() {}
    );
    read_bcm_faults = new HeapOrder(
        904,
        +[]() {}
    );
#endif

#ifdef ENABLE_PCU
    current_control_order = new HeapOrder(
        509,
        +[]() {},
        0,
        30000,
        &propulsion_current_reference,
        &hvbms_current_reading,
        true
    );
    Stop_Motor_to_pcu_order = new HeapOrder(
        508,
        +[]() {}
    );
#endif

#ifdef ENABLE_LCU
    Levitate_to_lcu_order = new HeapOrder(
        9010,
        +[]() {},
        &levitation_target_height
    );
    Stop_to_lcu_order = new HeapOrder(
        9000,
        +[]() {}
    );
#endif

#ifdef CUSTOM_KEEPALIVE
    keepalive_order = new HeapOrder(
        1,
        +[]() {
            if (keepalive_timeout_id != Scheduler::INVALID_ID) {
                Scheduler::cancel_timeout(keepalive_timeout_id);
            }
            keepalive_timeout_id = Scheduler::set_timeout(
                100'000,
                +[]() { keepalive_timeout_trigger(); }
            );
        }
    );
#endif

#ifdef ADJ_COMMIT_VALIDATION
    adj_commit_hash_value =
        ((uint64_t)ADJ_COMMIT_HASH[0]) | ((uint64_t)ADJ_COMMIT_HASH[1] << 8) |
        ((uint64_t)ADJ_COMMIT_HASH[2] << 16) | ((uint64_t)ADJ_COMMIT_HASH[3] << 24) |
        ((uint64_t)ADJ_COMMIT_HASH[4] << 32) | ((uint64_t)ADJ_COMMIT_HASH[5] << 40) |
        ((uint64_t)ADJ_COMMIT_HASH[6] << 48) | ((uint64_t)ADJ_COMMIT_HASH[7] << 56);
    adj_hash_order = new HeapOrder(
        65535,
        +[]() {
            if (adj_hash_on_wire != adj_commit_hash_value) {
                if (!FaultController::is_faulted()) {
                    FAULT("ADJ hashes don't match");
                }
            } else {
                adj_hash_verified = true;
            }
        },
        &adj_hash_on_wire
    );
#endif
}

} // namespace RemoteBoards
