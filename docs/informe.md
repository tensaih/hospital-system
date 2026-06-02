# Informe: Simulación de un Sistema de Gestión de Urgencias Hospitalarias

**Autores:** Iván Maldonado Rodríguez, Carlos  
**Docente:** Francisco Philips Iglesias Vásquez  
**Fecha:** 02 de junio de 2026  

---

## 1. Introducción

Los servicios de urgencias hospitalarias enfrentan constantemente el desafío de gestionar múltiples pacientes con diferentes niveles de criticidad, recursos médicos limitados y condiciones de salud que evolucionan dinámicamente mientras los pacientes esperan atención. Este escenario demanda un sistema capaz de priorizar, reasignar y atender pacientes de manera eficiente en tiempo real. El presente trabajo aborda dicho problema mediante el desarrollo de una simulación computacional en lenguaje C que modela el flujo completo de pacientes dentro de un servicio de urgencias. Para ello, se utilizan exclusivamente estructuras de datos dinámicas —colas, listas enlazadas, listas doblemente enlazadas y pilas— que permiten representar fielmente la naturaleza variable del entorno hospitalario. La simulación opera bajo un modelo de tiempo discreto (ticks), donde en cada unidad de tiempo se procesan llegadas, re-evaluaciones de triage, asignaciones médicas y altas, ofreciendo una visión integral y coherente del sistema bajo distintas configuraciones de carga y recursos.

---

## 2. Modelado del Problema

### 2.1. Estructuras de Datos Utilizadas

El sistema se modela mediante cuatro estructuras dinámicas fundamentales, cada una con una justificación concreta dentro del dominio del problema:

| Estructura | Tipo | Justificación |
|:---|:---|:---|
| **Colas de espera** (`espera_t`) | Cola FIFO enlazada | Representan la fila de espera de cada nivel de prioridad. El comportamiento FIFO garantiza que los pacientes sean atendidos en orden de llegada dentro de su misma prioridad, respetando la equidad temporal. |
| **Lista enlazada de colas** (`lista_colas_t`) | Lista simplemente enlazada | Agrupa dinámicamente las 4 colas de prioridad (1 a 4) en una estructura recorrible. Permite iterar sobre todas las colas para evaluar triage, asignar médicos y generar snapshots sin depender de arreglos estáticos. |
| **Lista enlazada de médicos** (`lista_docs_t`) | Lista simplemente enlazada | Modela el pool de médicos disponibles. Cada nodo contiene el estado del médico (libre/ocupado) y un puntero al paciente en atención, permitiendo la asignación y liberación dinámica. |
| **Pila de rollback** (`pila_t`) | Pila LIFO enlazada | Registra cada cambio de triage realizado, almacenando el ID del paciente y su prioridad anterior. Permite revertir los últimos cambios de prioridad de forma ordenada (último en entrar, primero en salir), simulando un mecanismo de «deshacer» coherente. |
| **Historial de atención** (`historial_t`) | Lista doblemente enlazada | Almacena el registro de todos los pacientes dados de alta. La doble enlace permite recorrer el historial tanto en orden cronológico (más antiguo primero) como en orden inverso (más reciente primero) para generar reportes y métricas. |

### 2.2. Funciones Clave

Las funciones implementadas se agrupan por componente:

**Gestión de colas:**
- `crearColaEspera()`: Inicializa una cola vacía con punteros `front` y `final` a `NULL`.
- `encolar()`: Inserta un paciente al final de la cola (operación O(1)).
- `desencolar()`: Extrae el paciente del frente de la cola (operación O(1)).
- `extraer_paciente()`: Busca y extrae un paciente específico por ID desde cualquier posición de la cola (operación O(n)), indispensable para el re-triage y rollback.

**Gestión de prioridades:**
- `crearListaColas()`: Crea dinámicamente 4 colas, una por cada nivel de prioridad.
- `obtener_cola_prioridad()`: Retorna la cola correspondiente a un nivel de prioridad dado.
- `todas_colas_vacias()`: Verifica si todas las colas de prioridad están vacías (condición de finalización).

