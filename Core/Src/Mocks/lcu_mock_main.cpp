#include "Communications/Packets/DataPackets.hpp"
#include "Mocks/MockPeer.hpp"
#include "ST-LIB.hpp"
#include "main.h"

namespace LCUMock {

inline constexpr const char* local_ip = "192.168.1.4";

inline constexpr auto eth = ST_LIB::EthernetDomain::Ethernet(
    MockPeer::ethernet_pinset,
    "00:80:e1:00:01:04",
    local_ip,
    "255.255.0.0"
);

using Board = MockPeer::Board<eth>;

inline ST_LIB::EthernetDomain::Instance* ethernet = nullptr;
#ifndef MOCK_NO_LEDS
inline ST_LIB::DigitalOutputDomain::Instance* status_led = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* disconnected_led = nullptr;
#endif
inline ServerSocket* vcu_socket = nullptr;
inline bool connected_to_master = false;
inline bool was_connected_to_master = false;

inline DataPackets::lcu_vertical_state lcu_vertical_state = DataPackets::lcu_vertical_state::Stopped;
inline DataPackets::lcu_horizontal_state lcu_horizontal_state = DataPackets::lcu_horizontal_state::Disabled;
inline float dummy_levitation_distance = 0.0f;

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

    new HeapOrder(37, +[]() {
        lcu_vertical_state = DataPackets::lcu_vertical_state::Levitation;
    }, &dummy_levitation_distance);
    new HeapOrder(46, +[]() {
        lcu_vertical_state = DataPackets::lcu_vertical_state::Stopped;
    });
    new HeapOrder(0, +[]() {
        lcu_vertical_state = DataPackets::lcu_vertical_state::Stopped;
    });

    DataPackets::LCU_Mock_Connection_Status_init(connected_to_master);
    DataPackets::LCU_State_init(lcu_vertical_state, lcu_horizontal_state);
    DataPackets::start();
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

} // namespace LCUMock

int main(void) {
    Hard_fault_check();
    LCUMock::init();

    while (1) {
        LCUMock::update();
    }
}

extern "C" void Error_Handler(void) {
    PANIC("HAL error handler triggered");
    while (1) {
    }
}
