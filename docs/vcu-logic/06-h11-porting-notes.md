# H11 Porting Notes

## Port Intent, Not Old Infrastructure

The H10 code is valuable for business logic only. H11 uses a newer ST-LIB with a different architecture. Do not preserve these old implementation details:

- direct construction of legacy `PWM`, `DigitalOutput`, `DigitalSensor`, or `LinearSensor` objects outside the H11 `Board` contract;
- H10 `ProtectionManager` state-machine linkage style;
- raw `HeapPacket` and `HeapOrder` registration if generated H11 packet code can express the same behavior;
- a general `Fault` application state.

Preserve the intent:

- state-gated command acceptance;
- local brake, manual-regulator feedback, pump, SDC, flow, pressure, and LED behavior;
- remote order forwarding with acknowledgement;
- telemetry cadence;
- fault on unsafe conditions, lost required peers, emergency stop, and failed acknowledgements.

## Suggested H11 Modules

The H10 classes map naturally to these H11 responsibilities:

| H10 concept | H11 implementation direction |
| --- | --- |
| `VCU` facade | Keep `VCU::init()` and `VCU::update()` as the top-level lifecycle. |
| `VCU_SM` | Implement normal operational modes in `StateMachine/VCU_StateMachine.hpp`; keep fault out of the operational enum. |
| `Comms` | Use generated packet/order code plus a small dispatcher for state-gated order handling. |
| `Actuators` | Use ST-LIB `Board` instances and small domain functions for pumps, brake output, and sensor reads. H11 must not command regulator pressure. |
| `Brakes` | Use ST-LIB output/input/interrupt abstractions and a clear brake policy with confirmed active levels. |
| `Leds` | Keep LED state helpers or scheduled tasks using H11 LED aliases. |
| H10 protections | Convert to ST-LIB protections, diagnostics, `FaultController`, and fault-policy callbacks. |

## Implementation Sequence

1. Define required remote peers for H11. At minimum H10 required control station, HVSCU, and PCU before `Operational`.
2. Implement operational states without a `Fault` enum value: `Idle`, `EndOfRun`, `Energized`, `Ready`, `Demonstration`, `Recovery`.
3. Add clear state transition predicates for contactors, brakes, recovery, and demonstration bitfield.
4. Add local brake behavior after active levels are confirmed. Keep regulator pressure manual and firmware-observed only.
5. Add order handlers that only raise flags or enqueue commands from packet callbacks.
6. Add dispatcher checks that enforce allowed states before acting.
7. Add remote acknowledgement tracking and timeout diagnostics.
8. Add ST-LIB protections for SDC, brake fault, pressure limits, temperature limits, and any remote-ack timeout faults.
9. Extend generated telemetry to cover the full VCU behavior when measurements and units are confirmed.

## Naming Translation

Prefer H11 English names over H10 names:

| H10 name | H11-style concept |
| --- | --- |
| `Potencia_refri` | Cooling pump power or cooling pump command. |
| `PresionAlta` | High pressure. |
| `PresionRegulador` | Manual-regulator pressure feedback. |
| `PresionFrenos` | Brake pressure. |
| `PresionCapsula` | Capsule pressure. |
| `Sdc` | SDC closed or SDC state. |
| `breaks_first_time` | Brake first-time flag. |
| `All_reeds` | All brake reeds active. |

## Current H11 Implementation Notes

The H11 code now implements the major H10 business-logic gaps deliberately rather than copying old infrastructure:

- command dispatchers exist for brake, unbrake, contactors, pumps, recovery, and remote-board commands;
- operational state transitions are derived from contactor status, brake status, recovery request, and the demonstration bitfield;
- generated sockets cover the control station, HVSCU, PCU, LCU, and BCU paths used by the dispatcher;
- remote acknowledgements are tracked for HVSCU, PCU, LCU, and booster behavior;
- H11 does not expose pressure-regulator control because a manual regulator is used;
- fault entry turns local outputs to their safe firmware-owned states and commands the brake active.

The remaining open decisions are calibration and hardware-validation items:

- pressure and temperature calibration still use placeholders;
- flow measurement wrapper is not finalized;
- fault/protection thresholds and diagnostic IDs are still open.

These docs define the intended behavior and the known H11 differences from the H10 reference.
