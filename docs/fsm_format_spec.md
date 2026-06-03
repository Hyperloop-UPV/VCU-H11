# Formato declarativo para definir máquinas de estados finitas

## 1. Objetivo

Este documento define un formato declarativo, legible por humanos y procesable por herramientas, para describir máquinas de estados finitas, o FSMs, de forma suficientemente precisa como para:

- Generar código automáticamente.
- Generar diagramas de estados.
- Generar tests unitarios.
- Revisar el comportamiento del sistema sin leer código imperativo.
- Pasar la especificación a una IA o agente de generación de código sin depender de texto ambiguo.

El formato recomendado es YAML, aunque puede traducirse directamente a JSON.

La idea principal es separar:

- La estructura de la máquina.
- La lógica de transición.
- Las condiciones de transición.
- Las acciones ejecutadas.
- La implementación concreta del hardware o del sistema real.

Una máquina de estados bien definida no debería depender de frases como “cuando pase algo, cambia a tal modo”. Debería declarar explícitamente qué evento provoca qué transición, bajo qué condición y qué acciones se ejecutan.

---

## 2. Modelo formal

Una máquina de estados se puede definir como:

```text
FSM = (S, E, G, A, T, s0, C)
```

Donde:

| Símbolo | Significado |
|---|---|
| `S` | Conjunto de estados |
| `E` | Conjunto de eventos |
| `G` | Conjunto de guards o condiciones booleanas |
| `A` | Conjunto de acciones |
| `T` | Conjunto de transiciones |
| `s0` | Estado inicial |
| `C` | Contexto o datos internos usados por la FSM |

Una transición se puede expresar como:

```text
estado_origen + evento + guard -> estado_destino + acciones
```

Ejemplo:

```text
IDLE + START [battery_connected && no_fault] -> PRECHARGE / enable_precharge(), reset_timer()
```

En YAML:

```yaml
- from: IDLE
  event: START
  guard: battery_connected && no_fault
  to: PRECHARGE
  actions:
    - enable_precharge
    - reset_timer
```

---

## 3. Estructura general del documento

Una especificación completa puede tener esta forma:

```yaml
schema_version: "1.0"
machine: ChargerFSM
description: Máquina de estados para control de carga de batería.

initial_state: IDLE

context:
  variables: []

states: []

events: []

guards: []

actions: []

transitions: []
```

Los campos principales son:

| Campo | Obligatorio | Descripción |
|---|---:|---|
| `schema_version` | Sí | Versión del formato usado |
| `machine` | Sí | Nombre de la máquina de estados |
| `description` | No | Descripción general |
| `initial_state` | Sí | Estado inicial |
| `context` | No | Variables internas usadas por guards y acciones |
| `states` | Sí | Lista o mapa de estados |
| `events` | Sí | Lista de eventos que la máquina acepta |
| `guards` | No | Lista de condiciones booleanas |
| `actions` | No | Lista de acciones ejecutables |
| `transitions` | Sí | Lista de transiciones |

---

## 4. `schema_version`

Indica la versión del formato de especificación.

```yaml
schema_version: "1.0"
```

Esto permite evolucionar el formato en el futuro sin romper generadores de código antiguos.

---

## 5. `machine`

Nombre lógico de la máquina de estados.

```yaml
machine: ChargerFSM
```

Recomendaciones:

- Usar `PascalCase`.
- Evitar espacios.
- Usar un nombre que pueda convertirse fácilmente en prefijo de funciones o tipos.

Por ejemplo, para C:

```c
typedef enum ChargerFSM_State { ... } ChargerFSM_State;
void ChargerFSM_dispatch(...);
```

---

## 6. `description`

Descripción humana del propósito de la máquina.

```yaml
description: >
  Controla el proceso de carga de una batería de alto voltaje,
  incluyendo precarga, carga activa, finalización y fallos.
```

Este campo no debería contener lógica normativa. La lógica debe estar en `transitions`, `guards` y `actions`.

---

## 7. `initial_state`

Estado inicial de la máquina.

```yaml
initial_state: IDLE
```

Debe coincidir exactamente con uno de los estados definidos en `states`.

---

## 8. `context`

El contexto define los datos que la FSM puede consultar o modificar.

Ejemplo:

```yaml
context:
  variables:
    - name: battery_voltage
      type: float
      unit: V
      access: read
      description: Tensión medida en la batería.

    - name: charge_current
      type: float
      unit: A
      access: read
      description: Corriente de carga medida.

    - name: fault_flags
      type: uint32_t
      access: read_write
      description: Máscara de bits con fallos activos.
```

