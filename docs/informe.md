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

### 2.1. Estructuras de Datos y Justificación

| Estructura | Tipo | Justificación |
|:---|:---|:---|
| **Colas de espera** (`espera_t`) | Cola FIFO enlazada | Representan las filas por prioridad. FIFO garantiza atención en orden de llegada dentro de cada nivel. |
| **Lista de colas** (`lista_colas_t`) | Lista simplemente enlazada | Agrupa las 4 colas de prioridad en una estructura recorrible, sin arreglos estáticos. |
| **Lista de médicos** (`lista_docs_t`) | Lista simplemente enlazada | Pool de médicos con estado libre/ocupado y puntero al paciente en atención. |
| **Pila de rollback** (`pila_t`) | Pila LIFO enlazada | Registra cambios de triage (ID + prioridad anterior) para revertirlos en orden LIFO. |
| **Historial** (`historial_t`) | Lista doblemente enlazada | Almacena altas con recorrido bidireccional para reportes (cronológico e inverso). |

### 2.2. Funciones Clave por Componente

**Gestión de colas (cola FIFO enlazada):**
- `crearColaEspera()`: Asigna memoria para una nueva cola e inicializa los punteros `front` y `final` a `NULL`, representando una cola vacía lista para recibir pacientes.
- `encolar()`: Crea un nuevo nodo con el paciente recibido y lo inserta al final de la cola enlazando con el puntero `final`, operando en tiempo O(1).
- `desencolar()`: Extrae y retorna el paciente del frente de la cola (FIFO), avanza el puntero `front` al siguiente nodo y libera el nodo extraído. Operación O(1).
- `extraer_paciente()`: Busca un paciente por su ID recorriendo la cola y lo extrae desde **cualquier posición** (frente, medio o final), actualizando correctamente los punteros `front`, `final` y los enlaces intermedios. Es la función más crítica del sistema, ya que habilita el re-triage y el rollback.
- `colaVacia()`: Retorna `true` si el puntero `front` es `NULL`. Se utiliza como guarda antes de desencolar y como parte de la condición de terminación de la simulación.
- `liberar_cola()`: Desencola y libera cada paciente iterativamente, luego libera la estructura contenedora, garantizando cero fugas de memoria.

**Gestor de colas por prioridad (lista enlazada de colas):**
- `crearListaColas()`: Crea dinámicamente 4 nodos (uno por prioridad 1 a 4), cada uno conteniendo su propia cola de espera vacía. Los inserta en orden inverso al frente de la lista para que queden ordenados de mayor a menor urgencia.
- `obtener_cola_prioridad()`: Recorre la lista buscando el nodo cuya prioridad coincida con la solicitada y retorna su cola de espera. Permite acceder a cualquier cola por su nivel de prioridad.
- `todas_colas_vacias()`: Recorre todas las colas verificando si alguna contiene pacientes. Retorna `true` solo si todas están vacías; se usa como parte de la condición de terminación del sistema.

**Pila de rollback (pila LIFO enlazada):**
- `crearPila()`: Asigna memoria e inicializa el `tope` a `NULL` (pila vacía).
- `push()`: Crea un nodo con el ID del paciente y su prioridad anterior al cambio, lo enlaza como nuevo tope de la pila. Se invoca cada vez que un paciente cambia de prioridad en el triage, registrando el cambio para una posible reversión futura.
- `pop()`: Desapila el nodo del tope, retorna sus datos (ID y prioridad anterior) y libera el nodo. Si la pila está vacía, retorna un registro centinela `{-1, -1}`. Se usa durante el rollback periódico para obtener los cambios más recientes.
- `liberar_pila()`: Vacía la pila invocando `pop()` repetidamente y luego libera la estructura.

**Historial de atención (lista doblemente enlazada):**
- `crearHistorial()`: Asigna memoria e inicializa `cabeza` y `cola` a `NULL`.
- `insertar_historial()`: Crea un nodo con el paciente dado de alta y el tick de salida, lo inserta al final de la lista doble manteniendo los punteros `prev`/`next`. Permite inserción en O(1) gracias al puntero `cola`.
- `imprimir_metricas()`: Recorre el historial completo calculando espera promedio, espera máxima y porcentaje de pacientes críticos atendidos dentro de un umbral de 15 ticks, agrupando estadísticas por prioridad original.
- `imprimir_historial_inverso()`: Recorre la lista desde la `cola` hacia la `cabeza` usando el puntero `prev`, mostrando las altas más recientes primero. Este recorrido inverso solo es posible gracias a la estructura de doble enlace.