**Pila de rollback:**
- `push()`: Apila un registro de cambio (ID del paciente + prioridad anterior).
- `pop()`: Desapila y retorna el registro más reciente para ejecutar la reversión.

**Historial (lista doblemente enlazada):**
- `insertar_historial()`: Inserta al final de la lista doble, manteniendo los punteros `prev` y `next`.
- `imprimir_historial_inverso()`: Recorre desde la cola hacia la cabeza, mostrando los pacientes más recientes primero.
- `imprimir_metricas()`: Recorre toda la lista para calcular estadísticas por prioridad.

**Carga de datos:**
- `cargar_pacientes()`: Lee el archivo de entrada, parsea cada línea con `fscanf` y encola los pacientes en la cola de llegada. Incluye validación de formato con manejo de errores.

### 2.3. Interacción entre Componentes

El siguiente diagrama ilustra el flujo de datos entre las estructuras:

```
 ┌────────────────┐     Tick ≤ llegada     ┌──────────────────────┐
 │   Archivo de   │ ──────────────────────►│  Cola de llegada     │
 │   entrada      │    cargar_pacientes()  │  (espera_t)          │
 └────────────────┘                        └──────────┬───────────┘
                                                      │ desencolar()
                                                      ▼
                                           ┌──────────────────────┐
                                           │  Lista de colas por  │
                                           │  prioridad (1 a 4)   │
                                           │  (lista_colas_t)     │
                                           └────┬──────┬──────────┘
                                   re-triage    │      │  asignar médico
                                   (extraer +   │      │  (desencolar)
                                    encolar)    │      │
                                                │      ▼
                              ┌─────────────────┤  ┌──────────────────┐
                              │  Pila rollback  │  │  Lista de médicos│
                              │  (pila_t)       │  │  (lista_docs_t)  │
                              │  push(cambio)   │  └──────┬───────────┘
                              └────────┬────────┘         │ tiempo_restante = 0
                                       │                  ▼
                               pop() cada 20 ticks  ┌──────────────────┐
                               (revierte prioridad) │  Historial       │
                                                    │  (historial_t)   │
                                                    │  Lista doble     │
                                                    └──────────────────┘
```

En cada tick del bucle principal, la simulación ejecuta las siguientes fases en orden:

1. **Llegada**: Se desencolan pacientes cuyo `tiempo_llegada ≤ tick` y se insertan en la cola de prioridad correspondiente.
2. **Re-triage**: Se evalúa probabilísticamente si cada paciente en espera mejora o empeora, moviendo al paciente entre colas y registrando el cambio en la pila.
3. **Rollback periódico**: Cada 20 ticks, se revierten los últimos 2 cambios de triage usando la pila.
4. **Finalización de atenciones**: Se decrementa el tiempo restante de cada médico ocupado; si llega a 0, el paciente se inserta en el historial.
5. **Asignación de médicos**: Los médicos libres toman al primer paciente de la cola con mayor prioridad disponible.
6. **Actualización de tiempos y alertas**: Se incrementa el tiempo de espera de cada paciente en cola y se emiten alertas por espera excesiva o acumulación.

---

## 3. Implementación del Sistema

### 3.1. Uso Correcto de Estructuras Dinámicas

Todas las estructuras de datos del sistema están implementadas mediante **nodos enlazados con asignación dinámica de memoria** (`malloc`/`free`). No se utiliza ningún arreglo como sustituto de las estructuras dinámicas requeridas:

- Las **colas** se implementan como listas simplemente enlazadas con punteros `front` y `final`.
- La **lista de colas por prioridad** es una lista enlazada de nodos, cada uno conteniendo una cola independiente.
- La **lista de médicos** es una lista enlazada de nodos `nodo_doc_t`.
- La **pila** se implementa como una lista enlazada con inserción y extracción por el tope.
- El **historial** es una lista doblemente enlazada con punteros `prev` y `next` en cada nodo.

