# VCU-H11 AI Context

This file is the durable AI context for the Vehicle Control Unit firmware. Future agents should read it before planning or editing this repository, then update it whenever they learn stable VCU knowledge.

## Purpose

VCU-H11 is the Vehicle Control Unit firmware for Hyperloop UPV. In conversations it may also be called the Master. It targets the STM32H723ZGT6 custom H11 board and is built on ST-LIB.

The repository was created from `Hyperloop-UPV/template-project`, but template example code and example docs are not part of the VCU product surface.

## Current Firmware Shape

- Main application entry: `Core/Src/main.cpp`.
- Main VCU facade: `Core/Inc/VCU.hpp`.
- Device requests, global runtime pointers, and app state: `Core/Inc/VCU_TYPES.hpp`.
- State machine skeleton: `Core/Inc/StateMachine/VCU_StateMachine.hpp`.
- Hardware pin aliases: `Core/Inc/Pinout/Pinout.hpp`.
- Packet schemas: `Core/Inc/Code_generation/JSON_ADE/boards/VCU/`.

`VCU::update()` should keep servicing infrastructure each loop:

- `FaultController::check_transitions()`
- `Board::evaluate_protections()`
- `Diagnostics::Hub::flush()`
- `Scheduler::update()`

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

After the latest ST-LIB release, FAULT state ownership belongs to infrastructure, not the application state machine.

VCU app state should describe operational modes only, for example:

- `Idle`
- `EndOfRun`
- `Energized`
- `Ready`
- `Demonstration`
- `Recovery`

Do not add `Fault` to the VCU operational state enum. Use ST-LIB protections and diagnostics for fault behavior. Current scaffold uses `ST_LIB::FaultPolicyNoMachine<on_fault_enter>`.

Relevant ST-LIB docs:

- `deps/ST-LIB/docs/protections-and-diagnostics.md`
- `deps/ST-LIB/docs/st-lib-board-contract.md`

## Naming

Firmware concepts must be in English. Prefer domain names that describe intent, not schematic abbreviations.

Current pinout naming examples:

- `cooling_pump`, not `bomba_mcu`
- `electrovalve`, not `electrovalv`
- `pressure_regulator_in` and `pressure_regulator_out`, not `regulador_*`
- `brake_reset` and `brake_fault`
- `ntc_temperature_1` and `ntc_temperature_2`
- `sdc_closed`

The Altium schematic may use Spanish names or shortened net names. Convert those into clear English names in firmware.

## Pinout Facts

The initial pinout was extracted from `docs/VCU_H11.zip`, specifically `MCU.SchDoc`, cross-checked with `docs/Schematic PDF_[No Variations].pdf`.

`docs/VCU.ioc` is the current source for each MCU pin role. Confirmed roles:

- `PF0` and `PF1`: TIM23 input capture channels for flow sensors.
- `PF3` and `PF4`: ADC3 inputs for NTC temperature sensors.
- `PF11` and `PF12`: ADC1 inputs for high/low pressure sensors.
- `PA5`: ADC2 input for pressure regulator output sensing.
- `PF6`: EXTI6 input for `sdc_closed`.
- `PD14`: digital input for `brake_fault`.
- `PD15`: digital output for `brake_reset`.
- `PE13`: digital output for `cooling_pump`.
- `PE15`: digital output for `electrovalve`.
- `PA8`: digital output for CAN silent mode.
- `PG3` and `PG4`: digital inputs for SDMMC write-protect and card-detect.
- `PG9` through `PG13`: digital outputs for info LEDs.

Current firmware-facing pin aliases:

| Concept | STM32 pin |
| --- | --- |
| `led_sleep` | `PG9` |
| `led_flash` | `PG10` |
| `led_can` | `PG11` |
| `led_fault` | `PG12` |
| `led_operational` / `led_status` | `PG13` |
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
| `cooling_pump` | `PE13` |
| `electrovalve` | `PE15` |
| `pressure_regulator_in` | `PA4` |
| `pressure_regulator_out` | `PA5` |
| `brake_reset` | `PD15` |
| `brake_fault` | `PD14` |

Ethernet RMII is intentionally not listed in `Pinout.hpp`. Use ST-LIB pinset selection instead.

For VCU board Ethernet with LAN8700, use:

```cpp
ST_LIB::EthernetDomain::PINSET_H11
```

`PINSET_H11` includes H11 RMII pins and no RXER pin.

## Build And Verification

Prefer the local CLI:

```bash
./hyper build main --preset board-debug --board-name VCU
./hyper build main --preset board-debug-eth-lan8700 --board-name VCU
```

Known passing checks:

- `./hyper build main --preset board-debug --board-name VCU`
- `./hyper build main --preset board-debug-eth-lan8700 --board-name VCU`
- `python3 -m py_compile hyper`

Do not restore `Core/Src/Runes/generated_metadata.cpp` after builds. The user explicitly requested that generated metadata stay as generated.

## Altium And Hardware Inputs

Current hardware inputs in `docs/`:

- `Schematic PDF_[No Variations].pdf`
- `VCU_H11.zip`
- `Gerber for PCB.PcbDoc.zip`
- `STEP_[No Variations] for PCB.PcbDoc.step`

The Altium source archive is more authoritative than PDF OCR/text extraction. Use the `.SchDoc` records for pin/net extraction when possible.

## Open Decisions

These need confirmation before implementing more runtime behavior:

- Active level and safe default for every output.
- Pull-up/pull-down requirements for digital inputs.
- Sensor calibration and units for pressure, flow, and NTC temperature.
- Protection thresholds and diagnostic IDs.
- Whether brake fault should be EXTI-triggered, a polled protection, or both.
- Exact flow-sensor abstraction. ST-LIB currently provides TIM23 input-capture support, but no dedicated flow/pulse sensor wrapper has been selected.
- Packet contract cleanup and whether generated ADE names should also be normalized to English.

## Maintenance Protocol

Update this file when a durable fact becomes known. Good updates include:

- New confirmed pin mappings or active levels.
- Chosen ST-LIB device request patterns.
- Confirmed VCU operational states and transitions.
- Protection and diagnostic policies.
- Build, flash, debug, or hardware validation workflows that were actually run.
- User preferences that affect future work.

Do not add transient command output, speculative assumptions, or one-off debugging notes. If a fact is inferred rather than confirmed, label it as inferred and keep it in `Open Decisions` until confirmed.
