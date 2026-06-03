# Ejemplo completo de máquina de estados declarativa

Este documento contiene una máquina de estados de ejemplo que usa prácticamente todos los elementos definibles del formato propuesto.

El ejemplo representa un cargador de batería de alto voltaje con:

- Estado inicial.
- Estados normales.
- Estado final.
- Estado de fallo.
- Estados jerárquicos.
- Acciones `on_entry`, `on_exit` y `do`.
- Timeouts.
- Eventos con payload.
- Guards por expresión y por función.
- Acciones simples y parametrizadas.
- Transiciones normales.
- Transiciones globales.
- Transiciones internas.
- Eventos ignorados.
- Eventos diferidos.
- Invariantes.
- Política de eventos no manejados.
- Prioridad explícita.
- Metadata para generación de código.

---

## Máquina de estados de ejemplo

```yaml
schema_version: "1.0"

machine: HighVoltageChargerFSM

description: >
  Máquina de estados para controlar un cargador de batería de alto voltaje.
  Incluye inicialización, autodiagnóstico, espera, precarga, carga activa,
  balanceo, finalización, parada ordenada y gestión de fallos.

metadata:
  author: Example
  target_language: c
  intended_platform: embedded
  tick_period_ms: 1
  codegen:
    generate_switch_dispatcher: true
    generate_transition_table: true
    generate_unit_tests: true
    generate_dot_diagram: true
    externalize_guards: true
    externalize_actions: true
    state_enum_prefix: HVCHG_STATE_
    event_enum_prefix: HVCHG_EVENT_
    function_prefix: HVChargerFSM_

initial_state: BOOT

unhandled_event_policy:
  behavior: log
  action: log_unhandled_event
  description: >
    Los eventos no manejados se registran, pero no cambian el estado de la FSM.

context:
  description: Datos internos consultados o modificados por la FSM.
  variables:
    - name: battery_voltage
      type: float
      unit: V
      access: read
      initial_value: 0.0
      range:
        min: 0.0
        max: 500.0
      description: Tensión medida en la batería.

    - name: dc_bus_voltage
      type: float
      unit: V
      access: read
      initial_value: 0.0
      range:
        min: 0.0
        max: 500.0
      description: Tensión del bus DC del cargador.

    - name: charge_current
      type: float
      unit: A
      access: read
      initial_value: 0.0
      range:
        min: -10.0
        max: 100.0
      description: Corriente medida de carga.

    - name: temperature_heatsink
      type: float
      unit: degC
      access: read
      initial_value: 25.0
      range:
        min: -40.0
        max: 150.0
      description: Temperatura del disipador.

    - name: target_voltage
      type: float
      unit: V
      access: read_write
      initial_value: 400.0
      description: Tensión objetivo de carga.

    - name: max_charge_current
      type: float
      unit: A
      access: read_write
      initial_value: 20.0
      description: Corriente máxima permitida.

    - name: fault_flags
      type: uint32_t
      access: read_write
      initial_value: 0
      description: Máscara de bits con fallos activos.

    - name: fault_reason
      type: FaultReason
      access: read_write
      initial_value: NONE
      description: Última causa de fallo registrada.

    - name: state_elapsed_ms
      type: uint32_t
      unit: ms
      access: read_write
      initial_value: 0
      description: Tiempo transcurrido desde la entrada al estado actual.

    - name: retry_counter
      type: uint8_t
      access: read_write
      initial_value: 0
      range:
        min: 0
        max: 3
      description: Contador de reintentos de arranque.

constants:
  - name: PRECHARGE_MIN_VOLTAGE
    type: float
    value: 50.0
    unit: V

  - name: PRECHARGE_TARGET_RATIO
    type: float
    value: 0.95

  - name: MAX_HEATSINK_TEMP
    type: float
    value: 85.0
    unit: degC

  - name: MAX_RETRIES
    type: uint8_t
    value: 3

states:
  - name: BOOT
    type: normal
    description: Estado inicial tras reset del microcontrolador.
    on_entry:
      - initialize_context
      - disable_pwm
      - open_main_contactor
      - open_precharge_contactor
    on_exit:
      - reset_state_timer
    timeout:
      duration_ms: 1000
      event: TIMEOUT
      description: El arranque no debería tardar más de 1 s.
    ignore:
      - ADC_UPDATE
      - STOP
    invariants:
      - pwm_enabled == false
      - main_contactor_closed == false

  - name: SELF_TEST
    type: normal
    description: Ejecuta comprobaciones internas antes de permitir carga.
    on_entry:
      - start_self_test
      - reset_state_timer
    do:
      - poll_self_test
    on_exit:
      - stop_self_test
    timeout:
      duration_ms: 3000
      event: TIMEOUT
      description: Si el autodiagnóstico tarda más de 3 s, se genera fallo.
    defer:
      - START
    invariants:
      - pwm_enabled == false

  - name: IDLE
    type: normal
    description: Sistema seguro esperando orden de arranque.
    on_entry:
      - disable_pwm
      - open_main_contactor
      - open_precharge_contactor
      - clear_runtime_measurements
    ignore:
      - ADC_UPDATE
      - TIMEOUT
    invariants:
      - pwm_enabled == false

  - name: ACTIVE
    type: composite
    description: Estado jerárquico que agrupa las fases activas de carga.
    initial_state: PRECHARGE
    history:
      enabled: true
      type: shallow
    on_entry:
      - reset_state_timer
      - enable_measurements
    on_exit:
      - disable_pwm
      - open_precharge_contactor
      - open_main_contactor
    substates:
      - name: PRECHARGE
        type: normal
        description: Igualación gradual del bus DC antes de cerrar contactor principal.
        on_entry:
          - close_precharge_contactor
          - reset_state_timer
        on_exit:
          - open_precharge_contactor
        do:
          - update_measurements
          - monitor_precharge_progress
        timeout:
          duration_ms: 5000
          event: TIMEOUT
          description: La precarga debe completarse antes de 5 s.
        invariants:
          - pwm_enabled == false
          - precharge_contactor_closed == true

      - name: CLOSE_MAIN_CONTACTOR
        type: normal
        description: Cierra el contactor principal cuando la precarga es válida.
        on_entry:
          - close_main_contactor
          - reset_state_timer
        timeout:
          duration_ms: 500
          event: TIMEOUT
        invariants:
          - precharge_contactor_closed == true

      - name: CHARGING_CC
        type: normal
        description: Fase de carga en corriente constante.
        on_entry:
          - enable_pwm
          - configure_current_control
          - reset_state_timer
        do:
          - update_measurements
          - run_current_control_loop
          - monitor_temperature
        on_exit:
          - freeze_current_controller
        invariants:
          - pwm_enabled == true
          - main_contactor_closed == true

      - name: CHARGING_CV
        type: normal
        description: Fase de carga en tensión constante.
        on_entry:
          - configure_voltage_control
          - reset_state_timer
        do:
          - update_measurements
          - run_voltage_control_loop
          - monitor_temperature
        on_exit:
          - freeze_voltage_controller
        invariants:
          - pwm_enabled == true
          - main_contactor_closed == true

      - name: BALANCING
        type: normal
        description: Balanceo de celdas al final de la carga.
        on_entry:
          - enable_balancing
          - reset_state_timer
        do:
          - update_measurements
          - run_balancing_control
        on_exit:
          - disable_balancing
        timeout:
          duration_ms: 600000
          event: TIMEOUT
          description: El balanceo no debe exceder 10 minutos.
        defer:
          - START
        invariants:
          - main_contactor_closed == true

  - name: STOPPING
    type: normal
    description: Parada ordenada del sistema.
    on_entry:
      - disable_pwm
      - ramp_down_outputs
      - reset_state_timer
    do:
      - monitor_output_shutdown
    timeout:
      duration_ms: 2000
      event: TIMEOUT
    invariants:
      - pwm_enabled == false

  - name: DONE
    type: final
    description: Carga completada correctamente.
    on_entry:
      - disable_pwm
      - open_precharge_contactor
      - open_main_contactor
      - mark_charge_complete
    ignore:
      - ADC_UPDATE
      - TIMEOUT

  - name: FAULT
    type: fault
    description: Estado seguro tras detectar un fallo.
    on_entry:
      - disable_pwm
      - disable_balancing
      - open_precharge_contactor
      - open_main_contactor
      - log_fault
    do:
      - blink_fault_led
    ignore:
      - START
      - ADC_UPDATE
      - TIMEOUT
    invariants:
      - pwm_enabled == false
      - main_contactor_closed == false

  - name: LOCKED_OUT
    type: fault
    description: Estado de bloqueo tras demasiados reintentos fallidos.
    on_entry:
      - disable_pwm
      - open_precharge_contactor
      - open_main_contactor
      - log_lockout
    ignore:
      - START
      - ADC_UPDATE
      - RESET

events:
  - name: POWER_ON
    source: system
    description: Evento generado al inicializar la FSM.

  - name: SELF_TEST_OK
    source: self_test_task
    description: Autodiagnóstico completado correctamente.

  - name: SELF_TEST_FAILED
    source: self_test_task
    payload:
      - name: diagnostic_code
        type: uint32_t
    description: El autodiagnóstico ha fallado.

  - name: START
    source: user_command
    description: Solicitud de inicio de carga.

  - name: STOP
    source: user_command
    description: Solicitud de parada ordenada.

  - name: EMERGENCY_STOP
    source: safety_input
    priority: 0
    description: Parada de emergencia de máxima prioridad.

  - name: ADC_UPDATE
    source: periodic_task
    payload:
      - name: battery_voltage
        type: float
        unit: V
      - name: charge_current
        type: float
        unit: A
      - name: temperature_heatsink
        type: float
        unit: degC
    description: Nuevas medidas analógicas disponibles.

  - name: PRECHARGE_COMPLETE
    source: internal
    description: La precarga ha alcanzado el umbral requerido.

  - name: MAIN_CONTACTOR_CLOSED
    source: io_feedback
    description: Realimentación de contactor principal cerrado.

  - name: VOLTAGE_TARGET_REACHED
    source: control_loop
    description: Se ha alcanzado la tensión objetivo.

  - name: CURRENT_TAPERED
    source: control_loop
    description: La corriente ha bajado por debajo del umbral de fin de carga.

  - name: BALANCING_COMPLETE
    source: balancing_task
    description: Balanceo de celdas completado.

  - name: TIMEOUT
    source: generated_by_fsm
    description: Evento sintético generado por timeout de estado.

  - name: FAULT_DETECTED
    source: safety_monitor
    payload:
      - name: fault_reason
        type: FaultReason
    priority: 1
    description: Se ha detectado un fallo.

  - name: RESET
    source: user_command
    description: Solicitud de reset de fallo.

guards:
  - name: always
    expression: true
    description: Guard siempre verdadero.

  - name: no_fault
    expression: fault_flags == 0
    dependencies:
      - fault_flags
    description: No hay fallos activos.

  - name: battery_connected
    expression: battery_voltage > PRECHARGE_MIN_VOLTAGE
    dependencies:
      - battery_voltage
      - PRECHARGE_MIN_VOLTAGE
    description: La batería parece estar conectada.

  - name: precharge_voltage_reached
    expression: dc_bus_voltage >= battery_voltage * PRECHARGE_TARGET_RATIO
    dependencies:
      - dc_bus_voltage
      - battery_voltage
      - PRECHARGE_TARGET_RATIO
    description: El bus DC está suficientemente cerca de la tensión de batería.

  - name: main_contactor_feedback_ok
    function: guard_main_contactor_feedback_ok
    description: Comprueba por GPIO o feedback externo que el contactor principal está cerrado.

  - name: voltage_target_reached
    expression: battery_voltage >= target_voltage
    dependencies:
      - battery_voltage
      - target_voltage
    description: La batería ha alcanzado la tensión objetivo.

  - name: charge_current_below_finish_threshold
    function: guard_charge_current_below_finish_threshold
    description: La corriente ha bajado lo suficiente para finalizar la fase CV.

  - name: current_too_high
    expression: charge_current > max_charge_current
    dependencies:
      - charge_current
      - max_charge_current
    description: Sobrecorriente de carga.

  - name: temperature_too_high
    expression: temperature_heatsink > MAX_HEATSINK_TEMP
    dependencies:
      - temperature_heatsink
      - MAX_HEATSINK_TEMP
    description: Sobretemperatura del disipador.

  - name: can_retry
    expression: retry_counter < MAX_RETRIES
    dependencies:
      - retry_counter
      - MAX_RETRIES
    description: Todavía quedan reintentos permitidos.

  - name: cannot_retry
    expression: retry_counter >= MAX_RETRIES
    dependencies:
      - retry_counter
      - MAX_RETRIES
    description: Se han agotado los reintentos.

  - name: fault_cleared
    function: guard_fault_cleared
    description: El fallo ha desaparecido y el sistema puede volver a IDLE.

actions:
  - name: initialize_context
    function: action_initialize_context
    modifies:
      - fault_flags
      - fault_reason
      - retry_counter
      - state_elapsed_ms
    blocking: false
    description: Inicializa variables internas de la FSM.

  - name: reset_state_timer
    function: action_reset_state_timer
    modifies:
      - state_elapsed_ms
    blocking: false

  - name: disable_pwm
    function: action_disable_pwm
    blocking: false
    description: Deshabilita las señales PWM de la etapa de potencia.

  - name: enable_pwm
    function: action_enable_pwm
    blocking: false
    description: Habilita las señales PWM.

  - name: open_main_contactor
    function: action_open_main_contactor
    blocking: false

  - name: close_main_contactor
    function: action_close_main_contactor
    blocking: false

  - name: open_precharge_contactor
    function: action_open_precharge_contactor
    blocking: false

  - name: close_precharge_contactor
    function: action_close_precharge_contactor
    blocking: false

  - name: start_self_test
    function: action_start_self_test
    blocking: false

  - name: poll_self_test
    function: action_poll_self_test
    blocking: false

  - name: stop_self_test
    function: action_stop_self_test
    blocking: false

  - name: enable_measurements
    function: action_enable_measurements
    blocking: false

  - name: update_measurements
    function: action_update_measurements
    blocking: false

  - name: monitor_precharge_progress
    function: action_monitor_precharge_progress
    blocking: false

  - name: configure_current_control
    function: action_configure_current_control
    blocking: false

  - name: configure_voltage_control
    function: action_configure_voltage_control
    blocking: false

  - name: run_current_control_loop
    function: action_run_current_control_loop
    blocking: false

  - name: run_voltage_control_loop
    function: action_run_voltage_control_loop
    blocking: false

  - name: freeze_current_controller
    function: action_freeze_current_controller
    blocking: false

  - name: freeze_voltage_controller
    function: action_freeze_voltage_controller
    blocking: false

  - name: monitor_temperature
    function: action_monitor_temperature
    blocking: false

  - name: enable_balancing
    function: action_enable_balancing
    blocking: false

  - name: run_balancing_control
    function: action_run_balancing_control
    blocking: false

  - name: disable_balancing
    function: action_disable_balancing
    blocking: false

  - name: ramp_down_outputs
    function: action_ramp_down_outputs
    blocking: false

  - name: monitor_output_shutdown
    function: action_monitor_output_shutdown
    blocking: false

  - name: mark_charge_complete
    function: action_mark_charge_complete
    blocking: false

  - name: clear_runtime_measurements
    function: action_clear_runtime_measurements
    blocking: false

  - name: log_unhandled_event
    function: action_log_unhandled_event
    blocking: false

  - name: log_fault
    function: action_log_fault
    blocking: false

  - name: log_lockout
    function: action_log_lockout
    blocking: false

  - name: blink_fault_led
    function: action_blink_fault_led
    blocking: false

  - name: increment_retry_counter
    function: action_increment_retry_counter
    modifies:
      - retry_counter
    blocking: false

  - name: clear_faults
    function: action_clear_faults
    modifies:
      - fault_flags
      - fault_reason
    blocking: false

  - name: set_fault_reason
    function: action_set_fault_reason
    parameters:
      - name: reason
        type: FaultReason
    modifies:
      - fault_reason
    blocking: false

  - name: log_event
    function: action_log_event
    parameters:
      - name: event_id
        type: LogEventId
    blocking: false

transitions:
  # -------------------------------------------------------------------------
  # Arranque e inicialización
  # -------------------------------------------------------------------------

  - from: BOOT
    event: POWER_ON
    to: SELF_TEST
    actions:
      - reset_state_timer
    priority: 10
    description: Tras inicialización básica, se entra en autodiagnóstico.

  - from: BOOT
    event: TIMEOUT
    to: FAULT
    actions:
      - name: set_fault_reason
        args:
          reason: BOOT_TIMEOUT
      - log_fault
    priority: 5
    description: Fallo si BOOT no progresa a tiempo.

  - from: SELF_TEST
    event: SELF_TEST_OK
    to: IDLE
    actions:
      - clear_faults
    priority: 10

  - from: SELF_TEST
    event: SELF_TEST_FAILED
    to: FAULT
    actions:
      - name: set_fault_reason
        args:
          reason: SELF_TEST_FAILED
      - log_fault
    priority: 5

  - from: SELF_TEST
    event: TIMEOUT
    to: FAULT
    actions:
      - name: set_fault_reason
        args:
          reason: SELF_TEST_TIMEOUT
      - log_fault
    priority: 5

  # -------------------------------------------------------------------------
  # Entrada en carga
  # -------------------------------------------------------------------------

  - from: IDLE
    event: START
    guard: battery_connected && no_fault
    to: ACTIVE.PRECHARGE
    actions:
      - reset_state_timer
      - name: log_event
        args:
          event_id: CHARGE_STARTED
    priority: 20
    description: Inicia la secuencia de carga si la batería está conectada y no hay fallos.

  - from: IDLE
    event: START
    guard: battery_connected == false
    to: FAULT
    actions:
      - name: set_fault_reason
        args:
          reason: BATTERY_NOT_CONNECTED
      - log_fault
    priority: 10
    description: Rechaza el arranque si no se detecta batería.

  # -------------------------------------------------------------------------
  # Secuencia activa: precarga y contactor principal
  # -------------------------------------------------------------------------

  - from: ACTIVE.PRECHARGE
    event: ADC_UPDATE
    guard: current_too_high
    to: FAULT
    actions:
      - name: set_fault_reason
        args:
          reason: PRECHARGE_OVERCURRENT
      - log_fault
    priority: 1
    description: Protección prioritaria durante precarga.

  - from: ACTIVE.PRECHARGE
    event: ADC_UPDATE
    guard: precharge_voltage_reached
    to: ACTIVE.CLOSE_MAIN_CONTACTOR
    actions:
      - name: log_event
        args:
          event_id: PRECHARGE_COMPLETED
    priority: 20

  - from: ACTIVE.PRECHARGE
    event: ADC_UPDATE
    guard: no_fault
    to: ACTIVE.PRECHARGE
    internal: true
    actions:
      - update_measurements
      - monitor_precharge_progress
    priority: 100
    description: Transición interna; no ejecuta on_exit ni on_entry.

  - from: ACTIVE.PRECHARGE
    event: TIMEOUT
    to: FAULT
    actions:
      - name: set_fault_reason
        args:
          reason: PRECHARGE_TIMEOUT
      - log_fault
    priority: 5

  - from: ACTIVE.CLOSE_MAIN_CONTACTOR
    event: MAIN_CONTACTOR_CLOSED
    guard: main_contactor_feedback_ok
    to: ACTIVE.CHARGING_CC
    actions:
      - open_precharge_contactor
      - name: log_event
        args:
          event_id: MAIN_CONTACTOR_READY
    priority: 10

  - from: ACTIVE.CLOSE_MAIN_CONTACTOR
    event: TIMEOUT
    to: FAULT
    actions:
      - name: set_fault_reason
        args:
          reason: MAIN_CONTACTOR_TIMEOUT
      - log_fault
    priority: 5

  # -------------------------------------------------------------------------
  # Carga CC/CV
  # -------------------------------------------------------------------------

  - from: ACTIVE.CHARGING_CC
    event: ADC_UPDATE
    guard: current_too_high
    to: FAULT
    actions:
      - name: set_fault_reason
        args:
          reason: CHARGE_OVERCURRENT
      - log_fault
    priority: 1

  - from: ACTIVE.CHARGING_CC
    event: ADC_UPDATE
    guard: temperature_too_high
    to: FAULT
    actions:
      - name: set_fault_reason
        args:
          reason: OVERTEMPERATURE
      - log_fault
    priority: 2

  - from: ACTIVE.CHARGING_CC
    event: ADC_UPDATE
    guard: voltage_target_reached
    to: ACTIVE.CHARGING_CV
    actions:
      - name: log_event
        args:
          event_id: ENTER_CV_MODE
    priority: 20

  - from: ACTIVE.CHARGING_CC
    event: ADC_UPDATE
    guard: no_fault
    to: ACTIVE.CHARGING_CC
    internal: true
    actions:
      - update_measurements
      - run_current_control_loop
      - monitor_temperature
    priority: 100

  - from: ACTIVE.CHARGING_CV
    event: ADC_UPDATE
    guard: current_too_high
    to: FAULT
    actions:
      - name: set_fault_reason
        args:
          reason: CHARGE_OVERCURRENT
      - log_fault
    priority: 1

  - from: ACTIVE.CHARGING_CV
    event: ADC_UPDATE
    guard: temperature_too_high
    to: FAULT
    actions:
      - name: set_fault_reason
        args:
          reason: OVERTEMPERATURE
      - log_fault
    priority: 2

  - from: ACTIVE.CHARGING_CV
    event: ADC_UPDATE
    guard: charge_current_below_finish_threshold
    to: ACTIVE.BALANCING
    actions:
      - name: log_event
        args:
          event_id: ENTER_BALANCING
    priority: 20

  - from: ACTIVE.CHARGING_CV
    event: ADC_UPDATE
    guard: no_fault
    to: ACTIVE.CHARGING_CV
    internal: true
    actions:
      - update_measurements
      - run_voltage_control_loop
      - monitor_temperature
    priority: 100

  # -------------------------------------------------------------------------
  # Balanceo y finalización
  # -------------------------------------------------------------------------

  - from: ACTIVE.BALANCING
    event: BALANCING_COMPLETE
    to: DONE
    actions:
      - name: log_event
        args:
          event_id: CHARGE_COMPLETED
    priority: 20

  - from: ACTIVE.BALANCING
    event: TIMEOUT
    to: DONE
    actions:
      - name: log_event
        args:
          event_id: BALANCING_TIMEOUT_FINISHING_ANYWAY
    priority: 30
    description: En este ejemplo, el timeout de balanceo no se considera fallo crítico.

  - from: ACTIVE.BALANCING
    event: ADC_UPDATE
    guard: temperature_too_high
    to: FAULT
    actions:
      - name: set_fault_reason
        args:
          reason: OVERTEMPERATURE
      - log_fault
    priority: 2

  # -------------------------------------------------------------------------
  # Parada ordenada
  # -------------------------------------------------------------------------

  - from: ACTIVE
    event: STOP
    to: STOPPING
    actions:
      - name: log_event
        args:
          event_id: STOP_REQUESTED
    priority: 10
    description: Transición desde cualquier subestado de ACTIVE hacia parada ordenada.

  - from: STOPPING
    event: ADC_UPDATE
    guard: no_fault
    to: STOPPING
    internal: true
    actions:
      - monitor_output_shutdown
    priority: 100

  - from: STOPPING
    event: TIMEOUT
    to: IDLE
    actions:
      - open_main_contactor
      - open_precharge_contactor
      - name: log_event
        args:
          event_id: STOP_COMPLETED
    priority: 20

  # -------------------------------------------------------------------------
  # Fallos, reset y reintentos
  # -------------------------------------------------------------------------

  - from: FAULT
    event: RESET
    guard: fault_cleared && can_retry
    to: IDLE
    actions:
      - clear_faults
      - increment_retry_counter
      - name: log_event
        args:
          event_id: FAULT_RESET
    priority: 10

  - from: FAULT
    event: RESET
    guard: fault_cleared && cannot_retry
    to: LOCKED_OUT
    actions:
      - name: log_event
        args:
          event_id: LOCKOUT_ENTERED
    priority: 5

  # -------------------------------------------------------------------------
  # Transiciones globales de seguridad
  # -------------------------------------------------------------------------

  - from: "*"
    event: EMERGENCY_STOP
    to: FAULT
    actions:
      - disable_pwm
      - disable_balancing
      - open_precharge_contactor
      - open_main_contactor
      - name: set_fault_reason
        args:
          reason: EMERGENCY_STOP
      - log_fault
    priority: 0
    description: Transición global de máxima prioridad.

  - from: "*"
    event: FAULT_DETECTED
    to: FAULT
    actions:
      - disable_pwm
      - disable_balancing
      - open_precharge_contactor
      - open_main_contactor
      - name: set_fault_reason
        args:
          reason: FROM_EVENT_PAYLOAD
      - log_fault
    priority: 1
    description: Cualquier fallo detectado lleva a estado seguro.
```

