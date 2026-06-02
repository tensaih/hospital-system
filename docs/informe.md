# Informe: Simulación de un Sistema de Gestión de Urgencias Hospitalarias

**Autores:** Iván Maldonado Rodríguez, Carlos  
**Docente:** Francisco Philips Iglesias Vásquez  
**Fecha:** 02 de junio de 2026  
**Archivo fuente:** `src/programav2.c` (679 líneas)

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

## 3. Implementación del Sistema — Análisis Detallado del Código

A continuación se presenta el análisis exhaustivo del código fuente `programav2.c`, organizado por bloques lógicos. Cada bloque indica las **líneas de inicio y fin** del fragmento correspondiente, seguido de la captura del código y su explicación.

---

### 3.1. Encabezado y Bibliotecas

📄 **Líneas 1 – 15**

```c
/*
 * Simulación de un sistema de gestión de urgencias hospitalarias
 * Autores: Iván Maldonado Rodriguez, carlos
 * Fecha: 02-05-2026
 * Docente: Francisco Philips Iglesias Vásquez
 * Descripción: 
 *  Este programa simula la gestión de pacientes en un servicio de urgencias,
 *  haciendo uso de estructuras dinámicas como colas, listas enlazadas y pilas para manejar
 *  la llegada, atención y evolución de los pacientes.
 * */
/* BIBLIOTECAS */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
```

**Explicación:** El bloque inicial contiene el comentario de documentación del proyecto con los datos de autoría y una descripción general del programa. A continuación se incluyen las cuatro bibliotecas estándar necesarias: `stdio.h` para operaciones de entrada/salida (printf, fscanf, fopen), `stdlib.h` para manejo de memoria dinámica (malloc, free) y generación de números aleatorios (rand, srand), `stdbool.h` para utilizar el tipo `bool` con valores `true`/`false`, y `time.h` para obtener la semilla del generador aleatorio mediante `time(NULL)`.

---

### 3.2. Estructura del Paciente (`patient_t`)

📄 **Líneas 17 – 31**

```c
/* DEFINICION DE ESTRUCTURAS */
typedef struct patient {
    int id;
    int edad;
    int prioridad_original;
    int tiempo_llegada;
    int tiempo_atencion;
    float prob_mejora;
    float prob_empeora;
    int prioridad_actual;
    int tiempo_restante;
    int tiempo_espera;
    int cambios_triage;
    int ultimo_tick_triage;
} patient_t;
```

**Explicación:** Esta estructura modela a cada paciente del sistema. Los campos `id`, `edad`, `prioridad_original`, `tiempo_llegada`, `tiempo_atencion`, `prob_mejora` y `prob_empeora` se cargan directamente desde el archivo de entrada. Los campos restantes son dinámicos y se actualizan durante la simulación: `prioridad_actual` refleja la prioridad vigente (que puede cambiar por re-triage), `tiempo_restante` es un contador decremental de la atención médica, `tiempo_espera` acumula los ticks que el paciente lleva en cola, `cambios_triage` cuenta cuántas veces cambió de prioridad (para emitir alertas), y `ultimo_tick_triage` registra el tick del último re-triage para evitar evaluar al mismo paciente dos veces en un mismo tick.

---

### 3.3. Estructura del Médico (`doc_t`)

📄 **Líneas 33 – 37**

```c
typedef struct doc {
    int id_medico;
    bool occu;
    struct patient *curr;
} doc_t;
```

**Explicación:** Representa a un médico dentro del hospital. El campo `id_medico` identifica al médico de forma única, `occu` (booleano) indica si está ocupado atendiendo a un paciente, y `curr` es un puntero al paciente actualmente en consulta. Cuando el médico está libre, `occu` es `false` y `curr` es `NULL`.

---

### 3.4. Estructuras de la Cola de Espera (`cola_t`, `espera_t`)

📄 **Líneas 39 – 48**

```c
/* Colas de espera */
typedef struct cola {
    patient_t *paciente;
    struct cola *next;
} cola_t;

typedef struct espera {
    cola_t *front;
    cola_t *final;
} espera_t;
```

**Explicación:** `cola_t` es el nodo individual de la cola, conteniendo un puntero al paciente y un puntero `next` al siguiente nodo. `espera_t` es la estructura de control de la cola FIFO, con dos punteros: `front` apunta al primer elemento (el próximo a ser atendido) y `final` apunta al último (donde se insertan nuevos pacientes). Esta implementación permite encolar y desencolar en tiempo O(1).

---

### 3.5. Estructuras de la Lista de Médicos (`nodo_doc_t`, `lista_docs_t`)

📄 **Líneas 50 – 57**

```c
typedef struct nodo_doc {
    doc_t medico;
    struct nodo_doc *next;
} nodo_doc_t;

typedef struct lista_docs {
    nodo_doc_t *cabeza;
} lista_docs_t;
```

**Explicación:** Se implementa como una lista simplemente enlazada. Cada `nodo_doc_t` contiene un médico (`doc_t`) embebido directamente (no un puntero) y un enlace al siguiente nodo. `lista_docs_t` mantiene un puntero `cabeza` al primer nodo de la lista. Esta estructura permite agregar o recorrer médicos dinámicamente sin necesidad de arreglos estáticos.

---

### 3.6. Estructuras del Gestor de Colas por Prioridad (`nodo_lista_cola_t`, `lista_colas_t`)

📄 **Líneas 59 – 67**

```c
typedef struct nodo_lista_cola {
    int prioridad_asignada;
    espera_t *cola_espera;
    struct nodo_lista_cola *next;
} nodo_lista_cola_t;

typedef struct lista_colas {
    nodo_lista_cola_t *cabeza;
} lista_colas_t;
```

**Explicación:** Esta es una lista enlazada de colas. Cada nodo `nodo_lista_cola_t` asocia un nivel de `prioridad_asignada` (1 a 4) con su cola de espera correspondiente (`cola_espera`). `lista_colas_t` agrupa todas las colas bajo un único punto de acceso (`cabeza`). Al recorrer esta lista, el sistema puede iterar sobre todas las prioridades para realizar triage, asignar médicos o generar snapshots.

