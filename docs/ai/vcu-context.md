# VCU-H11 AI Context

This file is the durable AI context for the Vehicle Control Unit firmware. Future agents should read it before planning or editing this repository, then update it whenever they learn stable VCU knowledge.

## Purpose

VCU-H11 is the Vehicle Control Unit firmware for Hyperloop UPV. In conversations it may also be called the Master. It targets the STM32H723ZGT6 custom H11 board and is built on ST-LIB.

The repository was created from `Hyperloop-UPV/template-project`, but template example code and example docs are not part of the VCU product surface.

## Legacy VCU Logic Reference

The previous VCU board codebase at `/Users/jsaegar/hyper-src/VCU-H10-V2` is a behavior reference only. It used an old ST-LIB API and should not be copied for H11 hardware access patterns.

Extracted H10-V2 business-logic docs live in `docs/vcu-logic/`. They describe the VCU as the vehicle master responsible for state-gated command arbitration, local brake/pump/sensor handling, telemetry, remote-board order forwarding, acknowledgement tracking, and faulting on unsafe conditions, lost required peers, emergency stop, or remote acknowledgement timeout.

Stable H10 behavior to preserve in H11 intent:

- Required peer connectivity gates normal operation.
- Orders should be validated against operational state before acting.
- Local fault/safety behavior includes SDC-open faulting, emergency-stop propagation, brake-on-fault intent, and tape/brake reed checks that need active-level and safety review before H11 implementation.

Current H11 product state machine:

- The explicit VCU states are `Idle`, `Connected`, `Manteinance`, `Precharging`, `HVActive`, `Ready`, `Propulsion`, `StaticLevitation`, `DynamicLevitation`, and `Fault`.
- `Idle` transitions to `Connected` only when the control station and required boards are connected. The current required peers are control station, HVBMS, and LCU. PCU connectivity is still tracked but temporarily does not gate `Idle -> Connected` while bench testing with only two Nucleo mock boards.
- `Connected` accepts high-level `MANTEINANCE` and `Precharge` orders. `Stop` returns commandable non-idle modes to `Connected`.
- `Brake` is accepted from `Ready`, `Propulsion`, `StaticLevitation`, and `DynamicLevitation`. From active motion/levitation states it engages brakes and returns to `HVActive` while keeping contactors closed; active-mode exit actions still stop propulsion and/or levitation.
- `Precharging` sends the formal HVBMS precharge request through the existing high-voltage remote link until a dedicated HVBMS socket/state packet is defined.
- `Fault` is intentionally part of the current formal product state machine by explicit user request. ST-LIB protections and `FaultController` still provide the underlying fault trigger infrastructure.
- Current protection/diagnostic policy: SDC open faults, `tapes_reached` faults, control-station disconnect faults outside `Idle`, and high pressure below 50 bar emits a warning. Brake fault is currently sampled and published but temporarily does not fault the VCU while `DISABLE_BRAKE_FAULT_PROTECTION` is enabled for bench testing.
- `tapes_reached` is currently a formal software signal only; H11 has no tape input pin defined in `Pinout.hpp` yet.

## Current Firmware Shape

- Main application entry: `Core/Src/main.cpp`.
- Main VCU facade: `Core/Inc/VCU.hpp`.
- Device requests, global runtime pointers, and app state: `Core/Inc/VCU_TYPES.hpp`.
- State machine skeleton: `Core/Inc/StateMachine/VCU_StateMachine.hpp`.
- Declarative operational FSM spec: `docs/vcu_state_machine.yaml`.
- Hardware pin aliases: `Core/Inc/Pinout/Pinout.hpp`.
- Packet schemas: `Core/Inc/Code_generation/JSON_ADE/boards/VCU/`.

Generated Ethernet comms bind the control-station UDP telemetry socket plus PCU, HVBMS, and LCU UDP sockets. The remote UDP sockets are needed because incoming state packets update the global `HeapPacket` values that the VCU uses for acknowledgement tracking.

Current VCU generated TCP sockets are control station, PCU, HVBMS, and LCU. Do
not keep an unused BCU TCP socket in the VCU generated socket list: each ST-LIB
TCP socket reserves an 8192-byte receive stream buffer, and the extra BCU socket
caused `std::bad_alloc` during `OrderPackets::start()` before the main loop
could service Ethernet.

ST-LIB TCP client sockets can be constructed before the Ethernet link has a
route to the remote board. Initial `tcp_connect()` failures such as lwIP
`ERR_RTE` must be treated as retryable by setting `pending_connection_reset`,
not as a startup panic, so VCU boot can continue while remote boards come up.
The VCU state machine should also call `reconnect()` on disconnected remote TCP
client sockets while refreshing peer connectivity.