### 3.2. Manejo Adecuado de Memoria

El programa asegura la liberación completa de toda la memoria asignada al finalizar la simulación, mediante funciones dedicadas:

```c
liberar_cola(pacientes_llegada);
liberar_lista_colas(gestor_colas);
liberar_lista_medicos(gestor_medicos);
liberar_pila(pila_rollback);
liberar_historial(registro_historico);
```

Cada función `liberar_*` recorre la estructura completa, liberando primero los datos contenidos (pacientes) y luego los nodos contenedores, evitando fugas de memoria. Además, en caso de error durante la lectura del archivo, se liberan todas las estructuras ya inicializadas antes de retornar.

### 3.3. Inserción y Eliminación en Cualquier Posición

La función `extraer_paciente()` permite extraer un paciente de **cualquier posición** dentro de una cola (frente, medio o final), actualizando correctamente los punteros `front`, `final` y los enlaces intermedios. Esta capacidad es esencial para:

- **Re-triage**: Mover un paciente de una cola de prioridad a otra.
- **Rollback**: Buscar y extraer un paciente de cualquier cola para restaurar su prioridad anterior.

```c
patient_t* extraer_paciente(espera_t *cola, int id) {
    cola_t *actual = cola->front;
    cola_t *anterior = NULL;
    while (actual != NULL && actual->paciente->id != id) {
        anterior = actual;
        actual = actual->next;
    }
    if (actual == NULL) return NULL;
    // Manejo de casos: frente, medio y final
    if (anterior == NULL) {
        cola->front = actual->next;
        if (cola->front == NULL) cola->final = NULL;
    } else {
        anterior->next = actual->next;
        if (actual->next == NULL) cola->final = anterior;
    }
    free(actual);
    return p;
}
```

### 3.4. Simulación en Tiempo Discreto

El sistema opera mediante un bucle `while(hospital_activo)` que avanza un tick por iteración. En cada tick se procesan secuencialmente todas las fases del sistema (llegada → triage → rollback → finalización → asignación → alertas), garantizando coherencia temporal. La simulación finaliza cuando:

- No quedan pacientes por llegar (`pacientes_llegada` vacía).
- Todas las colas de prioridad están vacías.
- Ningún médico tiene paciente en atención.

### 3.5. Lectura desde Archivo

El programa recibe el nombre del archivo como argumento de línea de comandos (`argv[1]`). La función `cargar_pacientes()` abre el archivo, lee la cantidad de médicos y pacientes, y parsea cada línea con el formato:

```
P[ID];[edad];[prioridad];[tiempo_llegada];[tiempo_atencion];[prob_empeorar];[prob_mejorar]
```

Se valida que cada lectura retorne exactamente 7 campos; en caso contrario, se reporta el error indicando la línea problemática y se libera toda la memoria asignada.

### 3.6. Pila Funcional para Revertir Cambios Reales

La pila de rollback no es decorativa: cada vez que un paciente cambia de prioridad en el triage, se registra mediante `push(pila_rollback, id, prioridad_anterior)`. Cada 20 ticks, el sistema ejecuta hasta 2 operaciones `pop()` y:

1. **Busca al paciente** en todas las colas de prioridad.
2. Si lo encuentra, lo **extrae** de su cola actual y lo **reinserta** en la cola correspondiente a su prioridad anterior, efectuando un rollback real.
3. Si el paciente ya fue atendido o está en consulta, se reporta como rollback omitido, demostrando el manejo correcto de casos borde.

---

## 4. Pruebas de Ejecución

### 4.1. Prueba 1 — Archivo `ejemploEntrada.txt` (10 pacientes, 3 médicos)

**Configuración de entrada:**

| Parámetro | Valor |
|:---|:---|
| Médicos | 3 |
| Pacientes | 10 |
| Prioridades | 1 (crítica) a 4 (baja) |
| Llegadas | Escalonadas del tick 0 al tick 8 |