---

### 3.7. Estructuras de la Pila de Rollback (`registro_cambio_t`, `nodo_pila_t`, `pila_t`)

📄 **Líneas 69 – 82**

```c
/* Pila */
typedef struct registro_cambio {
    int id_paciente;
    int prioridad_anterior;
} registro_cambio_t;

typedef struct nodo_pila {
    registro_cambio_t datos;
    struct nodo_pila * siguiente;
} nodo_pila_t;

typedef struct pila {
    nodo_pila_t* tope;
} pila_t;
```

**Explicación:** La pila se compone de tres tipos. `registro_cambio_t` almacena la información necesaria para revertir un cambio de triage: el `id_paciente` afectado y su `prioridad_anterior` antes del cambio. `nodo_pila_t` es el nodo de la pila enlazada, conteniendo los datos del registro y un puntero `siguiente` al nodo inferior. `pila_t` mantiene un puntero `tope` al nodo superior de la pila. La estructura LIFO garantiza que los cambios más recientes se reviertan primero.

---

### 3.8. Estructuras del Historial (`nodo_historial_t`, `historial_t`)

📄 **Líneas 84 – 95**

```c
/* Historial lista doblemente enlazada */
typedef struct nodo_historial {
    patient_t *paciente;
    int tick_salida;
    struct nodo_historial *next;
    struct nodo_historial *prev;
} nodo_historial_t;

typedef struct historial {
    nodo_historial_t *cabeza;
    nodo_historial_t *cola;
} historial_t;
```

**Explicación:** Implementa una lista doblemente enlazada para almacenar el historial de pacientes atendidos. Cada `nodo_historial_t` contiene un puntero al `paciente`, el `tick_salida` (momento del alta), y los punteros `next` (siguiente) y `prev` (anterior) que habilitan recorridos bidireccionales. `historial_t` mantiene punteros a la `cabeza` (primer alta) y a la `cola` (última alta), permitiendo inserción al final en O(1) y recorrido inverso desde la cola.

---

### 3.9. Declaraciones de Funciones (Prototipos)

📄 **Líneas 97 – 127**

```c
/* DECLARACIONES DE FUNCIONES */
espera_t* crearColaEspera();
void encolar(espera_t *cola, patient_t *paciente);
patient_t* desencolar(espera_t *cola);
patient_t* extraer_paciente(espera_t *cola, int id);
bool colaVacia(espera_t* cola);
void liberar_cola(espera_t* cola);

lista_colas_t* crearListaColas();
espera_t* obtener_cola_prioridad(lista_colas_t* lista, int prio);
bool todas_colas_vacias(lista_colas_t* lista);
void liberar_lista_colas(lista_colas_t* lista);

lista_docs_t* crearListaMedicos(int n);
void liberar_lista_medicos(lista_docs_t* lista);

pila_t* crearPila();
void push(pila_t* pila, int id_paciente, int prioridad_anterior);
registro_cambio_t pop(pila_t* pila);
void liberar_pila(pila_t* pila);

historial_t* crearHistorial();
void liberar_historial(historial_t *registro);
void insertar_historial(historial_t *h, patient_t *p, int tick);
void imprimir_metricas(historial_t *h);
void imprimir_historial_inverso(historial_t *h);

int cargar_pacientes(espera_t *pacientes, const char *filename);
void imprimir_pacientes(espera_t *cola);
void imprimir_snapshot(int tick, lista_colas_t* l_colas, lista_docs_t* l_docs);
float generar_random();
```

**Explicación:** Bloque de prototipos que declara todas las funciones del programa antes de `main()`. Están organizados por subsistema: funciones de cola (crear, encolar, desencolar, extraer, verificar vacía, liberar), funciones del gestor de colas por prioridad, funciones de la lista de médicos, funciones de la pila (crear, push, pop, liberar), funciones del historial (crear, liberar, insertar, métricas, impresión inversa), y funciones auxiliares (carga desde archivo, impresión, snapshot, generación de aleatorios). Esta organización permite que `main()` invoque cualquier función sin importar el orden de definición.

---

### 3.10. Función `main()` — Validación de Argumentos e Inicialización

📄 **Líneas 130 – 160**

```c
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf(":: SISTEMA DE GESTION DE URGENCIAS HOSPITALARIAS ::\n");
        printf(":: USO CORRECTO: %s <nombre_archivo.txt>\n", argv[0]);
        return 0;
    }

    srand((unsigned int)time(NULL));

    /* Inicialización de estructuras */
    espera_t* pacientes_llegada = crearColaEspera();
    lista_colas_t* gestor_colas = crearListaColas();
    pila_t* pila_rollback = crearPila();
    historial_t* registro_historico = crearHistorial();
    
    int n_docs = cargar_pacientes(pacientes_llegada, argv[1]);
    if (n_docs == -1) {
        printf("[ERROR]: Hubo un error en la lectura del archivo...\n");
        ...
        liberar_cola(pacientes_llegada);
        liberar_lista_colas(gestor_colas);
        liberar_pila(pila_rollback);
        liberar_historial(registro_historico);
        return 1; 
    }

    lista_docs_t* gestor_medicos = crearListaMedicos(n_docs);

    printf("::: Iniciando el sistema con %d medicos :::\n\n", n_docs);
```

**Explicación:** El programa recibe el nombre del archivo como argumento de línea de comandos (`argv[1]`). Si no se proporciona, muestra un mensaje de uso correcto y termina. A continuación se inicializa la semilla aleatoria con `srand(time(NULL))` para que cada ejecución genere resultados diferentes en el triage. Se crean las cuatro estructuras principales: la cola de llegada, el gestor de colas por prioridad, la pila de rollback y el historial. Luego se invoca `cargar_pacientes()` que lee el archivo y retorna la cantidad de médicos; si retorna `-1`, se imprime un mensaje de error, se liberan todas las estructuras ya creadas y se finaliza con código de error. Si la carga es exitosa, se crea la lista de médicos con `crearListaMedicos(n_docs)`.

---

### 3.11. Bucle Principal — Fase 1: Llegada de Pacientes