---

## Notas sobre el ejemplo

### 1. Estados jerárquicos

El estado `ACTIVE` contiene subestados:

```text
ACTIVE.PRECHARGE
ACTIVE.CLOSE_MAIN_CONTACTOR
ACTIVE.CHARGING_CC
ACTIVE.CHARGING_CV
ACTIVE.BALANCING
```

Esto permite definir una transición común:

```yaml
- from: ACTIVE
  event: STOP
  to: STOPPING
```

Esa transición aplica desde cualquier subestado de `ACTIVE`.

---

### 2. Transiciones internas

Ejemplo:

```yaml
- from: ACTIVE.CHARGING_CC
  event: ADC_UPDATE
  guard: no_fault
  to: ACTIVE.CHARGING_CC
  internal: true
  actions:
    - update_measurements
    - run_current_control_loop
```

Al estar marcada como `internal: true`, no se ejecutan `on_exit` ni `on_entry` del estado.

---

### 3. Prioridades

Las transiciones de protección tienen prioridad menor numéricamente:

```yaml
priority: 1
```

Las transiciones normales tienen prioridad más baja:

```yaml
priority: 20
```

La regla usada es:

```text
menor número = mayor prioridad
```

---

### 4. Guards por expresión y por función

Guard por expresión:

```yaml
- name: current_too_high
  expression: charge_current > max_charge_current
```