**Extracto de la ejecución:**

```
::: Iniciando el sistema con 3 medicos :::

[Tick 000] LLEGO paciente P001 (Prio 1).
[Tick 000] El medico 1 comenzo la atencion del paciente P001 (Prioridad 1).
[Tick 001] LLEGO paciente P002 (Prio 3).
[Tick 001] El medico 2 comenzo la atencion del paciente P002 (Prioridad 3).
[Tick 002] LLEGO paciente P003 (Prio 4).
[Tick 002] LLEGO paciente P004 (Prio 2).
[Tick 002] TRIAGE: P004 cambia de Prio 2 a 1
[Tick 002] El medico 3 comenzo la atencion del paciente P004 (Prioridad 1).
[Tick 005] LLEGO paciente P007 (Prio 2).
[Tick 005] TRIAGE: P006 cambia de Prio 3 a 2
[Tick 007] TRIAGE: P007 cambia de Prio 2 a 1
[Tick 007] TRIAGE: P006 cambia de Prio 2 a 1
[Tick 007] TRIAGE: P009 cambia de Prio 2 a 1

==== [SNAPSHOT TICK 010] ====
->  Medico 1: Atendiendo a P007 (Restante: 7)
->  Medico 2: Atendiendo a P005 (Restante: 6)
->  Medico 3: Atendiendo a P006 (Restante: 3)
->  Fila Prio 1: 2 en espera
->  Fila Prio 2: 0 en espera
->  Fila Prio 3: 1 en espera
->  Fila Prio 4: 1 en espera
=============================

[Tick 012] ALERTA: P003 ha cambiado de prioridad 3 veces!
[Tick 013] ALERTA: P003 ha cambiado de prioridad 4 veces!

--- ROLLBACK PERIODICO: Revirtiendo ultimos 2 cambios (Tick 20) ---
[Tick 020] ROLLBACK OMITIDO: P003 ya se encuentra en atencion o finalizado.
[Tick 020] ROLLBACK OMITIDO: P003 ya se encuentra en atencion o finalizado.
----------------------------------------------------------

::: SIMULACION FINALIZADA EN EL TICK 22 :::
```

**Reporte de métricas:**

| Prioridad | Atendidos | Espera Promedio | Espera Máxima |
|:---:|:---:|:---:|:---:|
| 1 | 2 | 1.50 ticks | 3 ticks |
| 2 | 3 | 3.00 ticks | 6 ticks |
| 3 | 3 | 4.33 ticks | 8 ticks |
| 4 | 2 | 14.00 ticks | 17 ticks |

**Pacientes críticos (Prio 1) atendidos en ≤ 15 ticks:** 100.0%

**Casos borde observados:**
- **Re-triage múltiple:** El paciente P003 cambió de prioridad 4 veces (Prio 4 → 3 → 2 → 3 → 4), activando la alerta por cambios excesivos.
- **Múltiples eventos simultáneos:** En el tick 2 llegaron P003 y P004 simultáneamente; P004 fue re-triageado y atendido en el mismo tick.
- **Rollback omitido:** En el tick 20, el paciente objetivo del rollback ya estaba en atención, demostrando el manejo correcto del caso borde.

---

### 4.2. Prueba 2 — Archivo `test_bordes.txt` (5 pacientes, 2 médicos, todos llegan en tick 0)

**Configuración de entrada:**

| Parámetro | Valor |
|:---|:---|
| Médicos | 2 |
| Pacientes | 5 |
| Llegadas | **Todos llegan en tick 0** (caso borde de simultaneidad máxima) |
| Prioridades | 2 pacientes críticos (Prio 1), 1 urgente (Prio 2), 1 moderado (Prio 3), 1 bajo (Prio 4) |

**Ejecución completa:**