📄 **Líneas 162 – 173**

```c
    int tick = 0;
    bool hospital_activo = true;

    while(hospital_activo) {
        
        /* Llegada de nuevos pacientes */
        while(!colaVacia(pacientes_llegada) && pacientes_llegada->front->paciente->tiempo_llegada <= tick) {
            patient_t* p = desencolar(pacientes_llegada);
            espera_t* cola_dest = obtener_cola_prioridad(gestor_colas, p->prioridad_actual);
            encolar(cola_dest, p);
            printf("[Tick %03d] LLEGO paciente P%03d (Prio %d).\n", tick, p->id, p->prioridad_actual);
        }
```

**Explicación:** Se inicializan el contador de ticks en 0 y la bandera `hospital_activo` en `true`. El bucle `while(hospital_activo)` es el corazón de la simulación en tiempo discreto. En la primera fase de cada tick, se revisa si hay pacientes en la cola de llegada cuyo `tiempo_llegada` sea menor o igual al tick actual. Si es así, se desencolan de la cola de llegada y se encolan en la cola de prioridad que les corresponde según su `prioridad_actual`. El `while` interno maneja el caso de múltiples pacientes llegando en el mismo tick (eventos simultáneos).

---

### 3.12. Bucle Principal — Fase 2: Evaluación de Triage (Re-triage)

📄 **Líneas 175 – 218**

```c
        /* Evaluar triage (reubicacion) */
        nodo_lista_cola_t* nodo_c = gestor_colas->cabeza;
        while(nodo_c != NULL) {
            cola_t *actual = nodo_c->cola_espera->front;
            
            while(actual != NULL) {
                patient_t *p = actual->paciente;
                cola_t *siguiente_seguro = actual->next;
                
                /* Ignorar pacientes recien reubicados */
                if (p->ultimo_tick_triage != tick) {
                    float chance = generar_random();
                    int nueva_prio = p->prioridad_actual;
                    
                    if (chance < p->prob_empeora && p->prioridad_actual > 1) {
                        nueva_prio--; 
                    } else if (chance > (1.0f - p->prob_mejora) && p->prioridad_actual < 4) {
                        nueva_prio++; 
                    }
                    
                    if (nueva_prio != p->prioridad_actual) {
                        push(pila_rollback, p->id, p->prioridad_actual);
                        
                        patient_t *extraido = extraer_paciente(nodo_c->cola_espera, p->id);
                        if (extraido) {
                            extraido->cambios_triage++; 
                            extraido->ultimo_tick_triage = tick;
                            printf("[Tick %03d] TRIAGE: P%03d cambia de Prio %d a %d\n", 
                                tick, extraido->id, extraido->prioridad_actual, nueva_prio);
                            
                            extraido->prioridad_actual = nueva_prio;
                            encolar(obtener_cola_prioridad(gestor_colas, nueva_prio), extraido);
                            
                            /* Alerta si el paciente tiene varios retriage */
                            if (extraido->cambios_triage >= 3) {
                                printf("[Tick %03d] ALERTA: P%03d ha cambiado de prioridad %d veces!\n", 
                                    tick, extraido->id, extraido->cambios_triage);
                            }
                        }
                    }
                }
                actual = siguiente_seguro;
            }
            nodo_c = nodo_c->next;
        }
```

**Explicación:** Esta fase recorre todas las colas de prioridad y evalúa cada paciente en espera. Se guarda `siguiente_seguro = actual->next` antes de procesar, porque si el paciente es extraído de la cola, el nodo actual se libera y acceder a `actual->next` causaría un error. El campo `ultimo_tick_triage` evita evaluar dos veces a un paciente recién reubicado en el mismo tick. Para cada paciente, se genera un número aleatorio: si cae por debajo de `prob_empeora`, la prioridad baja (se vuelve más urgente); si cae por encima de `(1 - prob_mejora)`, la prioridad sube (mejora). Si la prioridad cambia, se registra el cambio en la pila con `push()`, se extrae al paciente de su cola actual con `extraer_paciente()`, se actualiza su prioridad y se reinserta en la cola correspondiente. Si acumula 3 o más cambios de triage, se emite una alerta.

---

### 3.13. Bucle Principal — Fase 3: Rollback Periódico de Prioridades

📄 **Líneas 220 – 245**

```c
        /* Rollback periodico de prioridades */
        if (tick > 0 && tick % 20 == 0 && pila_rollback->tope != NULL) {
            printf("\n--- ROLLBACK PERIODICO: Revirtiendo ultimos 2 cambios (Tick %d) ---\n", tick);
            for (int r = 0; r < 2 && pila_rollback->tope != NULL; r++) {
                registro_cambio_t retroceso = pop(pila_rollback);
                patient_t* p_revertir = NULL;
                
                /* Buscar en cada cola */
                nodo_lista_cola_t* busq_c = gestor_colas->cabeza;
                while(busq_c != NULL) {
                    p_revertir = extraer_paciente(busq_c->cola_espera, retroceso.id_paciente);
                    if (p_revertir) break;
                    busq_c = busq_c->next;
                }
                
                if (p_revertir) {
                    printf("[Tick %03d] ROLLBACK EXITOSO: P%03d restaurado a Prio %d\n", 
                        tick, p_revertir->id, retroceso.prioridad_anterior);
                    p_revertir->prioridad_actual = retroceso.prioridad_anterior;
                    encolar(obtener_cola_prioridad(gestor_colas, retroceso.prioridad_anterior), p_revertir);
                } else {
                    /* Verificar si el paciente fue desencolado */
                    printf("[Tick %03d] ROLLBACK OMITIDO: P%03d ya se encuentra en atencion o finalizado.\n", 
                        tick, retroceso.id_paciente);
                }
            }
            printf("----------------------------------------------------------\n\n");
        }
```

