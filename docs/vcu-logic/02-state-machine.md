# State Machine

The current H11 VCU state machine is an explicit product FSM implemented in
`Core/Inc/StateMachine/VCU_StateMachine.hpp` and specified formally in
`docs/vcu_state_machine.yaml`.

## States

| State | Meaning | Entry behavior |
| --- | --- | --- |
| `Idle` | Safe initial state waiting for required communication links. | Brake, open contactors, publish state. |
| `Connected` | Required links are connected and the VCU waits for high-level orders. | Brake, open contactors, publish state. |
| `Manteinance` | Maintenance mode. The spelling follows the current order/state contract. | Unbrake, publish state. |
| `Precharging` | Precharge has been requested. | Send the formal HVBMS precharge request, publish state. |
| `HVActive` | High voltage is active. | Publish state. |
| `Ready` | Vehicle is unbraked and ready for propulsion or levitation mode selection. | Unbrake, publish state. |
| `Propulsion` | Propulsion mode. | Start propulsion, publish state. |
| `StaticLevitation` | Static levitation mode. | Start static levitation, publish state. |
| `DynamicLevitation` | Dynamic levitation mode. | Start dynamic levitation, publish state. |
| `Fault` | Faulted safe state. | Open SDC formally, turn on fault LED, propagate fault, open contactors, brake, publish state. |

`open_sdc` is currently formal only: H11 exposes `sdc_closed` as an input and
protection, not as a controllable output.

## Required Links

`Idle` transitions to `Connected` when the VCU is connected to:

- control station;
- HVBMS;
- PCU;
- LCU.

The precharge flow is named as HVBMS-facing in the formal FSM. The current code
uses the existing high-voltage remote link/state packet until a dedicated HVBMS
socket and state packet are defined.

## Transitions

| From | To | Trigger |
| --- | --- | --- |
| `Idle` | `Connected` | Required links are connected. |
| `Connected` | `Manteinance` | `MANTEINANCE` order. |
| `Connected` | `Precharging` | `Precharge` order. |
| `Manteinance` | `Connected` | `Stop` order. |
| `Precharging` | `Connected` | `Stop` order. |
| `Precharging` | `HVActive` | Precharge completion from the high-voltage board. |
| `HVActive` | `Connected` | `Stop` order. |
| `HVActive` | `Ready` | `Unbrake` order. |
| `Ready` | `Connected` | `Stop` order. |
| `Ready` | `HVActive` | `Brake` order. |
| `Ready` | `Propulsion` | `Propulsion` order. |
| `Ready` | `StaticLevitation` | `Static levitation` order. |
| `Ready` | `DynamicLevitation` | `Dynamic levitation` order. |
| `StaticLevitation` | `DynamicLevitation` | `Dynamic levitation` order. |
| `Propulsion` | `Connected` | `Stop` order. |
| `StaticLevitation` | `Connected` | `Stop` order. |
| `DynamicLevitation` | `Connected` | `Stop` order. |
| Any state | `Fault` | Protection trigger or control-station disconnect after connection. |

The `Stop` exits from `Propulsion`, `StaticLevitation`, and
`DynamicLevitation` are included so their exit actions can run:

- exiting `Propulsion` stops propelling;
- exiting `StaticLevitation` stops levitating;
- exiting `DynamicLevitation` stops propelling and levitating.

## Active-Mode Orders

Low-level propulsion orders are only forwarded in `Propulsion` and
`DynamicLevitation`.

Low-level levitation orders are only forwarded in `StaticLevitation` and
`DynamicLevitation`.

## Fault Handling

ST-LIB protections and `FaultController` remain the trigger infrastructure.
The formal VCU FSM also exposes `Fault` as a state and telemetry value. Fault
entry applies the safe-output policy currently implemented by the VCU:

- turn on fault LED;
- propagate fault to remote boards when connected;
- request contactor opening;
- brake;
- synchronize telemetry.