**Carga de datos y funciones auxiliares:**
- `cargar_pacientes()`: Abre el archivo de entrada, lee la cantidad de médicos y pacientes, parsea cada línea con `fscanf` validando exactamente 7 campos. Inicializa los campos dinámicos del paciente y lo encola en la cola de llegada. Reporta errores indicando la línea exacta del problema.
- `imprimir_snapshot()`: Genera una fotografía del estado del sistema recorriendo la lista de médicos (mostrando su estado) y la lista de colas (contando pacientes por prioridad). Se invoca cada 10 ticks.
- `generar_random()`: Retorna un `float` aleatorio en [0.0, 1.0] dividiendo `rand()` entre `RAND_MAX`. Se usa en el triage para evaluar probabilísticamente si un paciente mejora o empeora.

### 2.3. Interacción entre Componentes

```
 Archivo ──► Cola de llegada ──► Lista de colas por prioridad (1-4)
                                       │              │
                              re-triage│              │ asignar médico
                          (extraer +   │              │ (desencolar)
                           encolar)    │              ▼
                     Pila rollback ◄───┘    Lista de médicos
                     (push/pop cada 20t)          │
                                                  │ atención finalizada
                                                  ▼
                                          Historial (lista doble)
```

**Fases por tick:** (1) Llegada → (2) Re-triage → (3) Rollback periódico → (4) Finalización de atenciones → (5) Asignación de médicos → (6) Alertas.

---

## 3. Implementación del Sistema — Análisis del Código

### 3.1. Definición de Estructuras (Líneas 17–95)

📄 **Líneas 17–31** — Estructura del paciente:

```c
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

Los campos `id` a `prob_empeora` se cargan del archivo. Los campos `prioridad_actual`, `tiempo_restante`, `tiempo_espera`, `cambios_triage` y `ultimo_tick_triage` se actualizan dinámicamente durante la simulación. `ultimo_tick_triage` evita evaluar al mismo paciente dos veces en un tick.

📄 **Líneas 33–48** — Médico y cola de espera:

```c
typedef struct doc {
    int id_medico;
    bool occu;
    struct patient *curr;
} doc_t;

typedef struct cola {
    patient_t *paciente;
    struct cola *next;
} cola_t;

typedef struct espera {
    cola_t *front;
    cola_t *final;
} espera_t;
```

`doc_t` representa al médico con su estado (libre/ocupado) y un puntero al paciente en atención. `cola_t` es el nodo de la cola enlazada, y `espera_t` la controla con punteros `front`/`final` para encolar y desencolar en O(1).

📄 **Líneas 50–67** — Listas enlazadas de médicos y colas por prioridad:

```c
typedef struct nodo_doc {
    doc_t medico;
    struct nodo_doc *next;
} nodo_doc_t;

typedef struct lista_docs {
    nodo_doc_t *cabeza;
} lista_docs_t;

typedef struct nodo_lista_cola {
    int prioridad_asignada;
    espera_t *cola_espera;
    struct nodo_lista_cola *next;
} nodo_lista_cola_t;

typedef struct lista_colas {
    nodo_lista_cola_t *cabeza;
} lista_colas_t;
```

Ambas son listas simplemente enlazadas. `nodo_lista_cola_t` asocia un nivel de prioridad (1–4) con su cola de espera, permitiendo iterar sobre todas las prioridades sin arreglos estáticos.

📄 **Líneas 69–95** — Pila e historial:

```c
typedef struct registro_cambio {
    int id_paciente;
    int prioridad_anterior;
} registro_cambio_t;

typedef struct nodo_pila {
    registro_cambio_t datos;
    struct nodo_pila *siguiente;
} nodo_pila_t;

typedef struct pila {
    nodo_pila_t* tope;
} pila_t;

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