**Explicación:** Cada 20 ticks (tick > 0 y tick % 20 == 0), si la pila no está vacía, se ejecuta un rollback que revierte hasta 2 cambios de triage. Por cada cambio a revertir, se desapila con `pop()` el registro más reciente, obteniendo el ID del paciente y su prioridad anterior. Luego se busca al paciente en todas las colas de prioridad usando `extraer_paciente()`. Si se encuentra, se restaura su prioridad y se reinserta en la cola correcta (rollback exitoso). Si no se encuentra (porque el paciente ya está siendo atendido por un médico o ya fue dado de alta), se reporta como rollback omitido. Este mecanismo demuestra que la pila realiza reversiones reales sobre el estado del sistema.

---

### 3.14. Bucle Principal — Fase 4: Finalización de Atenciones Médicas

📄 **Líneas 247 – 260**

```c
        /* Finalización de atenciones medicas */
        nodo_doc_t* iter_med = gestor_medicos->cabeza;
        while (iter_med != NULL) {
            if (iter_med->medico.occu) {
                iter_med->medico.curr->tiempo_restante--;
                if (iter_med->medico.curr->tiempo_restante <= 0) {
                    printf("[Tick %03d] Medico %d TERMINO atencion de P%03d.\n", 
                        tick, iter_med->medico.id_medico, iter_med->medico.curr->id);
                    insertar_historial(registro_historico, iter_med->medico.curr, tick);
                    iter_med->medico.occu = false;
                    iter_med->medico.curr = NULL;
                }
            }
            iter_med = iter_med->next;
        }
```

**Explicación:** Se recorre la lista de médicos y, para cada médico ocupado, se decrementa el `tiempo_restante` del paciente que está atendiendo. Cuando el tiempo llega a 0 o menos, la atención finaliza: se imprime un mensaje de alta, se inserta al paciente en el historial con `insertar_historial()` (registrando el tick de salida), se marca al médico como libre (`occu = false`) y se desvincula al paciente (`curr = NULL`). El paciente queda almacenado en la lista doblemente enlazada del historial para el reporte final.

---

### 3.15. Bucle Principal — Fase 5: Asignación de Pacientes a Médicos

📄 **Líneas 262 – 278**

```c
        /* Asignar pacientes a medicos */
        iter_med = gestor_medicos->cabeza;
        while (iter_med != NULL) {
            if (!iter_med->medico.occu) {
                /* Evaluar por prioridad descendente */
                for (int prio = 1; prio <= 4; prio++) {
                    espera_t* c_prio = obtener_cola_prioridad(gestor_colas, prio);
                    if (!colaVacia(c_prio)) {
                        iter_med->medico.curr = desencolar(c_prio);
                        iter_med->medico.occu = true;
                        printf("[Tick %03d] El medico %d comenzo la atencion del paciente P%03d (Prioridad %d).\n", 
                            tick, iter_med->medico.id_medico, iter_med->medico.curr->id, 
                            iter_med->medico.curr->prioridad_actual);
                        break;
                    }
                }
            }
            iter_med = iter_med->next;
        }
```

**Explicación:** Se recorre nuevamente la lista de médicos buscando aquellos que estén libres (`!occu`). Para cada médico libre, se itera por las colas de prioridad desde la 1 (más urgente) hasta la 4 (menos urgente). Al encontrar la primera cola no vacía, se desencola al paciente del frente (FIFO), se asigna al médico (`curr = paciente`, `occu = true`) y se sale del bucle `for` con `break`. Esto garantiza que los pacientes más críticos siempre sean atendidos primero.

---

### 3.16. Bucle Principal — Fase 6: Actualización de Tiempos y Alertas

📄 **Líneas 280 – 300**

```c
        /* Actualizar tiempos y alertas */
        nodo_c = gestor_colas->cabeza;
        while(nodo_c != NULL) {
            cola_t *actual = nodo_c->cola_espera->front;
            int cont_acumulados = 0;
            
            while(actual != NULL) {
                actual->paciente->tiempo_espera++;
                cont_acumulados++;
                
                if (actual->paciente->tiempo_espera == 20) {
                    printf("[Tick %03d] ALERTA: El paciente P%03d lleva 20 ticks esperando en la cola de prioridad %d!\n", 
                        tick, actual->paciente->id, nodo_c->prioridad_asignada);
                }
                actual = actual->next;
            }
            
            if (cont_acumulados >= 10 && tick % 5 == 0) {
                printf("[Tick %03d] ALERTA: Acumulacion de %d pacientes en la cola de prioridad %d!\n", 
                    tick, cont_acumulados, nodo_c->prioridad_asignada);
            }
            nodo_c = nodo_c->next;
        }
```

**Explicación:** En esta fase se recorren todas las colas de prioridad y, para cada paciente en espera, se incrementa su `tiempo_espera` en 1. Si un paciente alcanza exactamente 20 ticks de espera, se emite una alerta individual. Además, se cuenta la cantidad de pacientes acumulados en cada cola; si hay 10 o más y el tick es múltiplo de 5, se emite una alerta de acumulación. Estas alertas permiten monitorear situaciones de congestión en tiempo real.

---

### 3.17. Bucle Principal — Snapshot y Condición de Terminación

📄 **Líneas 302 – 322**

```c
        /* Mostrar snapshot del sistema */
        if (tick > 0 && tick % 10 == 0) {
            imprimir_snapshot(tick, gestor_colas, gestor_medicos);
        }

        bool docs_trabajando = false;
        /* Verificar si quedan medicos trabajando */
        iter_med = gestor_medicos->cabeza;
        while (iter_med != NULL) {
            if (iter_med->medico.occu) docs_trabajando = true;
            iter_med = iter_med->next;
        }
        
        /* Si no quedan pacientes en espera y no hay medicos trabajando finalizará la simulación */
        if (colaVacia(pacientes_llegada) && todas_colas_vacias(gestor_colas) && !docs_trabajando) {
            hospital_activo = false;
        } else {
            tick++;
        }
    }
```

**Explicación:** Cada 10 ticks se imprime un snapshot del estado completo del sistema (médicos y colas). Luego se verifica la condición de terminación: se recorre la lista de médicos para comprobar si alguno sigue atendiendo. La simulación finaliza solo si se cumplen tres condiciones simultáneamente: (1) la cola de llegada está vacía (no quedan pacientes por llegar), (2) todas las colas de prioridad están vacías (nadie esperando), y (3) ningún médico está ocupado. Si alguna condición no se cumple, se incrementa el tick y continúa el bucle.