Guard por función externa:

```yaml
- name: main_contactor_feedback_ok
  function: guard_main_contactor_feedback_ok
```

Esto permite que algunas condiciones sean generables directamente y otras queden como funciones implementadas por firmware.

---

### 5. Acciones parametrizadas

Ejemplo:

```yaml
- name: set_fault_reason
  args:
    reason: OVERTEMPERATURE
```

Un generador de C podría convertirlo en:

```c
action_set_fault_reason(ctx, FAULT_REASON_OVERTEMPERATURE);
```

---

### 6. Eventos con payload

Ejemplo:

```yaml
- name: ADC_UPDATE
  payload:
    - name: battery_voltage
      type: float
      unit: V
```

Esto permite generar estructuras de evento:

```c
typedef struct {
    float battery_voltage;
    float charge_current;
    float temperature_heatsink;
} HVChargerFSM_AdcUpdatePayload;
```

---

### 7. Eventos ignorados y diferidos

Evento ignorado:

```yaml
ignore:
  - ADC_UPDATE
```

Evento diferido:

```yaml
defer:
  - START
```

La diferencia es:

- `ignore`: el evento se descarta de forma explícita.
- `defer`: el evento se guarda para procesarlo más adelante.

---

### 8. Política de eventos no manejados

```yaml
unhandled_event_policy:
  behavior: log
  action: log_unhandled_event
```

Esto evita que el comportamiento ante eventos inesperados quede implícito.

---

### 9. Invariantes

Ejemplo:

```yaml
invariants:
  - pwm_enabled == false
```

Un generador puede convertir esto en asserts, checks de debug o tests automáticos.

---

### 10. Qué partes debería generar automáticamente una herramienta

A partir de este YAML se podrían generar:

- `enum` de estados.
- `enum` de eventos.
- Estructura de contexto.
- Estructuras de payload.
- Prototipos de guards.
- Prototipos de acciones.
- Dispatcher principal.
- Función de inicialización.
- Función de tick para timeouts.
- Tests unitarios básicos.
- Diagrama `.dot` o Mermaid.
- Documentación de comportamiento.
```