La pila almacena registros de cambio (ID + prioridad anterior) con comportamiento LIFO. El historial es una lista doblemente enlazada con punteros `next`/`prev` y acceso por `cabeza` (primer alta) y `cola` (última alta), habilitando recorridos bidireccionales.

---

### 3.2. Función `main()` — Inicialización y Validación (Líneas 130–160)

```c
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf(":: USO CORRECTO: %s <nombre_archivo.txt>\n", argv[0]);
        return 0;
    }
    srand((unsigned int)time(NULL));

    espera_t* pacientes_llegada = crearColaEspera();
    lista_colas_t* gestor_colas = crearListaColas();
    pila_t* pila_rollback = crearPila();
    historial_t* registro_historico = crearHistorial();
    
    int n_docs = cargar_pacientes(pacientes_llegada, argv[1]);
    if (n_docs == -1) {
        /* Error: liberar todo y salir */
        liberar_cola(pacientes_llegada);
        liberar_lista_colas(gestor_colas);
        liberar_pila(pila_rollback);
        liberar_historial(registro_historico);
        return 1; 
    }
    lista_docs_t* gestor_medicos = crearListaMedicos(n_docs);
```

Se valida que se reciba el archivo como argumento. Se inicializa la semilla aleatoria y se crean las 5 estructuras principales. Si `cargar_pacientes()` falla (retorna `-1`), se liberan todas las estructuras antes de terminar, garantizando cero fugas de memoria incluso en caso de error.

---

### 3.3. Bucle Principal — Llegada de Pacientes (Líneas 165–173)

```c
    while(hospital_activo) {
        /* Llegada de nuevos pacientes */
        while(!colaVacia(pacientes_llegada) && pacientes_llegada->front->paciente->tiempo_llegada <= tick) {
            patient_t* p = desencolar(pacientes_llegada);
            espera_t* cola_dest = obtener_cola_prioridad(gestor_colas, p->prioridad_actual);
            encolar(cola_dest, p);
            printf("[Tick %03d] LLEGO paciente P%03d (Prio %d).\n", tick, p->id, p->prioridad_actual);
        }
```

Se desencolan los pacientes cuyo `tiempo_llegada ≤ tick` y se insertan en la cola de prioridad correspondiente. El `while` interno maneja múltiples llegadas simultáneas en el mismo tick.

---

### 3.4. Bucle Principal — Evaluación de Triage (Líneas 175–218)

```c
        nodo_lista_cola_t* nodo_c = gestor_colas->cabeza;
        while(nodo_c != NULL) {
            cola_t *actual = nodo_c->cola_espera->front;
            while(actual != NULL) {
                patient_t *p = actual->paciente;
                cola_t *siguiente_seguro = actual->next;
                
                if (p->ultimo_tick_triage != tick) {
                    float chance = generar_random();
                    int nueva_prio = p->prioridad_actual;
                    
                    if (chance < p->prob_empeora && p->prioridad_actual > 1)
                        nueva_prio--;
                    else if (chance > (1.0f - p->prob_mejora) && p->prioridad_actual < 4)
                        nueva_prio++;
                    
                    if (nueva_prio != p->prioridad_actual) {
                        push(pila_rollback, p->id, p->prioridad_actual);
                        patient_t *extraido = extraer_paciente(nodo_c->cola_espera, p->id);
                        if (extraido) {
                            extraido->cambios_triage++;
                            extraido->ultimo_tick_triage = tick;
                            extraido->prioridad_actual = nueva_prio;
                            encolar(obtener_cola_prioridad(gestor_colas, nueva_prio), extraido);
                            if (extraido->cambios_triage >= 3)
                                printf("[Tick %03d] ALERTA: P%03d ha cambiado %d veces!\n", ...);
                        }
                    }
                }
                actual = siguiente_seguro;
            }
            nodo_c = nodo_c->next;
        }
```

Se recorren todas las colas evaluando probabilísticamente cada paciente. Se guarda `siguiente_seguro` antes de procesar, ya que si el paciente es extraído, su nodo se libera. Si la prioridad cambia: se registra en la pila (`push`), se extrae al paciente de su cola actual (`extraer_paciente`), se actualiza su prioridad y se reinserta en la cola nueva. Si acumula ≥3 cambios, se emite alerta.

---