---

### 3.18. Finalización del Programa y Liberación de Memoria

📄 **Líneas 324 – 338**

```c
    printf("\n::: SIMULACION FINALIZADA EN EL TICK %d :::\n", tick);

    /* Resultados finales y recorrido inverso */
    imprimir_metricas(registro_historico);
    imprimir_historial_inverso(registro_historico);

    /* Liberar toda la memoria asignada */
    liberar_cola(pacientes_llegada);
    liberar_lista_colas(gestor_colas);
    liberar_lista_medicos(gestor_medicos);
    liberar_pila(pila_rollback);
    liberar_historial(registro_historico);
    
    return 0;
}
```

**Explicación:** Una vez que el bucle principal termina, se imprime el tick final de la simulación. Se generan los reportes finales: `imprimir_metricas()` calcula y muestra estadísticas por prioridad (espera promedio, máxima, porcentaje de críticos atendidos a tiempo), e `imprimir_historial_inverso()` recorre la lista doblemente enlazada desde la cola hacia la cabeza, mostrando los pacientes dados de alta en orden inverso (más recientes primero). Finalmente, se liberan las cinco estructuras dinámicas del sistema, asegurando cero fugas de memoria.

---

### 3.19. Función `generar_random()`

📄 **Líneas 341 – 343**

```c
float generar_random() {
    return (float)rand() / (float)RAND_MAX;
}
```

**Explicación:** Genera un número aleatorio de tipo `float` en el rango [0.0, 1.0]. Divide el resultado de `rand()` (entero) entre `RAND_MAX` (el valor máximo que puede retornar `rand()`). Se usa en el triage para determinar probabilísticamente si un paciente mejora o empeora.

---

### 3.20. Función `crearColaEspera()`

📄 **Líneas 346 – 351**

```c
espera_t* crearColaEspera(){
    espera_t* cola = (espera_t *)malloc(sizeof(espera_t));
    if (cola == NULL) return NULL;
    cola->front = cola->final = NULL;
    return cola;
}
```

**Explicación:** Asigna memoria para una nueva estructura `espera_t` e inicializa ambos punteros (`front` y `final`) a `NULL`, indicando que la cola está vacía. Retorna `NULL` si `malloc` falla, proporcionando manejo seguro de errores de memoria.

---

### 3.21. Función `encolar()`

📄 **Líneas 353 – 366**

```c
void encolar(espera_t * cola, patient_t *paciente){
    cola_t *nuevo = (cola_t *)malloc(sizeof(cola_t));
    if (nuevo == NULL) return;
    nuevo->paciente = paciente;
    nuevo->next = NULL;

    if (cola->final == NULL) {
        cola->front = nuevo;
        cola->final = nuevo;
    } else {
        cola->final->next = nuevo;
        cola->final = nuevo;
    }
}
```

**Explicación:** Inserta un paciente al final de la cola. Se crea un nuevo nodo `cola_t`, se le asigna el puntero al paciente y `next = NULL` (será el último). Si la cola está vacía (`final == NULL`), tanto `front` como `final` apuntan al nuevo nodo. Si no está vacía, se enlaza el nuevo nodo después del último actual (`cola->final->next = nuevo`) y se actualiza `final`. Operación en O(1) gracias al puntero `final`.

---

### 3.22. Función `desencolar()`

📄 **Líneas 368 – 376**

```c
patient_t *desencolar(espera_t *cola) {
    if (cola->front == NULL) return NULL;
    cola_t *aux = cola->front;
    patient_t *paciente = aux->paciente;
    cola->front = (cola->front)->next;
    if (cola->front == NULL) cola->final = NULL;
    free(aux);
    return paciente;
}
```

**Explicación:** Extrae el paciente del frente de la cola (FIFO). Se guarda el puntero al nodo frontal en `aux` y el puntero al paciente. Se avanza `front` al siguiente nodo. Si la cola queda vacía (`front == NULL`), se actualiza `final` a `NULL` también. Se libera el nodo `aux` (pero no el paciente, que sigue en uso) y se retorna el puntero al paciente. Operación en O(1).

---

### 3.23. Función `extraer_paciente()`

📄 **Líneas 378 – 402**

