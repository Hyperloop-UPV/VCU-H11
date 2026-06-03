# VCU Logic Documentation

This folder documents the Vehicle Control Unit business logic extracted from the previous board codebase at `/Users/jsaegar/hyper-src/VCU-H10-V2`.

The H10-V2 project used an old ST-LIB API and several implementation details are no longer valid for H11. The durable part is the control intent: what the VCU supervises, which commands it accepts, how it gates them by state, what telemetry it reports, and what conditions push the vehicle into a safe fault response.

Use these documents as the behavioral reference when implementing H11 firmware. Do not copy the old H10 hardware-access style.

## Document Map

- [Purpose and Responsibilities](01-purpose-and-responsibilities.md): what the VCU is responsible for in the vehicle.
- [State Machine](02-state-machine.md): the general and operational states, transitions, and cyclic work.
- [Communications and Orders](03-communications-and-orders.md): sockets, packet flow, order handling, and remote-board acknowledgements.
- [Local Subsystems](04-local-subsystems.md): brakes, pressure regulation, pumps, sensors, and LEDs.
- [Protections and Faults](05-protections-and-faults.md): fault triggers, safety actions, and unresolved safety questions.
- [H11 Porting Notes](06-h11-porting-notes.md): how to carry the H10 behavior into the current H11 architecture.

## Mental Model

The VCU is the vehicle master. It is the coordinator between the operator/control station, local hardware attached to the VCU, and remote vehicle boards such as HVBMS, PCU, and LCU.

At a high level, each loop does four jobs:

1. Maintain infrastructure and communications.
2. Advance the vehicle state machines.
3. Read local sensors and publish telemetry.
4. Process accepted orders, forward remote requests, and fault if safety assumptions are violated.

The VCU does not directly implement every vehicle subsystem. Instead, it arbitrates when other boards may act. For example, it does not close the high-voltage contactors itself; it sends the contactor order to HVBMS, waits until HVBMS reports the expected state, then updates the VCU's own operational state.

## Source Caveats

The inspected H10-V2 snapshot contains legacy and partially disabled code paths:

- `state_machine.json` is a template example. The real VCU state machine is in `Core/Inc/state_machine.hpp`.
- BMSL, LCU, BCU, and some propulsion/charging paths are referenced, but some sockets or orders are commented out in the inspected snapshot.
- Some names are Spanish or legacy abbreviations. H11 firmware should keep English concept names.
- The H10 code modeled `Fault` as a general state. H11 should preserve the fault behavior through ST-LIB protections, diagnostics, and fault policy infrastructure rather than adding `Fault` to the application operational state.