### 3.5. Bucle Principal — Rollback Periódico (Líneas 220–245)

```c
        if (tick > 0 && tick % 20 == 0 && pila_rollback->tope != NULL) {
            for (int r = 0; r < 2 && pila_rollback->tope != NULL; r++) {
                registro_cambio_t retroceso = pop(pila_rollback);
                patient_t* p_revertir = NULL;
                
                nodo_lista_cola_t* busq_c = gestor_colas->cabeza;
                while(busq_c != NULL) {
                    p_revertir = extraer_paciente(busq_c->cola_espera, retroceso.id_paciente);
                    if (p_revertir) break;
                    busq_c = busq_c->next;
                }
                
                if (p_revertir) {
                    p_revertir->prioridad_actual = retroceso.prioridad_anterior;
                    encolar(obtener_cola_prioridad(gestor_colas, retroceso.prioridad_anterior), p_revertir);
                } else {
                    printf("[Tick %03d] ROLLBACK OMITIDO: P%03d ya en atencion o finalizado.\n", ...);
                }
            }
        }
```

Cada 20 ticks se revierten hasta 2 cambios usando la pila. Se desapila con `pop()`, se busca al paciente en todas las colas con `extraer_paciente()`. Si se encuentra, se restaura su prioridad y se reinserta (rollback exitoso). Si no está en ninguna cola (ya atendido), se reporta como omitido. Esto demuestra que la pila efectúa reversiones reales.

---

### 3.6. Bucle Principal — Atención y Asignación Médica (Líneas 247–278)

```c
        /* Finalización de atenciones */
        nodo_doc_t* iter_med = gestor_medicos->cabeza;
        while (iter_med != NULL) {
            if (iter_med->medico.occu) {
                iter_med->medico.curr->tiempo_restante--;
                if (iter_med->medico.curr->tiempo_restante <= 0) {
                    insertar_historial(registro_historico, iter_med->medico.curr, tick);
                    iter_med->medico.occu = false;
                    iter_med->medico.curr = NULL;
                }
            }
            iter_med = iter_med->next;
        }

        /* Asignar pacientes a médicos libres */
        iter_med = gestor_medicos->cabeza;
        while (iter_med != NULL) {
            if (!iter_med->medico.occu) {
                for (int prio = 1; prio <= 4; prio++) {
                    espera_t* c_prio = obtener_cola_prioridad(gestor_colas, prio);
                    if (!colaVacia(c_prio)) {
                        iter_med->medico.curr = desencolar(c_prio);
                        iter_med->medico.occu = true;
                        break;
                    }
                }
            }
            iter_med = iter_med->next;
        }
```

Se recorre la lista de médicos decrementando `tiempo_restante` del paciente en atención. Cuando llega a 0, se inserta al paciente en el historial (lista doble) y se libera al médico. Luego, cada médico libre busca pacientes desde la prioridad 1 (más urgente) hasta la 4, desencolando al primero disponible. Esto garantiza que los pacientes críticos siempre sean atendidos primero.

---

### 3.7. Bucle Principal — Alertas y Condición de Terminación (Líneas 280–322)

```c
        /* Actualizar tiempos y alertas */
        nodo_c = gestor_colas->cabeza;
        while(nodo_c != NULL) {
            cola_t *actual = nodo_c->cola_espera->front;
            int cont_acumulados = 0;
            while(actual != NULL) {
                actual->paciente->tiempo_espera++;
                cont_acumulados++;
                if (actual->paciente->tiempo_espera == 20)
                    printf("ALERTA: P%03d lleva 20 ticks esperando!\n", ...);
                actual = actual->next;
            }
            if (cont_acumulados >= 10 && tick % 5 == 0)
                printf("ALERTA: Acumulacion de %d pacientes!\n", ...);
            nodo_c = nodo_c->next;
        }

        if (tick > 0 && tick % 10 == 0)
            imprimir_snapshot(tick, gestor_colas, gestor_medicos);

        /* Condición de terminación */
        if (colaVacia(pacientes_llegada) && todas_colas_vacias(gestor_colas) && !docs_trabajando)
            hospital_activo = false;
        else
            tick++;
```