```c
patient_t* extraer_paciente(espera_t *cola, int id) {
    if (cola->front == NULL) return NULL;
    
    cola_t *actual = cola->front; 
    cola_t *anterior = NULL;

    while (actual != NULL && actual->paciente->id != id) {
        anterior = actual;
        actual = actual->next;
    }

    if (actual == NULL) return NULL;
    patient_t *p = actual->paciente;

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

**Explicación:** Busca y extrae un paciente específico por su ID desde **cualquier posición** de la cola. Recorre la cola con dos punteros (`actual` y `anterior`) hasta encontrar el nodo con el ID buscado. Si no lo encuentra, retorna `NULL`. Si lo encuentra, maneja tres casos: (1) si `anterior == NULL`, el paciente está al frente: se avanza `front` y si queda vacía se actualiza `final`; (2) si `actual->next == NULL`, el paciente está al final: se actualiza `final` al nodo anterior; (3) si está en el medio: se enlaza `anterior->next` con `actual->next`, saltando el nodo extraído. Se libera el nodo contenedor y se retorna el paciente. Esta función es crítica para el re-triage y el rollback.

---

### 3.24. Función `colaVacia()`

📄 **Líneas 404 – 406**

```c
bool colaVacia(espera_t* cola) {
    return (cola->front == NULL);
}
```

**Explicación:** Retorna `true` si la cola no tiene elementos (puntero `front` es `NULL`). Se usa como condición de guarda antes de desencolar y como parte de la verificación de terminación del sistema.

---

### 3.25. Función `liberar_cola()`

📄 **Líneas 408 – 415**

```c
void liberar_cola(espera_t* cola) {
    while (!colaVacia(cola)) {
        patient_t* p = desencolar(cola);
        free(p); 
    }
    free(cola);
}
```

**Explicación:** Libera completamente una cola y todos sus pacientes. Desencola cada paciente iterativamente, liberando tanto el nodo (dentro de `desencolar()`) como la memoria del paciente (`free(p)`). Finalmente libera la estructura `espera_t` misma. Esta función garantiza que no haya fugas de memoria al destruir una cola.

---

### 3.26. Función `crearListaColas()`

📄 **Líneas 418 – 431**

```c
lista_colas_t* crearListaColas() {
    lista_colas_t* lista = (lista_colas_t*)malloc(sizeof(lista_colas_t));
    if (lista == NULL) return NULL;
    lista->cabeza = NULL;
    /* Inicializar prioridades invertidas para mantener orden */
    for (int i = 4; i >= 1; i--) {
        nodo_lista_cola_t* n = (nodo_lista_cola_t*)malloc(sizeof(nodo_lista_cola_t));
        n->prioridad_asignada = i;
        n->cola_espera = crearColaEspera();
        n->next = lista->cabeza;
        lista->cabeza = n;
    }
    return lista;
}
```

**Explicación:** Crea la lista enlazada que contiene las 4 colas de prioridad. Se insertan en orden inverso (de 4 a 1) al frente de la lista, de modo que al finalizar la cabeza apunta a la prioridad 1 y la lista queda ordenada de mayor a menor urgencia (1 → 2 → 3 → 4). Cada nodo contiene su número de prioridad y una cola de espera vacía recién creada. Esta técnica de inserción al frente evita la necesidad de un puntero al final de la lista.

---

### 3.27. Función `obtener_cola_prioridad()`

📄 **Líneas 433 – 440**

```c
espera_t* obtener_cola_prioridad(lista_colas_t* lista, int prio) {
    nodo_lista_cola_t* actual = lista->cabeza;
    while(actual != NULL) {
        if (actual->prioridad_asignada == prio) return actual->cola_espera;
        actual = actual->next;
    }
    return NULL;
}
```

**Explicación:** Recorre la lista de colas buscando el nodo cuya `prioridad_asignada` coincida con el valor `prio` solicitado. Retorna el puntero a la cola de espera correspondiente, o `NULL` si no la encuentra. Esta función se usa cada vez que se necesita encolar o desencolar de una prioridad específica.

---

### 3.28. Función `todas_colas_vacias()`

📄 **Líneas 442 – 449**

```c
bool todas_colas_vacias(lista_colas_t* lista) {
    nodo_lista_cola_t* actual = lista->cabeza;
    while(actual != NULL) {
        if (!colaVacia(actual->cola_espera)) return false;
        actual = actual->next;
    }
    return true;
}
```

**Explicación:** Verifica si todas las colas de prioridad están vacías. Recorre la lista completa y, al encontrar cualquier cola con al menos un paciente, retorna `false` inmediatamente. Solo retorna `true` si todas las colas están vacías. Se usa como parte de la condición de terminación del bucle principal.

---

### 3.29. Función `liberar_lista_colas()`

📄 **Líneas 451 – 460**

```c
void liberar_lista_colas(lista_colas_t* lista) {
    nodo_lista_cola_t* actual = lista->cabeza;
    while(actual != NULL) {
        nodo_lista_cola_t* temp = actual;
        actual = actual->next;
        liberar_cola(temp->cola_espera);
        free(temp);
    }
    free(lista);
}
```

**Explicación:** Libera la lista enlazada de colas y todo su contenido. Para cada nodo, primero libera la cola de espera contenida (con `liberar_cola()`, que a su vez libera los pacientes y nodos internos), luego libera el nodo de la lista. Finalmente libera la estructura `lista_colas_t`. El uso de `temp` evita perder la referencia al nodo actual al avanzar `actual = actual->next`.

---

### 3.30. Función `crearListaMedicos()`

📄 **Líneas 462 – 475**

```c
lista_docs_t* crearListaMedicos(int n) {
    lista_docs_t* lista = (lista_docs_t*)malloc(sizeof(lista_docs_t));
    lista->cabeza = NULL;
    /* Lista de medicos (insercion hacia atras) */
    for (int i = n; i >= 1; i--) {
        nodo_doc_t* nuevo = (nodo_doc_t*)malloc(sizeof(nodo_doc_t));
        nuevo->medico.id_medico = i;
        nuevo->medico.occu = false;
        nuevo->medico.curr = NULL;
        nuevo->next = lista->cabeza;
        lista->cabeza = nuevo;
    }
    return lista;
}
```

**Explicación:** Crea una lista enlazada con `n` médicos. Similar a `crearListaColas()`, se insertan en orden inverso (de n a 1) al frente de la lista, de modo que la lista queda ordenada del médico 1 al n. Cada médico se inicializa como libre (`occu = false`) y sin paciente asignado (`curr = NULL`).

---

### 3.31. Función `liberar_lista_medicos()`

📄 **Líneas 477 – 487**

```c
void liberar_lista_medicos(lista_docs_t* lista) {
    nodo_doc_t* actual = lista->cabeza;
    while(actual != NULL) {
        nodo_doc_t* temp = actual;
        actual = actual->next;
        /* Liberar paciente en atencion */
        if (temp->medico.curr != NULL) free(temp->medico.curr);
        free(temp);
    }
    free(lista);
}
```

**Explicación:** Libera la lista de médicos. Si algún médico todavía tiene un paciente asignado (`curr != NULL`), también libera la memoria de ese paciente. Esto cubre el caso (poco probable en flujo normal) donde la simulación termina con pacientes aún en atención. Luego libera cada nodo y finalmente la estructura contenedora.

---

### 3.32. Función `cargar_pacientes()`

📄 **Líneas 490 – 534**

```c
int cargar_pacientes(espera_t* pacientes, const char *filename){
    FILE *file = fopen(filename, "r");
    if (file == NULL) return -1;
    
    int cant_docs, cant_pacientes;
    
    /* Comprobamos que la lectura sea correcta */
    if (fscanf(file, "%d", &cant_docs) != 1) { 
        fclose(file); 
        return -1; 
    }
    if (fscanf(file, "%d", &cant_pacientes) != 1) { 
        fclose(file); 
        return -1; 
    }

    for (int i = 0; i < cant_pacientes; i++){
        patient_t* p = (patient_t *)malloc(sizeof(patient_t));
        if (p == NULL) { 
            fclose(file); 
            return -1; 
        }
        
        int leidos = fscanf(file, " P%d;%d;%d;%d;%d;%f;%f", 
            &p->id, &p->edad, &p->prioridad_original, 
            &p->tiempo_llegada, &p->tiempo_atencion, 
            &p->prob_empeora, &p->prob_mejora);
        
        if (leidos != 7) {
            printf("[ERROR]: formato corrupto en el archivo. Paciente %d (Linea %d).\n", i+1, 3 + i);
            free(p);
            fclose(file);
            return -1;
        }
        
        p->prioridad_actual = p->prioridad_original;
        p->tiempo_espera = 0;
        p->tiempo_restante = p->tiempo_atencion;
        p->cambios_triage = 0;
        p->ultimo_tick_triage = -1;
        encolar(pacientes, p);
    }
    fclose(file);
    return cant_docs;
}
```

**Explicación:** Lee el archivo de entrada y carga todos los pacientes en la cola de llegada. Primero abre el archivo y lee las dos primeras líneas: cantidad de médicos y cantidad de pacientes, validando cada lectura. Luego itera `cant_pacientes` veces: asigna memoria para un nuevo `patient_t`, parsea los 7 campos del formato `P[ID];[edad];[prio];[llegada];[atencion];[prob_emp];[prob_mej]` con `fscanf`. Si la lectura no retorna exactamente 7 valores, reporta el error con el número de línea, libera la memoria del paciente corrupto, cierra el archivo y retorna `-1`. Si la lectura es correcta, inicializa los campos dinámicos (`prioridad_actual` = original, `tiempo_espera` = 0, `tiempo_restante` = atención, `cambios_triage` = 0, `ultimo_tick_triage` = -1) y encola al paciente. Finalmente cierra el archivo y retorna la cantidad de médicos.

---

### 3.33. Función `crearPila()`

📄 **Líneas 537 – 542**

```c
pila_t* crearPila() {
    pila_t* p = (pila_t*)malloc(sizeof(pila_t));
    if (p == NULL) return NULL;
    p->tope = NULL;
    return p;
}
```

**Explicación:** Asigna memoria para una nueva pila e inicializa el `tope` a `NULL` (pila vacía). Retorna `NULL` si falla la asignación.

---

### 3.34. Función `push()`

📄 **Líneas 544 – 551**

```c
void push(pila_t* pila, int id_paciente, int prioridad_anterior) {
    nodo_pila_t* nuevo_nodo = (nodo_pila_t*)malloc(sizeof(nodo_pila_t));
    if (nuevo_nodo == NULL) return;
    nuevo_nodo->datos.id_paciente = id_paciente;
    nuevo_nodo->datos.prioridad_anterior = prioridad_anterior;
    nuevo_nodo->siguiente = pila->tope;
    pila->tope = nuevo_nodo;
}
```

**Explicación:** Apila un nuevo registro de cambio de triage. Crea un nodo con los datos del cambio (ID del paciente y su prioridad antes de la modificación), enlaza su `siguiente` al tope actual de la pila, y actualiza el `tope` al nuevo nodo. Operación en O(1). Se invoca cada vez que un paciente cambia de prioridad en el triage.

---

### 3.35. Función `pop()`

📄 **Líneas 553 – 562**

```c
registro_cambio_t pop(pila_t* pila) {
    registro_cambio_t value = {-1, -1};
    if (pila->tope == NULL) return value;
    
    nodo_pila_t* aux = pila->tope;
    value = aux->datos;
    pila->tope = pila->tope->siguiente;
    free(aux);
    return value;
}
```

**Explicación:** Desapila y retorna el registro de cambio del tope. Si la pila está vacía, retorna un registro centinela con valores `{-1, -1}`. Si no está vacía, guarda el nodo tope en `aux`, copia sus datos, avanza el `tope` al nodo siguiente, libera `aux` y retorna los datos. Operación en O(1). Se usa durante el rollback periódico para obtener los cambios más recientes a revertir.

---

### 3.36. Función `liberar_pila()`

📄 **Líneas 565 – 570**

```c
void liberar_pila(pila_t* pila) {
    while (pila->tope != NULL) {
        pop(pila); 
    }
    free(pila);
}
```

**Explicación:** Vacía la pila ejecutando `pop()` repetidamente hasta que el tope sea `NULL`, lo que libera cada nodo individualmente. Luego libera la estructura `pila_t`. Reutiliza `pop()` de forma elegante para la limpieza.

---

### 3.37. Función `crearHistorial()`

📄 **Líneas 573 – 577**

```c
historial_t* crearHistorial() {
    historial_t *h = (historial_t *)malloc(sizeof(historial_t));
    if (h) h->cabeza = h->cola = NULL;
    return h;
}
```

**Explicación:** Crea una lista doblemente enlazada vacía para el historial de atención. Inicializa `cabeza` y `cola` a `NULL`. La verificación `if (h)` asegura que solo se inicialicen los campos si `malloc` fue exitoso.

---

### 3.38. Función `liberar_historial()`

📄 **Líneas 580 – 589**

```c
void liberar_historial(historial_t *registro) {
    nodo_historial_t *actual = registro->cabeza;
    while (actual != NULL) {
        nodo_historial_t *temp = actual;
        actual = actual->next;
        free(temp->paciente); 
        free(temp);
    }
    free(registro);
}
```

**Explicación:** Libera la lista doblemente enlazada del historial y todos los pacientes contenidos. Recorre desde la cabeza, y para cada nodo libera primero el paciente apuntado (`free(temp->paciente)`) y luego el nodo mismo (`free(temp)`). Finalmente libera la estructura `historial_t`. La liberación del paciente es correcta porque al insertarse en el historial, el paciente ya no está referenciado por ninguna otra estructura.

---

### 3.39. Función `insertar_historial()`

📄 **Líneas 591 – 601**

```c
void insertar_historial(historial_t *h, patient_t *p, int tick) {
    nodo_historial_t *nuevo = (nodo_historial_t *)malloc(sizeof(nodo_historial_t));
    nuevo->paciente = p;
    nuevo->tick_salida = tick;
    nuevo->next = NULL;
    nuevo->prev = h->cola;

    if (h->cola == NULL) h->cabeza = nuevo;
    else h->cola->next = nuevo;
    h->cola = nuevo;
}
```

**Explicación:** Inserta un paciente dado de alta al final de la lista doblemente enlazada. Se crea un nuevo nodo con el paciente, el tick de salida, `next = NULL` (será el último) y `prev` apuntando al nodo que actualmente es la cola. Si la lista está vacía, el nuevo nodo es tanto `cabeza` como `cola`. Si no, se enlaza después del último nodo (`h->cola->next = nuevo`) y se actualiza `cola`. Los enlaces dobles (`prev`/`next`) permiten recorrer la lista en ambas direcciones.

---

### 3.40. Función `imprimir_metricas()`

📄 **Líneas 603 – 641**

```c
void imprimir_metricas(historial_t *h) {
    if (h->cabeza == NULL) return;

    int sum_espera[4] = {0}, conteo_prio[4] = {0}, max_espera[4] = {0};
    int criticos_a_tiempo = 0, umbral = 15; 

    nodo_historial_t *actual = h->cabeza;
    printf("\n--- REPORTE FINAL DE METRICAS ---\n");
    
    while(actual != NULL) {
        patient_t *p = actual->paciente;
        int idx = p->prioridad_original - 1;

        sum_espera[idx] += p->tiempo_espera;
        conteo_prio[idx]++;

        if (p->tiempo_espera > max_espera[idx]) max_espera[idx] = p->tiempo_espera;
        if (p->prioridad_original == 1 && p->tiempo_espera <= umbral) criticos_a_tiempo++;
        
        actual = actual->next;
    }

    /* Mostramos metricas de colas de espera */
    for (int i = 0; i < 4; i++) {
        float prom;
        if (conteo_prio[i] > 0) prom = (float)sum_espera[i] / (float)conteo_prio[i];
        else prom = 0.0f;
        printf("[Prioridad %d] Atendidos: %d, Espera Promedio: %.2f ticks, Espera Maxima: %d ticks\n", 
            i + 1, conteo_prio[i], prom, max_espera[i]);
    }
    
    float porcentaje_criticos;
    if (conteo_prio[0] > 0) {
        porcentaje_criticos = ((float)criticos_a_tiempo / (float)conteo_prio[0]) * 100.0f;
    } 
    else porcentaje_criticos = 0.0f;

    printf("\nPacientes Criticos (Prio 1) atendidos en <= %d ticks: %.1f%%\n", umbral, porcentaje_criticos);
}
```

**Explicación:** Recorre el historial completo (lista doble, en sentido cabeza → cola) y calcula métricas por nivel de prioridad: suma total de espera, conteo de pacientes atendidos y espera máxima. Usa arreglos auxiliares de 4 posiciones (uno por prioridad) como acumuladores. Adicionalmente cuenta cuántos pacientes críticos (prioridad original = 1) fueron atendidos dentro del umbral de 15 ticks. Finalmente imprime la espera promedio y máxima por prioridad, junto con el porcentaje de pacientes críticos atendidos a tiempo.

---

### 3.41. Función `imprimir_historial_inverso()`

📄 **Líneas 644 – 656**

```c
void imprimir_historial_inverso(historial_t *h) {
    if (h->cola == NULL) return;
    
    printf("\n--- HISTORIAL DE ATENCION (Mas recientes primero) ---\n");
    nodo_historial_t *actual = h->cola;
    while (actual != NULL) {
        patient_t *p = actual->paciente;
        printf("Alta en Tick %03d -> P%03d (Prio Orig: %d | Prio Final: %d | Espera: %d ticks)\n", 
               actual->tick_salida, p->id, p->prioridad_original, p->prioridad_actual, p->tiempo_espera);
        actual = actual->prev;
    }
    printf("-------------------------------------------------------\n");
}
```

**Explicación:** Demuestra el recorrido inverso de la lista doblemente enlazada. Comienza desde la `cola` (último paciente dado de alta) y avanza hacia la `cabeza` usando el puntero `prev` de cada nodo. Para cada paciente muestra el tick de alta, su ID, la prioridad original de ingreso, la prioridad final al momento de ser atendido y su tiempo de espera. Este recorrido inverso solo es posible gracias a la estructura de doble enlace.

---

### 3.42. Función `imprimir_snapshot()`

📄 **Líneas 658 – 679**

```c
void imprimir_snapshot(int tick, lista_colas_t* l_colas, lista_docs_t* l_docs) {
    printf("\n==== [SNAPSHOT TICK %03d] ====\n", tick);
    
    /* Listado medicos */
    nodo_doc_t* doc = l_docs->cabeza;
    while(doc != NULL) {
        if (doc->medico.occu) 
            printf("->  Medico %d: Atendiendo a P%03d (Restante: %d)\n", 
                doc->medico.id_medico, doc->medico.curr->id, doc->medico.curr->tiempo_restante);
        else 
            printf("->  Medico %d: Libre\n", doc->medico.id_medico);
        doc = doc->next;
    }
    
    /* Manejo de colas */
    nodo_lista_cola_t* c = l_colas->cabeza;
    while(c != NULL) {
        int conteo = 0;
        cola_t* p = c->cola_espera->front;
        while(p != NULL) { conteo++; p = p->next; }
        printf("->  Fila Prio %d: %d en espera\n", c->prioridad_asignada, conteo);
        c = c->next;
    }
    printf("=============================\n\n");
}
```

**Explicación:** Genera una fotografía del estado actual del sistema. Primero recorre la lista de médicos mostrando si cada uno está libre o atendiendo a un paciente (con el tiempo restante de consulta). Luego recorre la lista de colas de prioridad y, para cada una, cuenta manualmente cuántos pacientes hay en espera recorriendo los nodos de la cola. Imprime el conteo por prioridad. Esta función se invoca cada 10 ticks para monitorear el sistema durante la simulación.

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