```
::: Iniciando el sistema con 2 medicos :::

[Tick 000] LLEGO paciente P001 (Prio 1).
[Tick 000] LLEGO paciente P002 (Prio 1).
[Tick 000] LLEGO paciente P003 (Prio 4).
[Tick 000] LLEGO paciente P004 (Prio 2).
[Tick 000] LLEGO paciente P005 (Prio 3).
[Tick 000] El medico 1 comenzo la atencion del paciente P001 (Prioridad 1).
[Tick 000] El medico 2 comenzo la atencion del paciente P002 (Prioridad 1).
[Tick 003] TRIAGE: P004 cambia de Prio 2 a 1
[Tick 003] Medico 1 TERMINO atencion de P001.
[Tick 003] El medico 1 comenzo la atencion del paciente P004 (Prioridad 1).
[Tick 004] Medico 2 TERMINO atencion de P002.
[Tick 004] El medico 2 comenzo la atencion del paciente P005 (Prioridad 3).
[Tick 008] Medico 1 TERMINO atencion de P004.
[Tick 008] El medico 1 comenzo la atencion del paciente P003 (Prioridad 4).
[Tick 010] Medico 1 TERMINO atencion de P003.
[Tick 010] Medico 2 TERMINO atencion de P005.

==== [SNAPSHOT TICK 010] ====
->  Medico 1: Libre
->  Medico 2: Libre
->  Fila Prio 1: 0 en espera
->  Fila Prio 2: 0 en espera
->  Fila Prio 3: 0 en espera
->  Fila Prio 4: 0 en espera
=============================

::: SIMULACION FINALIZADA EN EL TICK 10 :::
```

**Reporte de métricas:**

| Prioridad | Atendidos | Espera Promedio | Espera Máxima |
|:---:|:---:|:---:|:---:|
| 1 | 2 | 0.00 ticks | 0 ticks |
| 2 | 1 | 3.00 ticks | 3 ticks |
| 3 | 1 | 4.00 ticks | 4 ticks |
| 4 | 1 | 8.00 ticks | 8 ticks |

**Pacientes críticos (Prio 1) atendidos en ≤ 15 ticks:** 100.0%

**Historial de atención (más recientes primero):**
```
Alta en Tick 010 -> P005 (Prio Orig: 3 | Prio Final: 3 | Espera: 4 ticks)
Alta en Tick 010 -> P003 (Prio Orig: 4 | Prio Final: 4 | Espera: 8 ticks)
Alta en Tick 008 -> P004 (Prio Orig: 2 | Prio Final: 1 | Espera: 3 ticks)
Alta en Tick 004 -> P002 (Prio Orig: 1 | Prio Final: 1 | Espera: 0 ticks)
Alta en Tick 003 -> P001 (Prio Orig: 1 | Prio Final: 1 | Espera: 0 ticks)
```

**Casos borde observados:**
- **Todos los pacientes llegan simultáneamente (tick 0):** El sistema procesa correctamente 5 llegadas en un solo tick, asignando los 2 médicos a los pacientes de mayor prioridad y encolando el resto.
- **Colas vacías:** Al finalizar (tick 10), todas las colas están vacías y ambos médicos están libres, confirmando la condición de terminación del sistema.
- **Re-triage efectivo:** P004 (Prio 2) empeoró a Prio 1 en el tick 3 y fue atendido inmediatamente al liberarse un médico, validando que el triage prioriza correctamente.
- **Paciente de baja prioridad (Prio 4):** P003 fue el último en ser atendido, esperando 8 ticks, lo cual es coherente con la política de priorización.

---

### 4.3. Prueba 3 — Archivo `pacientes.txt` (30 pacientes, 1 médico — estrés máximo)

**Configuración de entrada:**

| Parámetro | Valor |
|:---|:---|
| Médicos | **1** (recurso mínimo) |
| Pacientes | 30 |
| Llegadas | Escalonadas del tick 0 al tick 62 |

**Extracto de la ejecución (fases relevantes):**