Se incrementa `tiempo_espera` de cada paciente en cola y se emiten alertas (espera ≥ 20 ticks o acumulación ≥ 10 pacientes). Cada 10 ticks se imprime un snapshot del sistema. La simulación finaliza cuando: no quedan pacientes por llegar, todas las colas están vacías y ningún médico está ocupado.

---

### 3.8. Cierre y Liberación de Memoria (Líneas 324–338)

```c
    imprimir_metricas(registro_historico);
    imprimir_historial_inverso(registro_historico);

    liberar_cola(pacientes_llegada);
    liberar_lista_colas(gestor_colas);
    liberar_lista_medicos(gestor_medicos);
    liberar_pila(pila_rollback);
    liberar_historial(registro_historico);
    return 0;
```

Se generan los reportes finales y se liberan las 5 estructuras. Cada función `liberar_*` recorre su estructura completa liberando nodos y datos contenidos.

---

### 3.9. Funciones de Cola (Líneas 346–415)

📄 **Líneas 346–366** — `crearColaEspera()` y `encolar()`:

```c
espera_t* crearColaEspera(){
    espera_t* cola = (espera_t *)malloc(sizeof(espera_t));
    if (cola == NULL) return NULL;
    cola->front = cola->final = NULL;
    return cola;
}

void encolar(espera_t *cola, patient_t *paciente){
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

`crearColaEspera()` asigna memoria e inicializa `front`/`final` a `NULL`. `encolar()` crea un nuevo nodo y lo inserta al final: si la cola está vacía, es tanto `front` como `final`; si no, se enlaza después de `final` y se actualiza. Ambas operaciones son O(1).

📄 **Líneas 368–406** — `desencolar()`, `extraer_paciente()` y `colaVacia()`:

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

bool colaVacia(espera_t* cola) {
    return (cola->front == NULL);
}
```

`desencolar()` extrae del frente en O(1), liberando el nodo contenedor pero no el paciente. `extraer_paciente()` es la función más importante para el re-triage: busca por ID y extrae desde **cualquier posición** (frente, medio o final), actualizando correctamente los punteros `front` y `final` en todos los casos. `colaVacia()` verifica si `front` es `NULL`.

📄 **Líneas 408–415** — `liberar_cola()`:

```c
void liberar_cola(espera_t* cola) {
    while (!colaVacia(cola)) {
        patient_t* p = desencolar(cola);
        free(p); 
    }
    free(cola);
}
```

Desencola y libera cada paciente iterativamente, luego libera la estructura. Garantiza cero fugas de memoria.

---

### 3.10. Funciones del Gestor de Prioridades (Líneas 418–460)

📄 **Líneas 418–449** — Creación, búsqueda y verificación:

```c
lista_colas_t* crearListaColas() {
    lista_colas_t* lista = (lista_colas_t*)malloc(sizeof(lista_colas_t));
    if (lista == NULL) return NULL;
    lista->cabeza = NULL;
    for (int i = 4; i >= 1; i--) {
        nodo_lista_cola_t* n = (nodo_lista_cola_t*)malloc(sizeof(nodo_lista_cola_t));
        n->prioridad_asignada = i;
        n->cola_espera = crearColaEspera();
        n->next = lista->cabeza;
        lista->cabeza = n;
    }
    return lista;
}

espera_t* obtener_cola_prioridad(lista_colas_t* lista, int prio) {
    nodo_lista_cola_t* actual = lista->cabeza;
    while(actual != NULL) {
        if (actual->prioridad_asignada == prio) return actual->cola_espera;
        actual = actual->next;
    }
    return NULL;
}

bool todas_colas_vacias(lista_colas_t* lista) {
    nodo_lista_cola_t* actual = lista->cabeza;
    while(actual != NULL) {
        if (!colaVacia(actual->cola_espera)) return false;
        actual = actual->next;
    }
    return true;
}
```

`crearListaColas()` inserta 4 nodos en orden inverso (4→1) al frente de la lista, resultando en orden 1→2→3→4. Cada nodo contiene una cola vacía. `obtener_cola_prioridad()` recorre la lista buscando por prioridad. `todas_colas_vacias()` verifica si todas están vacías (condición de terminación).

---

### 3.11. Funciones de la Lista de Médicos (Líneas 462–487)

