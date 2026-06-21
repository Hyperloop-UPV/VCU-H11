#include "Communications/Packets/DataPackets.hpp"
#include "Mocks/MockPeer.hpp"
#include "ST-LIB.hpp"
#include "main.h"

namespace HVBMSMock {

inline constexpr const char* local_ip = "192.168.1.7";

inline constexpr auto eth = ST_LIB::EthernetDomain::Ethernet(
    MockPeer::ethernet_pinset,
    "00:80:e1:00:01:07",
    local_ip,
    "255.255.0.0"
);

using Board = MockPeer::Board<eth>;

inline ST_LIB::EthernetDomain::Instance* ethernet = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* status_led = nullptr;
inline ST_LIB::DigitalOutputDomain::Instance* disconnected_led = nullptr;
inline ServerSocket* vcu_socket = nullptr;
inline bool connected_to_master = false;
inline bool was_connected_to_master = false;

inline void update_leds() {
    MockPeer::update_connection_leds(
        connected_to_master,
        was_connected_to_master,
        *status_led,
        *disconnected_led
    );
}

inline void init() {
    ethernet = MockPeer::init_ethernet<Board, eth>();
    status_led = &Board::instance_of<MockPeer::status_led_req>();
    disconnected_led = &Board::instance_of<MockPeer::disconnected_led_req>();
    status_led->turn_off();
    disconnected_led->turn_off();
    vcu_socket = MockPeer::create_vcu_server_socket(local_ip);
    DataPackets::HVBMS_Mock_Connection_Status_init(connected_to_master);
    DataPackets::start();
    Scheduler::register_task(200'000, +[]() { update_leds(); });
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

} // namespace HVBMSMock

int main(void) {
    Hard_fault_check();
    HVBMSMock::init();

    while (1) {
        HVBMSMock::update();
    }
}

extern "C" void Error_Handler(void) {
    PANIC("HAL error handler triggered");
    while (1) {
    }
}