```
::: Iniciando el sistema con 1 medicos :::

[Tick 000] LLEGO paciente P001 (Prio 4).
[Tick 000] LLEGO paciente P002 (Prio 1).
[Tick 000] El medico 1 comenzo la atencion del paciente P002 (Prioridad 1).
...
[Tick 020] ALERTA: El paciente P008 lleva 20 ticks esperando en la cola de prioridad 3!

--- ROLLBACK PERIODICO: Revirtiendo ultimos 2 cambios (Tick 20) ---
[Tick 020] ROLLBACK EXITOSO: P008 restaurado a Prio 3
[Tick 020] ROLLBACK EXITOSO: P004 restaurado a Prio 4
----------------------------------------------------------
...

==== [SNAPSHOT TICK 210] ====
->  Medico 1: Atendiendo a P022 (Restante: 4)
->  Fila Prio 1: 0 en espera
->  Fila Prio 2: 0 en espera
->  Fila Prio 3: 2 en espera
->  Fila Prio 4: 2 en espera
=============================
...
--- ROLLBACK PERIODICO: Revirtiendo ultimos 2 cambios (Tick 220) ---
[Tick 220] ROLLBACK EXITOSO: P030 restaurado a Prio 4
[Tick 220] ROLLBACK EXITOSO: P030 restaurado a Prio 3
----------------------------------------------------------
...
::: SIMULACION FINALIZADA EN EL TICK 247 :::
```

**Reporte de métricas:**

| Prioridad | Atendidos | Espera Promedio | Espera Máxima |
|:---:|:---:|:---:|:---:|
| 1 | 6 | 80.50 ticks | 175 ticks |
| 2 | 7 | 84.86 ticks | 166 ticks |
| 3 | 10 | 111.00 ticks | 215 ticks |
| 4 | 7 | 55.14 ticks | 106 ticks |

**Pacientes críticos (Prio 1) atendidos en ≤ 15 ticks:** 16.7%

**Casos borde observados:**
- **Re-triage masivo:** El paciente P030 acumuló **72 cambios de prioridad** mientras esperaba atención, demostrando la robustez de la extracción/inserción dinámica.
- **Rollback exitoso real:** En el tick 220, se ejecutaron 2 rollbacks exitosos consecutivos sobre P030, moviéndolo primero de Prio 3 a Prio 4 y luego de Prio 4 a Prio 3, comprobando la funcionalidad real de la pila.
- **Estrés con 1 médico:** Con un solo médico atendiendo 30 pacientes, la simulación alcanzó 247 ticks y mostró tiempos de espera de hasta 215 ticks, evidenciando el impacto de la escasez de recursos.
- **Alertas de espera excesiva:** Múltiples pacientes activaron la alerta de 20 ticks de espera, validando el sistema de monitoreo.

---

## 5. Conclusiones

El desarrollo de este sistema de simulación de urgencias hospitalarias permitió aplicar de manera integral los conceptos de estructuras de datos dinámicas estudiados durante el curso, trasladándolos a un escenario con relevancia práctica real. La implementación demuestra que las colas, pilas y listas enlazadas no son abstracciones meramente teóricas, sino herramientas poderosas para modelar sistemas donde los datos fluyen, se transforman y se reorganizan de forma continua. El mecanismo de rollback mediante la pila resultó particularmente valioso para comprender cómo el principio LIFO permite implementar funcionalidades de «deshacer» en sistemas complejos. Asimismo, la lista doblemente enlazada demostró su versatilidad al facilitar recorridos bidireccionales para la generación de reportes. Las pruebas realizadas bajo distintas configuraciones —desde escenarios controlados hasta pruebas de estrés con recursos mínimos— validaron la robustez y coherencia del sistema. Este proyecto refuerza el aprendizaje de que la elección adecuada de la estructura de datos no solo determina la corrección del programa, sino también su capacidad para escalar y adaptarse a condiciones adversas, una competencia fundamental en el desarrollo de software profesional.