```c
lista_docs_t* crearListaMedicos(int n) {
    lista_docs_t* lista = (lista_docs_t*)malloc(sizeof(lista_docs_t));
    lista->cabeza = NULL;
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

void liberar_lista_medicos(lista_docs_t* lista) {
    nodo_doc_t* actual = lista->cabeza;
    while(actual != NULL) {
        nodo_doc_t* temp = actual;
        actual = actual->next;
        if (temp->medico.curr != NULL) free(temp->medico.curr);
        free(temp);
    }
    free(lista);
}
```

Crea `n` médicos como lista enlazada, todos inicializados como libres. La liberación verifica si algún médico aún tiene paciente asignado para liberarlo también.

---

### 3.12. Lectura desde Archivo (Líneas 490–534)

```c
int cargar_pacientes(espera_t* pacientes, const char *filename){
    FILE *file = fopen(filename, "r");
    if (file == NULL) return -1;
    
    int cant_docs, cant_pacientes;
    if (fscanf(file, "%d", &cant_docs) != 1) { fclose(file); return -1; }
    if (fscanf(file, "%d", &cant_pacientes) != 1) { fclose(file); return -1; }

    for (int i = 0; i < cant_pacientes; i++){
        patient_t* p = (patient_t *)malloc(sizeof(patient_t));
        if (p == NULL) { fclose(file); return -1; }
        
        int leidos = fscanf(file, " P%d;%d;%d;%d;%d;%f;%f", 
            &p->id, &p->edad, &p->prioridad_original, 
            &p->tiempo_llegada, &p->tiempo_atencion, 
            &p->prob_empeora, &p->prob_mejora);
        
        if (leidos != 7) {
            printf("[ERROR]: formato corrupto. Paciente %d (Linea %d).\n", i+1, 3+i);
            free(p); fclose(file); return -1;
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

Lee la cantidad de médicos y pacientes, luego parsea cada línea con `fscanf` validando 7 campos. Si falla, reporta la línea exacta del error. Inicializa los campos dinámicos del paciente y lo encola. Retorna la cantidad de médicos o `-1` si hay error.

---

### 3.13. Funciones de la Pila (Líneas 537–570)

```c
pila_t* crearPila() {
    pila_t* p = (pila_t*)malloc(sizeof(pila_t));
    if (p == NULL) return NULL;
    p->tope = NULL;
    return p;
}

void push(pila_t* pila, int id_paciente, int prioridad_anterior) {
    nodo_pila_t* nuevo_nodo = (nodo_pila_t*)malloc(sizeof(nodo_pila_t));
    if (nuevo_nodo == NULL) return;
    nuevo_nodo->datos.id_paciente = id_paciente;
    nuevo_nodo->datos.prioridad_anterior = prioridad_anterior;
    nuevo_nodo->siguiente = pila->tope;
    pila->tope = nuevo_nodo;
}

registro_cambio_t pop(pila_t* pila) {
    registro_cambio_t value = {-1, -1};
    if (pila->tope == NULL) return value;
    nodo_pila_t* aux = pila->tope;
    value = aux->datos;
    pila->tope = pila->tope->siguiente;
    free(aux);
    return value;
}

void liberar_pila(pila_t* pila) {
    while (pila->tope != NULL) { pop(pila); }
    free(pila);
}
```

`push()` crea un nodo con los datos del cambio y lo enlaza como nuevo tope (O(1)). `pop()` desapila el tope, retorna sus datos y libera el nodo (O(1)). Si la pila está vacía, retorna `{-1, -1}` como centinela. `liberar_pila()` reutiliza `pop()` repetidamente hasta vaciarla.

---

### 3.14. Funciones del Historial — Lista Doblemente Enlazada (Líneas 573–656)

```c
historial_t* crearHistorial() {
    historial_t *h = (historial_t *)malloc(sizeof(historial_t));
    if (h) h->cabeza = h->cola = NULL;
    return h;
}

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