Campos recomendados por variable:

| Campo | Obligatorio | Descripción |
|---|---:|---|
| `name` | Sí | Nombre de la variable |
| `type` | Sí | Tipo lógico o tipo de lenguaje destino |
| `unit` | No | Unidad física, si aplica |
| `access` | No | `read`, `write` o `read_write` |
| `description` | No | Descripción humana |
| `initial_value` | No | Valor inicial |
| `range` | No | Rango válido |

Ejemplo con rango:

```yaml
- name: battery_voltage
  type: float
  unit: V
  range:
    min: 0.0
    max: 500.0
```

---

## 9. `states`

Los estados representan situaciones discretas de la FSM.

Formato simple:

```yaml
states:
  - IDLE
  - PRECHARGE
  - CHARGING
  - DONE
  - FAULT
```

Formato recomendado:

```yaml
states:
  - name: IDLE
    description: Sistema parado esperando orden de arranque.
    on_entry:
      - disable_pwm
    on_exit: []

  - name: CHARGING
    description: Carga activa de la batería.
    on_entry:
      - enable_pwm
    on_exit:
      - disable_pwm
```

Campos posibles:

| Campo | Obligatorio | Descripción |
|---|---:|---|
| `name` | Sí | Nombre del estado |
| `description` | No | Descripción humana |
| `type` | No | Tipo especial: `normal`, `final`, `fault`, etc. |
| `on_entry` | No | Acciones al entrar al estado |
| `on_exit` | No | Acciones al salir del estado |
| `do` | No | Acciones periódicas mientras se permanece en el estado |
| `timeout` | No | Timeout asociado al estado |
| `internal_transitions` | No | Transiciones internas que no cambian de estado |
| `metadata` | No | Información extra para herramientas |

---

## 10. Acciones `on_entry`, `on_exit` y `do`

### `on_entry`

Acciones ejecutadas al entrar en un estado.

```yaml
on_entry:
  - enable_pwm
  - reset_state_timer
```

### `on_exit`

Acciones ejecutadas al salir de un estado.

```yaml
on_exit:
  - disable_pwm
```

### `do`

Acciones ejecutadas periódicamente mientras la FSM permanece en el estado.

```yaml
do:
  - update_control_loop
  - monitor_temperature
```

Recomendación: `do` debería usarse con cuidado. En sistemas embebidos suele corresponder a una función llamada periódicamente desde un tick o scheduler.

---

## 11. `timeout` de estado

Un estado puede tener un timeout.

```yaml
timeout:
  duration_ms: 5000
  event: TIMEOUT
  description: Si la precarga tarda más de 5 s, se genera TIMEOUT.
```

Campos posibles:

| Campo | Obligatorio | Descripción |
|---|---:|---|
| `duration_ms` | Sí | Duración en milisegundos |
| `event` | Sí | Evento generado al cumplirse el timeout |
| `description` | No | Descripción humana |

El generador puede implementar esto como:

- Temporizador interno de estado.
- Evento sintético generado por el scheduler.
- Comparación de timestamps.

---

## 12. `events`

Los eventos son entradas discretas que la FSM procesa.

Formato simple:

```yaml
events:
  - START
  - STOP
  - ADC_UPDATE
  - TIMEOUT
  - RESET
```

Formato recomendado:

```yaml
events:
  - name: START
    source: user_command
    description: Solicitud externa para iniciar la carga.

  - name: ADC_UPDATE
    source: periodic_task
    payload:
      - name: battery_voltage
        type: float
      - name: charge_current
        type: float
    description: Nueva muestra de ADC disponible.
```

Campos posibles:

| Campo | Obligatorio | Descripción |
|---|---:|---|
| `name` | Sí | Nombre del evento |
| `source` | No | Origen lógico del evento |
| `description` | No | Descripción humana |
| `payload` | No | Datos asociados al evento |
| `priority` | No | Prioridad del evento |

---

## 13. `guards`

Los guards son condiciones booleanas que habilitan o bloquean transiciones.

```yaml
guards:
  - name: battery_connected
    expression: battery_voltage > 50.0
    description: La batería está conectada si su tensión supera 50 V.
```

Campos posibles:

| Campo | Obligatorio | Descripción |
|---|---:|---|
| `name` | Sí | Nombre del guard |
| `expression` | No | Expresión lógica abstracta |
| `function` | No | Nombre de función que lo implementa |
| `description` | No | Descripción humana |
| `dependencies` | No | Variables de contexto usadas |

Hay dos estilos válidos.

