#ifndef VCU_STATE_MACHINE_HPP
#define VCU_STATE_MACHINE_HPP

#include "VCU_TYPES.hpp"

namespace VCU_StateMachine {

inline void start() {
    VCU::operational_state = VCU::OperationalState::Idle;
    Scheduler::register_task(200'000, +[]() { VCU::led_status->toggle(); });
}

inline void update() {}

} // namespace VCU_StateMachine

#endif // VCU_STATE_MACHINE_HPP