void imprimir_historial_inverso(historial_t *h) {
    if (h->cola == NULL) return;
    nodo_historial_t *actual = h->cola;
    while (actual != NULL) {
        patient_t *p = actual->paciente;
        printf("Alta en Tick %03d -> P%03d (Prio Orig: %d | Prio Final: %d | Espera: %d ticks)\n", 
               actual->tick_salida, p->id, p->prioridad_original, p->prioridad_actual, p->tiempo_espera);
        actual = actual->prev;
    }
}
```

`insertar_historial()` agrega al final de la lista doble: `prev` apunta al antiguo último nodo, `next` es `NULL`. `imprimir_historial_inverso()` recorre desde la `cola` hacia la `cabeza` usando `prev`, mostrando los pacientes más recientes primero. Este recorrido inverso solo es posible gracias a la doble enlace.

📄 **Líneas 603–641** — `imprimir_metricas()` recorre el historial calculando espera promedio, máxima y porcentaje de pacientes críticos atendidos dentro de 15 ticks, agrupando estadísticas por prioridad original.

---

### 3.15. Función `imprimir_snapshot()` (Líneas 658–679)

```c
void imprimir_snapshot(int tick, lista_colas_t* l_colas, lista_docs_t* l_docs) {
    printf("\n==== [SNAPSHOT TICK %03d] ====\n", tick);
    nodo_doc_t* doc = l_docs->cabeza;
    while(doc != NULL) {
        if (doc->medico.occu) printf("->  Medico %d: Atendiendo a P%03d (Restante: %d)\n", ...);
        else printf("->  Medico %d: Libre\n", doc->medico.id_medico);
        doc = doc->next;
    }
    nodo_lista_cola_t* c = l_colas->cabeza;
    while(c != NULL) {
        int conteo = 0;
        cola_t* p = c->cola_espera->front;
        while(p != NULL) { conteo++; p = p->next; }
        printf("->  Fila Prio %d: %d en espera\n", c->prioridad_asignada, conteo);
        c = c->next;
    }
}
```

Recorre la lista de médicos mostrando su estado (libre o atendiendo con tiempo restante) y luego recorre las colas contando pacientes en espera por prioridad. Se invoca cada 10 ticks como monitor del sistema.

---

## 4. Pruebas de Ejecución

### 4.1. Prueba 1 — `ejemploEntrada.txt` (10 pacientes, 3 médicos)

**Extracto de la ejecución:**

```
::: Iniciando el sistema con 3 medicos :::

[Tick 000] LLEGO paciente P001 (Prio 1).
[Tick 000] El medico 1 comenzo la atencion del paciente P001 (Prioridad 1).
[Tick 002] LLEGO paciente P003 (Prio 4).
[Tick 002] LLEGO paciente P004 (Prio 2).
[Tick 002] TRIAGE: P004 cambia de Prio 2 a 1
[Tick 002] El medico 3 comenzo la atencion del paciente P004 (Prioridad 1).
[Tick 007] TRIAGE: P007 cambia de Prio 2 a 1
[Tick 007] TRIAGE: P006 cambia de Prio 2 a 1
[Tick 012] ALERTA: P003 ha cambiado de prioridad 3 veces!

==== [SNAPSHOT TICK 010] ====
->  Medico 1: Atendiendo a P007 (Restante: 7)
->  Medico 2: Atendiendo a P005 (Restante: 6)
->  Medico 3: Atendiendo a P006 (Restante: 3)
->  Fila Prio 1: 2 en espera
->  Fila Prio 3: 1 en espera
->  Fila Prio 4: 1 en espera
=============================

--- ROLLBACK PERIODICO: Revirtiendo ultimos 2 cambios (Tick 20) ---
[Tick 020] ROLLBACK OMITIDO: P003 ya se encuentra en atencion o finalizado.
----------------------------------------------------------

::: SIMULACION FINALIZADA EN EL TICK 22 :::
```

| Prioridad | Atendidos | Espera Promedio | Espera Máxima |
|:---:|:---:|:---:|:---:|
| 1 | 2 | 1.50 ticks | 3 ticks |
| 2 | 3 | 3.00 ticks | 6 ticks |
| 3 | 3 | 4.33 ticks | 8 ticks |
| 4 | 2 | 14.00 ticks | 17 ticks |

**Pacientes críticos atendidos en ≤ 15 ticks:** 100.0%

**Casos borde:** Re-triage múltiple de P003 (4 cambios, activó alerta), llegadas simultáneas en tick 2, rollback omitido porque el paciente ya estaba en atención.

---

### 4.2. Prueba 2 — `test_bordes.txt` (5 pacientes, 2 médicos, todos llegan en tick 0)

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
[Tick 010] Medico 1 TERMINO atencion de P003.
[Tick 010] Medico 2 TERMINO atencion de P005.

==== [SNAPSHOT TICK 010] ====
->  Medico 1: Libre
->  Medico 2: Libre
->  Fila Prio 1-4: 0 en espera
=============================

::: SIMULACION FINALIZADA EN EL TICK 10 :::
```