### 13.1 Guard como expresión

```yaml
- name: current_too_high
  expression: charge_current > max_charge_current
```

El generador puede convertir esto a código si el lenguaje y las variables están bien definidas.

### 13.2 Guard como función externa

```yaml
- name: current_too_high
  function: guard_current_too_high
```

El generador solo declara la función, pero no la implementa.

Ejemplo en C:

```c
bool guard_current_too_high(const ChargerFSM_Context *ctx);
```

---

## 14. `actions`

Las acciones son operaciones ejecutadas por la FSM.

```yaml
actions:
  - name: enable_pwm
    function: action_enable_pwm
    description: Habilita la etapa de potencia.
```

Campos posibles:

| Campo | Obligatorio | Descripción |
|---|---:|---|
| `name` | Sí | Nombre lógico de la acción |
| `function` | No | Nombre de función destino |
| `description` | No | Descripción humana |
| `parameters` | No | Parámetros |
| `modifies` | No | Variables de contexto modificadas |
| `blocking` | No | Indica si la acción bloquea |

Ejemplo con parámetros:

```yaml
- name: set_fault_reason
  function: action_set_fault_reason
  parameters:
    - name: reason
      type: FaultReason
```

Uso en una transición:

```yaml
actions:
  - name: set_fault_reason
    args:
      reason: OVERCURRENT
```

---

## 15. `transitions`

Las transiciones definen el comportamiento principal.

```yaml
transitions:
  - from: IDLE
    event: START
    guard: battery_connected && no_fault
    to: PRECHARGE
    actions:
      - reset_timer
      - enable_precharge
```

Campos posibles:

| Campo | Obligatorio | Descripción |
|---|---:|---|
| `from` | Sí | Estado origen |
| `event` | Sí | Evento que dispara la transición |
| `guard` | No | Condición necesaria |
| `to` | Sí | Estado destino |
| `actions` | No | Acciones asociadas a la transición |
| `priority` | No | Prioridad explícita |
| `description` | No | Descripción humana |

---

## 16. Orden y prioridad de transiciones

Si varias transiciones tienen el mismo `from` y el mismo `event`, puede haber ambigüedad.

Ejemplo:

```yaml
- from: CHARGING
  event: ADC_UPDATE
  guard: current_too_high
  to: FAULT

- from: CHARGING
  event: ADC_UPDATE
  guard: voltage_reached
  to: DONE
```

Si ambas condiciones pueden ser verdaderas a la vez, el resultado depende del orden o de una prioridad explícita.

Forma recomendada:

```yaml
- from: CHARGING
  event: ADC_UPDATE
  guard: current_too_high
  to: FAULT
  priority: 10

- from: CHARGING
  event: ADC_UPDATE
  guard: voltage_reached
  to: DONE
  priority: 20
```

Regla recomendada:

- Menor valor de `priority` implica mayor prioridad.
- Si no hay `priority`, se respeta el orden de aparición.
- Un validador debería avisar si hay condiciones potencialmente solapadas.

---

## 17. Transiciones globales

Una transición global aplica desde cualquier estado.

```yaml
- from: "*"
  event: EMERGENCY_STOP
  to: FAULT
  actions:
    - disable_pwm
    - open_contactor
    - set_fault_emergency_stop
  priority: 0
```

Uso típico:

- Parada de emergencia.
- Fallo crítico.
- Reset global.
- Pérdida de comunicación.

---

## 18. Transiciones internas

Una transición interna procesa un evento sin cambiar de estado.

```yaml
- from: CHARGING
  event: ADC_UPDATE
  guard: normal_operating_conditions
  to: CHARGING
  internal: true
  actions:
    - update_control_loop
```

También puede representarse dentro del estado:

```yaml
states:
  - name: CHARGING
    internal_transitions:
      - event: ADC_UPDATE
        guard: normal_operating_conditions
        actions:
          - update_control_loop
```

Recomendación:

- Usar `internal: true` si quieres que no se ejecuten `on_exit` ni `on_entry`.
- Usar `to: CHARGING` sin `internal: true` si quieres que sí se ejecuten salida y entrada del mismo estado.

---

## 19. Transiciones sin guard

Una transición sin guard se considera siempre habilitada.

```yaml
- from: IDLE
  event: START
  to: RUNNING
```

Equivale a:

```yaml
guard: true
```

---

## 20. Acciones parametrizadas en transiciones

Las acciones pueden llamarse directamente por nombre:

```yaml
actions:
  - disable_pwm
```

O con argumentos:

```yaml
actions:
  - name: set_fault_reason
    args:
      reason: OVERTEMPERATURE
  - name: log_event
    args:
      event_id: CHARGER_FAULT
```

Esto permite generar llamadas como:

```c
action_set_fault_reason(ctx, FAULT_OVERTEMPERATURE);
action_log_event(ctx, CHARGER_FAULT);
```

---

## 21. Estados finales

Un estado final representa una condición terminal.

```yaml
- name: DONE
  type: final
  description: Proceso completado correctamente.
```

Un generador puede usar esto para:

- No esperar más eventos.
- Generar una función `is_finished()`.
- Marcar el estado como terminal en un diagrama.

---

## 22. Estados de fallo

Un estado de fallo representa una condición anómala.

```yaml
- name: FAULT
  type: fault
  on_entry:
    - disable_pwm
    - open_contactor
    - log_fault
```

No todos los fallos tienen que ir al mismo estado, pero suele ser útil tener un estado `FAULT` común y guardar la causa del fallo en el contexto.

---

## 23. Historial de estado

Algunas máquinas jerárquicas necesitan recordar el último subestado activo.

Ejemplo:

```yaml
history:
  enabled: true
  type: shallow
```

Tipos posibles:

| Tipo | Significado |
|---|---|
| `shallow` | Recuerda solo el subestado directo |
| `deep` | Recuerda toda la jerarquía de subestados |

Este campo es opcional y solo tiene sentido en máquinas jerárquicas.

---

## 24. Estados jerárquicos

Para máquinas complejas se pueden usar estados con subestados.

```yaml
states:
  - name: ACTIVE
    initial_state: PRECHARGE
    substates:
      - name: PRECHARGE
      - name: CHARGING
      - name: BALANCING
```

Esto representa una máquina jerárquica tipo statechart.

Recomendación:

- Usar FSM plana para sistemas pequeños.
- Usar jerarquía cuando haya estados que compartan transiciones, acciones o comportamiento común.

---

## 25. Eventos diferidos

Un estado puede diferir eventos para procesarlos más tarde.

```yaml
states:
  - name: PRECHARGE
    defer:
      - START_BALANCING
```

Esto significa que, si llega `START_BALANCING` durante `PRECHARGE`, no se descarta, sino que se guarda hasta que la FSM entre en un estado donde pueda procesarlo.

---

## 26. Eventos ignorados explícitamente

Puedes declarar eventos ignorados para dejar claro que no es un olvido.

```yaml
states:
  - name: IDLE
    ignore:
      - ADC_UPDATE
      - TIMEOUT
```

Esto ayuda a diferenciar entre:

- Evento no contemplado por error.
- Evento ignorado intencionadamente.

---

## 27. Política para eventos no manejados

Puede definirse a nivel global.

```yaml
unhandled_event_policy:
  behavior: ignore
  action: log_unhandled_event
```

Opciones típicas:

| Valor | Significado |
|---|---|
| `ignore` | Ignorar el evento |
| `log` | Registrar el evento |
| `fault` | Ir a estado de fallo |
| `assert` | Disparar una aserción en debug |

---

## 28. Invariantes de estado

Un invariante es una condición que debe cumplirse mientras la FSM esté en un estado.

```yaml
states:
  - name: CHARGING
    invariants:
      - pwm_enabled == true
      - contactor_closed == true
```

Si un invariante falla, el sistema puede:

- Registrar error.
- Ir a `FAULT`.
- Activar una aserción en debug.

---

## 29. Validaciones recomendadas

Un validador de especificación debería comprobar:

1. `initial_state` existe en `states`.
2. Todos los `from` existen, salvo `"*"`.
3. Todos los `to` existen.
4. Todos los `event` existen.
5. Todos los guards usados existen o son expresiones válidas.
6. Todas las acciones usadas existen.
7. No hay estados inalcanzables, salvo que estén marcados como intencionados.
8. No hay transiciones ambiguas sin prioridad.
9. No hay estados sin salida salvo estados `final` o `fault`.
10. No hay eventos definidos pero nunca usados, salvo que estén marcados como reservados.

---

## 30. Generación de código

Un generador puede producir:

- Enumeraciones de estados.
- Enumeraciones de eventos.
- Estructura de contexto.
- Función `init`.
- Función `dispatch`.
- Función `tick` para timeouts y acciones `do`.
- Prototipos de guards.
- Prototipos de acciones.
- Tabla de transiciones.
- Tests unitarios.

Ejemplo de API generada en C:

```c
typedef enum {
    CHARGER_STATE_IDLE,
    CHARGER_STATE_PRECHARGE,
    CHARGER_STATE_CHARGING,
    CHARGER_STATE_DONE,
    CHARGER_STATE_FAULT
} ChargerFSM_State;

typedef enum {
    CHARGER_EVENT_START,
    CHARGER_EVENT_STOP,
    CHARGER_EVENT_ADC_UPDATE,
    CHARGER_EVENT_TIMEOUT,
    CHARGER_EVENT_RESET
} ChargerFSM_Event;

void ChargerFSM_init(ChargerFSM_Context *ctx);
void ChargerFSM_dispatch(ChargerFSM_Context *ctx, ChargerFSM_Event event);
void ChargerFSM_tick(ChargerFSM_Context *ctx, uint32_t elapsed_ms);
ChargerFSM_State ChargerFSM_get_state(const ChargerFSM_Context *ctx);
```

---

## 31. Reglas recomendadas para IA o agentes de código

Cuando se pase esta especificación a una IA, conviene añadir instrucciones como:

```text
Generate production-quality C code from this FSM specification.

Rules:
- Do not invent states, events, guards or actions.
- Preserve transition priority.
- Use the order listed when priority is not specified.
- Generate external prototypes for guards and actions.
- Do not implement hardware-specific functions unless explicitly defined.
- Generate unit-testable code.
- If the FSM specification is ambiguous, mark the affected part as TODO instead of inventing behavior.
```

---

## 32. Plantilla mínima

```yaml
schema_version: "1.0"
machine: ExampleFSM
initial_state: IDLE

states:
  - name: IDLE
  - name: RUNNING
  - name: FAULT
    type: fault

events:
  - name: START
  - name: STOP
  - name: ERROR
  - name: RESET

guards:
  - name: no_fault
    expression: fault_flags == 0

actions:
  - name: start_system
  - name: stop_system
  - name: clear_faults

transitions:
  - from: IDLE
    event: START
    guard: no_fault
    to: RUNNING
    actions:
      - start_system

  - from: RUNNING
    event: STOP
    to: IDLE
    actions:
      - stop_system

  - from: "*"
    event: ERROR
    to: FAULT
    actions:
      - stop_system

  - from: FAULT
    event: RESET
    to: IDLE
    actions:
      - clear_faults
```

---

## 33. Plantilla completa recomendada

```yaml
schema_version: "1.0"
machine: NameFSM
description: Descripción general de la máquina.

initial_state: IDLE

unhandled_event_policy:
  behavior: log
  action: log_unhandled_event

context:
  variables:
    - name: variable_name
      type: uint32_t
      access: read_write
      description: Descripción.

states:
  - name: IDLE
    type: normal
    description: Estado inicial.
    on_entry: []
    on_exit: []
    do: []
    ignore: []
    defer: []
    invariants: []

  - name: FAULT
    type: fault
    on_entry:
      - safe_shutdown

events:
  - name: START
    source: external
    description: Evento de arranque.

guards:
  - name: can_start
    expression: fault_flags == 0
    dependencies:
      - fault_flags

actions:
  - name: safe_shutdown
    function: action_safe_shutdown
    description: Lleva el sistema a estado seguro.

transitions:
  - from: IDLE
    event: START
    guard: can_start
    to: RUNNING
    actions:
      - initialize_system
    priority: 10
    description: Arranca el sistema si no hay fallos.
```

---

## 34. Recomendaciones de estilo

- Nombres de estados en `UPPER_SNAKE_CASE`.
- Nombres de eventos en `UPPER_SNAKE_CASE`.
- Nombres de guards en `lower_snake_case`.
- Nombres de acciones en `lower_snake_case`.
- Evitar lógica compleja dentro de `transitions`.
- Dar prioridad explícita a transiciones críticas de seguridad.
- Separar la FSM del hardware.
- No esconder comportamiento en descripciones en lenguaje natural.

---

## 35. Regla práctica

Una buena especificación de FSM debería poder responder claramente a estas preguntas:

1. ¿En qué estado empieza?
2. ¿Qué eventos acepta?
3. ¿Qué puede pasar desde cada estado?
4. ¿Qué condiciones bloquean o permiten cada transición?
5. ¿Qué acciones se ejecutan al entrar, salir o cambiar de estado?
6. ¿Qué pasa ante eventos no esperados?
7. ¿Qué pasa ante errores o timeouts?
8. ¿Hay transiciones críticas con prioridad clara?

Si alguna de estas preguntas no se puede responder leyendo el YAML, la máquina todavía no está suficientemente formalizada.