Peer connection monitoring publishes a runtime `INFO` diagnostic on rising
edges when the control station, HVBMS, PCU, or LCU connects to the master. Once
a peer has connected, a later disconnect transitions the VCU to `Fault` with a
board-specific reason such as `HVBMS disconnected`, `LCU disconnected`, or
`PCU disconnected`.

Status LED behavior is driven by a 200 ms scheduled task. In `Idle`, while the
VCU is still waiting for required peers, `led_operational` blinks and
`led_connecting` stays off. Once required peers are connected, `led_operational`
stays on. Fault states turn on `led_fault` and turn off the other status LEDs.

VCU-H11 requires Ethernet. Do not support or maintain non-Ethernet VCU builds;
application code intentionally fails compilation when `STLIB_ETH` is not
defined.

For isolated VCU bench testing, the firmware supports a `SINGLE` compile mode.
Build it with the `board-debug-eth-ksz8041-single` preset while the nested ADJ
repository is on branch `vcu/single`. That ADJ branch removes remote-board
sockets and packets/orders, leaving only the control-station links and local VCU
telemetry/orders. In `SINGLE`, required peer connectivity means control-station
connectivity only, remote-board command forwarding is compiled out, and
precharge completion is simulated locally by marking contactors closed.

The build also supports simple Ethernet mock peer entry points selected with the
`VCU_APP_TARGET` CMake cache variable. `master` builds the normal VCU main,
`hvbms_mock` builds a Nucleo mock with IP `192.168.1.7`, and `lcu_mock` builds a
Nucleo mock with IP `192.168.1.4`. The mock build presets set `BOARD_NAME` to
the matching mock ADJ board so generated packets/sockets match the mock target.
The current mock firmwares initialize Ethernet, create a VCU-facing
`ServerSocket` directly in firmware on port `50500`, initialize the generated
mock connection-status data packet with `connected_to_master`, and publish that
status over UDP while the VCU connects to the mock server. On the Nucleo mock
boards, the `PB0` LED blinks while waiting for the first VCU connection, stays
on while connected, and turns off if the VCU disconnects after having connected;
the red Nucleo LED on `PB14` turns on for that post-connection disconnect.
Mock server sockets recreate their listener after ST-LIB reports the accepted
connection is no longer connected, so the mock can accept the master again
after a loss. For bench feedback, the mock `ServerSocket`s use ST-LIB TCP
keepalive with a short timeout. ST-LIB server sockets should stop reporting
connected once lwIP leaves an established state or exhausts keepalive probes;
the ST-LIB server poll path actively sends TCP keepalive probes so silent server
connections do not remain accepted indefinitely. The VCU-facing mock TCP server
is intentionally not declared in ADJ, because the control station should only
open TCP connections to the VCU/master. Generated packet headers are shared
source-tree files, so the CMake `run_generator` target is intentionally always
run before firmware compilation with the currently configured `BOARD_NAME`;
otherwise a mock build can accidentally compile against stale VCU-generated
packet headers, or the reverse.

The nested ADJ repository has a `vcu-packets-mock` branch based on
`vcu-packets`. It adds two mock boards, `HVBMSMOCK` at `192.168.1.7` and
`LCUMOCK` at `192.168.1.4`, each with a single boolean
`connected_to_master` data packet for bench mock status. For bench visibility
only, the mock connection-status packets are assigned to unique mock UDP sockets
(`hvbms_mock_control_station_udp` and `lcu_mock_control_station_udp`) and sent
to the control-station backend at `192.168.0.9:50400` every 100 ms, even though
the real remote boards will communicate only with the VCU.

ADJ measurements that represent VCU state-machine states should keep their
numeric wire type, currently `uint8`, and add `enumValues` in numeric order so
the control station can display labels. Remote board status fields are not part
of the VCU state machine and are currently not exposed as VCU measurements or
packets.

`VCU::update()` should keep servicing infrastructure each loop:

- `FaultController::check_transitions()`
- `Board::evaluate_protections()`
- `Diagnostics::Hub::flush()`
- `Scheduler::update()`

`docs/vcu_state_machine.yaml` describes the current H11 FSM in the formal format from `docs/fsm_format_spec.md`, and `docs/vcu_state_machine_diagram.md` renders the same flow as Mermaid.

## ST-LIB Board Conventions

Use the `ST_LIB::Board` type as the single application-level hardware registry.

Expected pattern:

1. Declare `inline constexpr` device request objects at namespace scope in `VCU_TYPES.hpp`.
2. Pass those requests to `using Board = ST_LIB::Board<...>` in `VCU.hpp`.
3. Call `Board::init()` once during `VCU::init()`.
4. Store runtime instance pointers from `Board::instance_of<request>()` only after `Board::init()`.
5. Access hardware through those ST-LIB instances, not through HAL directly.

Do not declare device requests inside functions. They must remain compile-time objects.

Use the most abstract ST-LIB object that matches the hardware role. For example, analog pressure sensors should be exposed through `LinearSensor<float>` fed by `ADCDomain` instances, NTC channels through `NTC`, and edge/interrupt sensors through `EXTIDomain`/`SensorInterrupt` where appropriate. Do not represent a higher-level sensor as a raw `DigitalInput` or `DigitalOutput` unless ST-LIB does not currently provide a better abstraction.

## Faults, Protections, Diagnostics

ST-LIB protections and diagnostics remain the mechanism for detecting and entering fault behavior. Current scaffold uses `ST_LIB::FaultPolicyNoMachine<on_fault_enter>`.

By explicit user request, the current VCU product FSM also exposes `Fault` as a formal state and telemetry value. Keep ST-LIB protection/fault policy as the trigger infrastructure; do not replace it with ad hoc HAL checks.

Relevant ST-LIB docs:

- `deps/ST-LIB/docs/protections-and-diagnostics.md`
- `deps/ST-LIB/docs/st-lib-board-contract.md`

## Naming

Firmware concepts must be in English. Prefer domain names that describe intent, not schematic abbreviations.

Current pinout naming examples:

- `cooling_pump_1` and `cooling_pump_2`, not `bomba_*`
- `electrovalve`, not `electrovalv`
- `pressure_regulator_out`, not `regulador_*`
- `brake_reset` and `brake_fault`
- `ntc_temperature_1` and `ntc_temperature_2`
- `sdc_closed`

The Altium schematic may use Spanish names or shortened net names. Convert those into clear English names in firmware.

## Pinout Facts

The current pinout was extracted from `docs/VCU_H11G.zip`, specifically `MCU.SchDoc`, cross-checked with `docs/Schematic PDF_[No Variations].pdf`.

`docs/VCU.ioc` is the current source for each MCU pin role. Confirmed roles:

- `PF0` and `PF1`: TIM23 input capture channels for flow sensors.
- `PF3` and `PF4`: ADC3 inputs for NTC temperature sensors.
- `PF11`: ADC1 input for `high_pressure`, the conditioned output of `Transductor_Alta_Presion` on connector `J13`. The schematic notes this as a ratiometric 0-5000 PSI transducer with a 0.5-4.5 V analog output scaled to about 0.367-3.3 V before the MCU.
- `PF12`: ADC1 input for `low_pressure`, the conditioned output of `Transductor_Baja_Presion` on connector `J14`. The schematic notes this as a ratiometric 0-10 bar sealed-gauge transducer with a 0.5-4.5 V analog output scaled to about 0.367-3.3 V before the MCU; because it is sealed gauge, the note says the real pressure range starts around 1.013 bar.
- `PA5`: ADC2 input for manual pressure regulator feedback sensing.
- `PA6`: not used by H11 firmware for regulator control. H11 uses a manual pressure regulator that is not commanded by firmware.
- `PF6`: EXTI6 input for `sdc_closed`.
- `PD14`: digital input for `brake_fault`.
- `PD15`: digital output for `brake_reset`.
- `PE13`: digital output for `cooling_pump_1`.
- `PE14`: digital output for `cooling_pump_2`.
- `PE15`: digital output for `electrovalve`.
- `PA8`: digital output for CAN silent mode.
- `PG3` and `PG4`: digital inputs for SDMMC write-protect and card-detect.
- `PG9` through `PG13`: digital outputs for info LEDs. The corrected Altium net on `PG11` is `LED_CONNECTING`, not `LED_FLASH`.

Current firmware-facing pin aliases:

| Concept | STM32 pin |
| --- | --- |
| `led_sleep` | `PG9` |
| `led_can` | `PG10` |
| `led_connecting` | `PG11` |
| `led_fault` | `PG12` |
| `led_operational` | `PG13` |
| `can_txd` | `PA12` |
| `can_rxd` | `PA11` |
| `can_silent` | `PA8` |
| `sdmmc_cmd` | `PD2` |
| `sdmmc_clk` | `PC12` |
| `sdmmc_d0` | `PC8` |
| `sdmmc_d1` | `PC9` |
| `sdmmc_d2` | `PC10` |
| `sdmmc_d3` | `PC11` |
| `sdmmc_card_detect` | `PG4` |
| `sdmmc_write_protect` | `PG3` |
| `flow_1` | `PF0` |
| `flow_2` | `PF1` |
| `ntc_temperature_1` | `PF3` |
| `ntc_temperature_2` | `PF4` |
| `high_pressure` | `PF11` |
| `low_pressure` | `PF12` |
| `sdc_closed` | `PF6` |
| `cooling_pump_1` | `PE13` |
| `cooling_pump_2` | `PE14` |
| `electrovalve` | `PE15` |
| `pressure_regulator_out` | `PA5` |
| `brake_reset` | `PD15` |
| `brake_fault` | `PD14` |