| Prioridad | Atendidos | Espera Promedio | Espera Máxima |
|:---:|:---:|:---:|:---:|
| 1 | 2 | 0.00 ticks | 0 ticks |
| 2 | 1 | 3.00 ticks | 3 ticks |
| 3 | 1 | 4.00 ticks | 4 ticks |
| 4 | 1 | 8.00 ticks | 8 ticks |

**Pacientes críticos atendidos en ≤ 15 ticks:** 100.0%

**Casos borde:** 5 llegadas simultáneas en tick 0, colas vacías al finalizar, re-triage de P004 (Prio 2→1) atendido inmediatamente, P003 (Prio 4) espera más tiempo como es correcto por la priorización.

---

### 4.3. Prueba 3 — `pacientes.txt` (30 pacientes, 1 médico — estrés máximo)

```
::: Iniciando el sistema con 1 medicos :::
[Tick 000] LLEGO paciente P001 (Prio 4).
[Tick 000] LLEGO paciente P002 (Prio 1).
[Tick 000] El medico 1 comenzo la atencion del paciente P002 (Prioridad 1).
...
[Tick 020] ALERTA: El paciente P008 lleva 20 ticks esperando!

--- ROLLBACK PERIODICO (Tick 20) ---
[Tick 020] ROLLBACK EXITOSO: P008 restaurado a Prio 3
[Tick 020] ROLLBACK EXITOSO: P004 restaurado a Prio 4
...
--- ROLLBACK PERIODICO (Tick 220) ---
[Tick 220] ROLLBACK EXITOSO: P030 restaurado a Prio 4
[Tick 220] ROLLBACK EXITOSO: P030 restaurado a Prio 3
...
::: SIMULACION FINALIZADA EN EL TICK 247 :::
```

| Prioridad | Atendidos | Espera Promedio | Espera Máxima |
|:---:|:---:|:---:|:---:|
| 1 | 6 | 80.50 ticks | 175 ticks |
| 2 | 7 | 84.86 ticks | 166 ticks |
| 3 | 10 | 111.00 ticks | 215 ticks |
| 4 | 7 | 55.14 ticks | 106 ticks |

**Pacientes críticos atendidos en ≤ 15 ticks:** 16.7%

**Casos borde:** P030 acumuló 72 cambios de prioridad (robustez de extracción/inserción), rollbacks exitosos reales en tick 220, alerta de espera excesiva, simulación de 247 ticks con 1 solo médico evidenciando impacto de recursos limitados.

---

## 5. Conclusiones

El desarrollo de este sistema de simulación de urgencias hospitalarias permitió aplicar de manera integral los conceptos de estructuras de datos dinámicas estudiados durante el curso, trasladándolos a un escenario con relevancia práctica real. La implementación demuestra que las colas, pilas y listas enlazadas no son abstracciones meramente teóricas, sino herramientas poderosas para modelar sistemas donde los datos fluyen, se transforman y se reorganizan de forma continua. El mecanismo de rollback mediante la pila resultó particularmente valioso para comprender cómo el principio LIFO permite implementar funcionalidades de «deshacer» en sistemas complejos. Asimismo, la lista doblemente enlazada demostró su versatilidad al facilitar recorridos bidireccionales para la generación de reportes. Las pruebas realizadas bajo distintas configuraciones —desde escenarios controlados hasta pruebas de estrés con recursos mínimos— validaron la robustez y coherencia del sistema. Este proyecto refuerza el aprendizaje de que la elección adecuada de la estructura de datos no solo determina la corrección del programa, sino también su capacidad para escalar y adaptarse a condiciones adversas, una competencia fundamental en el desarrollo de software profesional.
