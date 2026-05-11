# VCU-H11

Vehicle Control Unit firmware for Hyperloop UPV.

The VCU is the pod-level controller for the H11 custom board based on the STM32H723ZGT6. It coordinates the vehicle-facing control and telemetry functions around cooling, pressure regulation, braking support signals, SDC state, flow and temperature sensing, CAN, Ethernet, SD storage, and board status indication.

## Responsibilities

The firmware is responsible for:

- Initializing the VCU hardware through ST-LIB.
- Managing VCU operational modes.
- Reading pressure, temperature, flow, SDC, brake, and SD-card related inputs.
- Driving status LEDs and VCU actuators such as the cooling pump, electrovalve, pressure regulator command path, brake reset, and CAN silent control.
- Publishing VCU telemetry and receiving commands through the generated packet interface.
- Evaluating protections and reporting diagnostics through ST-LIB infrastructure.

## Architecture

VCU-H11 is a C++23 STM32 firmware project built around ST-LIB.

Application code is organized around these entry points:

- `Core/Src/main.cpp`: firmware entry point.
- `Core/Inc/VCU.hpp`: VCU facade and `ST_LIB::Board<...>` definition.
- `Core/Inc/VCU_TYPES.hpp`: Board device declarations, runtime instances, sensors, and VCU state.
- `Core/Inc/Pinout/Pinout.hpp`: board pin aliases with firmware-facing names.
- `Core/Inc/StateMachine/VCU_StateMachine.hpp`: operational state machine.
- `Core/Inc/Code_generation/JSON_ADE/boards/VCU/`: packet schema inputs for generated communication code.

All hardware access should go through ST-LIB. Application code should use the highest-level ST-LIB abstraction that fits the hardware role, for example `LinearSensor` for linear analog sensors, `NTC` for NTC channels, `SensorInterrupt`/`EXTIDomain` for edge-triggered sensors, and Board-managed domain instances for lower-level peripherals.

Fault state ownership belongs to ST-LIB infrastructure. The VCU application state machine should describe operational modes, while protections and diagnostics handle fault behavior.

## Hardware References

Hardware design references are stored under `docs/`:

- `VCU.ioc`: STM32CubeMX pin and peripheral role reference.
- `VCU_H11.zip`: Altium project archive.
- `Schematic PDF_[No Variations].pdf`: schematic PDF export.
- `Gerber for PCB.PcbDoc.zip`: PCB fabrication output.
- `STEP_[No Variations] for PCB.PcbDoc.step`: mechanical model.

## Build And Flash

Use the local `hyper` helper CLI for common workflows:

```sh
./hyper doctor
./hyper build main --preset board-debug --board-name VCU
./hyper build main --preset board-debug-eth-lan8700 --board-name VCU
./hyper run main --preset board-debug --board-name VCU --uart
```

Useful environment variables:

- `HYPER_DEFAULT_PRESET`
- `HYPER_FLASH_METHOD`
- `HYPER_UART_PORT`
- `HYPER_UART_BAUD`
- `HYPER_UART_TOOL`

For UART sessions, `tio` is the recommended terminal tool.

## Packet Generation

The packet interface is generated from the ADJ schema for `BOARD_NAME=VCU`:

```text
Core/Inc/Code_generation/JSON_ADE/boards/VCU/
```

CMake regenerates packet headers during configure. Manual regeneration is also available:

```sh
python3 Core/Inc/Code_generation/Generator.py VCU
```

Generated packet headers are build outputs and should not be edited by hand:

- `Core/Inc/Communications/Packets/DataPackets.hpp`
- `Core/Inc/Communications/Packets/OrderPackets.hpp`

## Documentation

Project-specific AI and maintenance context:

- `AGENTS.md`
- `docs/ai/vcu-context.md`