H11 does not use firmware-controlled pressure regulation. Do not request `PA6` / `TIM13_CH1` as a regulator PWM device, do not expose a set-regulator order, and do not publish commanded regulator duty. The pressure regulator is manual; firmware may only observe its feedback through `pressure_regulator_out` on `PA5` / `ADC2_INP19`.

Ethernet RMII is intentionally not listed in `Pinout.hpp`. Use ST-LIB pinset selection instead.

For VCU board Ethernet with KSZ8041, use:

```cpp
ST_LIB::EthernetDomain::PINSET_H11
```

`PINSET_H11` includes H11 RMII pins and no RXER pin. The current VCU H11 board
uses the KSZ8041 PHY path; the LAN8700 presets are not the correct hardware
target for this board.

## Build And Verification

Prefer the local CLI:

```bash
./hyper build main --preset board-debug-eth-ksz8041 --board-name VCU
```

Known passing checks:

- `./hyper build main --preset board-debug-eth-ksz8041 --board-name VCU`
- `./hyper build main --preset nucleo-debug-eth-hvbms-mock --board-name VCU`
- `./hyper build main --preset nucleo-debug-eth-lcu-mock --board-name VCU`
- `python3 -m py_compile hyper`

Known intentional failure:

- `./hyper build main --preset board-debug --board-name VCU` fails because
  `STLIB_ETH` is not defined.

When modifying ADJ files under `Core/Inc/Code_generation/JSON_ADE`, run the ADJ validator before handoff. The validator depends on `jsonschema`; install the pinned tester requirements into the repository virtual environment when needed:

```bash
.venv/bin/python -m pip install -r Core/Inc/Code_generation/JSON_ADE/.github/workflows/scripts/adj-tester/requirements.txt
```

Run the validator from the ADJ root with the repository virtual environment:

```bash
cd Core/Inc/Code_generation/JSON_ADE
../../../../.venv/bin/python .github/workflows/scripts/adj-tester/main.py
```

The validator checks schema validity and project consistency such as unused measurements. If ADJ packets or orders are removed for a test branch, trim `VCU_measurements.json` to match the remaining packet/order variables or the validator will fail.

Do not restore `Core/Src/Runes/generated_metadata.cpp` after builds. The user explicitly requested that generated metadata stay as generated.

## Altium And Hardware Inputs

Current hardware inputs in `docs/`:

- `Schematic PDF_[No Variations].pdf`
- `VCU_H11G.zip`
- `Gerber for PCB.PcbDoc.zip`
- `STEP_[No Variations] for PCB.PcbDoc.step`
- `Assembly Drawings_[No Variations].pdf`
- `BOM_[No Variations].csv`

The Altium source archive is more authoritative than PDF OCR/text extraction. Use the `.SchDoc` records for pin/net extraction when possible.

## Open Decisions

These need confirmation before implementing more runtime behavior:

- Active level and safe default for every output.
- Pull-up/pull-down requirements for digital inputs.
- Sensor calibration and units for pressure, flow, and NTC temperature.
- Protection thresholds and diagnostic IDs.
- Whether brake fault should remain a polled protection only or also become EXTI-triggered.
- Exact flow-sensor abstraction. ST-LIB currently provides TIM23 input-capture support, but no dedicated flow/pulse sensor wrapper has been selected.
- Final pressure, flow, and temperature packet units once calibration is defined.

## Maintenance Protocol

Update this file when a durable fact becomes known. Good updates include:

- New confirmed pin mappings or active levels.
- Chosen ST-LIB device request patterns.
- Confirmed VCU operational states and transitions.
- Protection and diagnostic policies.
- Build, flash, debug, or hardware validation workflows that were actually run.
- ADJ validation workflows, especially if the command or required environment changes.
- User preferences that affect future work.

Do not add transient command output, speculative assumptions, or one-off debugging notes. If a fact is inferred rather than confirmed, label it as inferred and keep it in `Open Decisions` until confirmed.
