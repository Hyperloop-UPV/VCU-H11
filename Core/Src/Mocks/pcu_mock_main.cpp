#include "Mocks/MockPeer.hpp"
#include "ST-LIB.hpp"
#include "main.h"

namespace PCUMock {

inline constexpr const char* local_ip = "192.168.1.5";

inline constexpr auto eth = ST_LIB::EthernetDomain::Ethernet(
    MockPeer::ethernet_pinset,
    "00:80:e1:00:01:05",
    local_ip,
    "255.255.255.0"
);

using Board = MockPeer::Board<eth>;

enum class PCU_State : uint8_t {
    Connecting = 0,
    Idle = 1,
    Accelerating = 2,
    Fault = 3,
};

inline ST_LIB::EthernetDomain::Instance* ethernet = nullptr;
#ifndef MOCK_NO_LEDS
inline ST_LIB::DigitalOutputDomain::Instance* status_led = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* disconnected_led = nullptr;
#endif
inline ServerSocket* vcu_socket = nullptr;
inline bool connected_to_master = false;
inline bool was_connected_to_master = false;

inline PCU_State pcu_state = PCU_State::Idle;
inline float dummy_svpwm_params[4] = {};

inline HeapPacket* state_packet = nullptr;
inline DatagramSocket* state_udp = nullptr;

#ifndef MOCK_NO_LEDS
inline void update_leds() {
    MockPeer::update_connection_leds(
        connected_to_master,
        was_connected_to_master,
        *status_led,
        *disconnected_led
    );
}
#endif

inline void init() {
    ethernet = MockPeer::init_ethernet<Board, eth>();
#ifndef MOCK_NO_LEDS
    status_led = &Board::instance_of<MockPeer::status_led_req>();
    disconnected_led = &Board::instance_of<MockPeer::disconnected_led_req>();
    status_led->turn_off();
    disconnected_led->turn_off();
#endif
    vcu_socket = MockPeer::create_vcu_server_socket(local_ip);

    Diagnostics::install_ethernet_sink(vcu_socket);

    new HeapOrder(
        507, +[]() {
            pcu_state = PCU_State::Accelerating;
        },
        &dummy_svpwm_params[0],
        &dummy_svpwm_params[1],
        &dummy_svpwm_params[2],
        &dummy_svpwm_params[3]
    );
    new HeapOrder(508, +[]() {
        pcu_state = PCU_State::Idle;
    });

    state_packet = new HeapPacket(
        static_cast<uint16_t>(553), &pcu_state
    );

    state_udp = new DatagramSocket(
        "192.168.1.5", 50402, "192.168.1.3", 50402
    );

    Scheduler::register_task(16'670, +[]() {
        state_udp->send_packet(*state_packet);
    });

#ifndef MOCK_NO_LEDS
    Scheduler::register_task(200'000, +[]() { update_leds(); });
#endif
}

inline void update() {
    MockPeer::service_connection_status(
        *ethernet,
        vcu_socket,
        local_ip,
        connected_to_master,
        was_connected_to_master
    );
}

} // namespace PCUMock

int main(void) {
    Hard_fault_check();
    PCUMock::init();

    while (1) {
        PCUMock::update();
    }
}

extern "C" void Error_Handler(void) {
    PANIC("HAL error handler triggered");
    while (1) {
    }
}
