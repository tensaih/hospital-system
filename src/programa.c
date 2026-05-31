#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

/* Definitions */
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
} patient_t;

typedef struct doc {
    bool occu;
    struct patient *curr;
} doc_t;

/* COLAS */
typedef struct cola {
    patient_t *paciente;
    struct cola *next;
} cola_t;

typedef struct espera {
    cola_t *front;
    cola_t *final;
} espera_t;

/* PILA */
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

/* HISTORIAL (Lista Doble) */
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

/* Prototipos de funciones */
espera_t* crearColaEspera();
void encolar(espera_t *cola, patient_t *paciente);
patient_t* desencolar(espera_t *cola);
patient_t* extraer_paciente(espera_t *cola, int id);
bool colasEsperaVacias(espera_t** colas, int n);
bool colaVacia(espera_t* cola);

pila_t* crearPila();
void push(pila_t* pila, int id_paciente, int prioridad_anterior);
registro_cambio_t pop(pila_t* pila);

historial_t* crearHistorial();
void liberar_historial(historial_t *registro);
void insertar_historial(historial_t *h, patient_t *p, int tick);
void imprimir_metricas(historial_t *h);

int cargar_pacientes(espera_t *pacientes, const char *filename);
void imprimir_pacientes(espera_t *cola);
float generar_random();


int main(){
    srand((unsigned int)time(NULL));

    /* Inicialización de estructuras */
    espera_t* pacientes = crearColaEspera();
    espera_t* colas[4];
    for (int i = 0; i < 4; i++){
        colas[i] = crearColaEspera();
    }
    
    pila_t* pila_rollback = crearPila();
    historial_t* registro_historico = crearHistorial();
    
    /* Carga de archivo */
    int n_docs = cargar_pacientes(pacientes, "bin/pacientes.txt");
    if (n_docs == -1){
        printf("Hubo un error al cargar los pacientes.\n");
        return 0;
    }

    /* Inicialización de médicos */
    doc_t* medicos = (doc_t*)malloc(n_docs * sizeof(doc_t));
    for(int i = 0; i < n_docs; i++) {
        medicos[i].occu = false;
        medicos[i].curr = NULL;
    }

    printf(">>> INICIANDO SIMULACION CON %d MEDICOS <<<\n\n", n_docs);

    int tick = 0;
    bool simulacion_activa = true;

    // --- CICLO PRINCIPAL (TICKS) ---
    while(simulacion_activa) {
        
        // A. LLEGADA DE PACIENTES
        while(!colaVacia(pacientes) && pacientes->front->paciente->tiempo_llegada <= tick) {
            patient_t* p = desencolar(pacientes);
            encolar(colas[p->prioridad_actual - 1], p);
            printf("[Tick %03d] LLEGO paciente P%03d (Prio %d).\n", tick, p->id, p->prioridad_actual);
        }

        // B. TRIAGE DINAMICO
        for(int i = 0; i < 4; i++) {
            cola_t *actual = colas[i]->front;
            while(actual != NULL) {
                patient_t *p = actual->paciente;
                actual = actual->next; // Avanzamos el iterador antes de mover al paciente
                
                float chance = generar_random();
                int nueva_prio = p->prioridad_actual;
                
                
                if(chance < p->prob_empeora && p->prioridad_actual > 1) {
                    nueva_prio--; // Empeora (Baja numéricamente)
                } else if (chance > (1.0f - p->prob_mejora) && p->prioridad_actual < 4) {
                    nueva_prio++; // Mejora (Sube numéricamente)
                }
                
                if(nueva_prio != p->prioridad_actual) {
                    // Guardar cambio para posible rollback
                    push(pila_rollback, p->id, p->prioridad_actual);
                    
                    // Extraer de su cola actual
                    patient_t *extraido = extraer_paciente(colas[i], p->id);
                    if(extraido) {
                        printf("[Tick %03d] TRIAGE: P%03d cambia de Prio %d a %d\n", 
                            tick, extraido->id, extraido->prioridad_actual, nueva_prio);
                        extraido->prioridad_actual = nueva_prio;
                        encolar(colas[nueva_prio - 1], extraido);
                    }
                }
            }
        }

        // C. EVENTO DE ROLLBACK (Ejemplo estático en el Tick 50)
        if(tick == 50) {
            printf("\n--- EVENTO: APLICANDO ROLLBACK A 2 PACIENTES ---\n");
            for(int r = 0; r < 2; r++) {
                registro_cambio_t retroceso = pop(pila_rollback);
                if(retroceso.id_paciente != -1) {
                    patient_t* p_revertir = NULL;
                    // Buscar al paciente en todas las colas
                    for(int i = 0; i < 4; i++) {
                        p_revertir = extraer_paciente(colas[i], retroceso.id_paciente);
                        if(p_revertir) break;
                    }
                    if(p_revertir) {
                        printf("[Tick %03d] ROLLBACK: P%03d restaurado a Prio %d\n", tick, p_revertir->id, retroceso.prioridad_anterior);
                        p_revertir->prioridad_actual = retroceso.prioridad_anterior;
                        encolar(colas[retroceso.prioridad_anterior - 1], p_revertir);
                    }
                }
            }
            printf("------------------------------------------------\n\n");
        }

        // D. FINALIZACIÓN DE ATENCIONES MÉDICAS
        for(int m = 0; m < n_docs; m++) {
            if(medicos[m].occu) {
                medicos[m].curr->tiempo_restante--;
                if(medicos[m].curr->tiempo_restante <= 0) {
                    printf("[Tick %03d] Medico %d TERMINO de atender a P%03d.\n", tick, m+1, medicos[m].curr->id);
                    insertar_historial(registro_historico, medicos[m].curr, tick);
                    medicos[m].occu = false;
                    medicos[m].curr = NULL;
                }
            }
        }

        // E. ASIGNACIÓN DE MÉDICOS DISPONIBLES
        for(int m = 0; m < n_docs; m++) {
            if(!medicos[m].occu) {
                // Prioridad 1 a 4
                for(int prio = 0; prio < 4; prio++) {
                    if(!colaVacia(colas[prio])) {
                        medicos[m].curr = desencolar(colas[prio]);
                        medicos[m].occu = true;
                        printf("[Tick %03d] Medico %d EMPIEZA atencion de P%03d (Prio %d).\n", tick, m+1, medicos[m].curr->id, medicos[m].curr->prioridad_actual);
                        break;
                    }
                }
            }
        }

        // F. ACTUALIZAR TIEMPOS DE ESPERA
        for(int i = 0; i < 4; i++) {
            cola_t *actual = colas[i]->front;
            while(actual != NULL) {
                actual->paciente->tiempo_espera++;
                actual = actual->next;
            }
        }

        // G. EVALUAR FIN DE SIMULACIÓN
        bool docs_trabajando = false;
        for(int m = 0; m < n_docs; m++) {
            if(medicos[m].occu) docs_trabajando = true;
        }

        if(colaVacia(pacientes) && colasEsperaVacias(colas, 4) && !docs_trabajando) {
            simulacion_activa = false;
        } else {
            tick++;
        }
    }

    printf("\n>>> SIMULACION FINALIZADA EN EL TICK %d <<<\n", tick);

    // Generar métricas finales
    imprimir_metricas(registro_historico);

    // --- LIBERACIÓN DE MEMORIA OBLIGATORIA ---
    for (int i = 0; i < 4; i++){
        free(colas[i]);
    }
    free(pacientes);
    free(medicos);
    free(pila_rollback);
    liberar_historial(registro_historico);
    return 0;
}

/* ==========================================
   4. DEFINICIÓN DE FUNCIONES
   ========================================== */

// --- Utilidades ---
float generar_random() {
    return (float)rand() / (float)RAND_MAX;
}

// --- Colas (Tus funciones originales + Extraer) ---
espera_t* crearColaEspera(){
    espera_t* cola = (espera_t *)malloc(sizeof(espera_t));
    if (cola == NULL) return NULL;
    cola->front = cola->final = NULL;
    return cola;
}

void encolar(espera_t * cola, patient_t *paciente){
    cola_t *nuevo = (cola_t *)malloc(sizeof(cola_t));
    if(nuevo == NULL) return;
    nuevo->paciente = paciente;
    nuevo->next = NULL;

    if(cola->final == NULL) {
        cola->front = nuevo;
        cola->final = nuevo;
    } else {
        cola->final->next = nuevo;
        cola->final = nuevo;
    }
}

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
    
    cola_t *actual = cola->front; // primero -> segundo
    cola_t *anterior = NULL;

    while (actual != NULL && actual->paciente->id != id) {
        anterior = actual;
        actual = actual->next;
    }

    if (actual == NULL) return NULL;

    patient_t *p = actual->paciente;

    if (anterior == NULL) { // Es el primero de la fila
        cola->front = actual->next;
        if (cola->front == NULL) cola->final = NULL;
    } else { // Está en el medio o final
        anterior->next = actual->next;
        if (actual->next == NULL) cola->final = anterior;
    }
    
    free(actual);
    return p;
}

bool colasEsperaVacias(espera_t** colas, int n) {
    for (int i = 0; i < n; ++i) {
        if (colas[i]->front != NULL) return false;
    }
    return true;
}

bool colaVacia(espera_t* cola) {
    return (cola->front == NULL);
}

int cargar_pacientes(espera_t* pacientes, const char *filename){
    FILE *file = fopen(filename, "r");
    if (file == NULL) return -1;
    
    int cant_docs, cant_pacientes;
    fscanf(file, "%d", &cant_docs);
    fscanf(file, "%d", &cant_pacientes);

    for(int i = 0; i < cant_pacientes; i++){
        patient_t* p = (patient_t *)malloc(sizeof(patient_t));
        if (p == NULL) { fclose(file); return -1; }
        
        fscanf(file, " P%d;%d;%d;%d;%d;%f;%f", &p->id, &p->edad, &p->prioridad_original, &p->tiempo_llegada, &p->tiempo_atencion, &p->prob_empeora, &p->prob_mejora);
        
        p->prioridad_actual = p->prioridad_original;
        p->tiempo_espera = 0;
        p->tiempo_restante = p->tiempo_atencion;
        encolar(pacientes, p);
    }
    fclose(file);
    return cant_docs;
}

void imprimir_pacientes(espera_t *cola) {
    if (cola == NULL || colaVacia(cola)) {
        printf("La cola de pacientes esta vacia.\n");
        return;
    }
    cola_t *actual = cola->front;
    printf("\n--- LISTA DE PACIENTES CARGADOS ---\n");
    printf("ID\tEdad\tPrio\tLlegada\tAtencion\tP.Empeora\tP.Mejora\n");
    printf("------------------------------------------------------------------------\n");
    while (actual != NULL) {
        patient_t *p = actual->paciente; 
        printf("P%03d\t%d\t%d\t%d\t%d\t\t%.2f\t\t%.2f\n", p->id, p->edad, p->prioridad_original, p->tiempo_llegada, p->tiempo_atencion, p->prob_empeora, p->prob_mejora);
        actual = actual->next;
    }
    printf("------------------------------------------------------------------------\n\n");
}

// --- Pilas (Tus funciones originales + Inicializador) ---
pila_t* crearPila() {
    pila_t* p = (pila_t*)malloc(sizeof(pila_t));
    if (p) p->tope = NULL;
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

// --- Historial Doblemente Enlazado (Métricas) ---
historial_t* crearHistorial() {
    historial_t *h = (historial_t *)malloc(sizeof(historial_t));
    if(h) h->cabeza = h->cola = NULL;
    return h;
}
[]-[]-[]-[]-NULL
void liberar_historial(historial_t *registro) {
    nodo_historial_t *actual = registro->cabeza;
    while (actual != NULL) {
        nodo_historial_t *temp = actual;
        actual = actual->next;
        free(temp);
    }
    free(registro);
}

void insertar_historial(historial_t *h, patient_t *p, int tick) {
    nodo_historial_t *nuevo = (nodo_historial_t *)malloc(sizeof(nodo_historial_t));
    nuevo->paciente = p;
    nuevo->tick_salida = tick;
    nuevo->next = NULL;
    nuevo->prev = h->cola;

    if(h->cola == NULL) h->cabeza = nuevo;
    else h->cola->next = nuevo;
    h->cola = nuevo;
}

void imprimir_metricas(historial_t *h) {
    if(h->cabeza == NULL) return;

    int sum_espera[4] = {0};
    int conteo_prio[4] = {0};
    int max_espera[4] = {0};
    int criticos_a_tiempo = 0;
    int umbral = 15; // Límite de tiempo aceptable para prioridad 1

    nodo_historial_t *actual = h->cabeza;
    printf("\n--- REPORTE FINAL DE METRICAS ---\n");
    
    while(actual != NULL) {
        patient_t *p = actual->paciente;
        int idx = p->prioridad_original - 1;

        sum_espera[idx] += p->tiempo_espera;
        conteo_prio[idx]++;

        if(p->tiempo_espera > max_espera[idx]) {
            max_espera[idx] = p->tiempo_espera;
        }

        if(p->prioridad_original == 1 && p->tiempo_espera <= umbral) {
            criticos_a_tiempo++;
        }
        actual = actual->next;
    }

    float prom;
    for(int i = 0; i < 4; i++) {
        if (conteo_prio[i] > 0) {
            prom = (float)(sum_espera[i] / conteo_prio[i]);
        } else prom = 0.0;
        printf("Prioridad %d | Atendidos: %d | Espera Promedio: %.2f ticks | Maximo tiempo de espera: %d ticks\n", i + 1, conteo_prio[i], prom, max_espera[i]);
    }
    
    float porcentaje_criticos;

    if (conteo_prio[0] > 0) {
        porcentaje_criticos = (float)(criticos_a_tiempo / conteo_prio[0]) * 100.0;
    } else porcentaje_criticos = 0;

    printf("\nPacientes Cr\xA1ticos (Prio 1) atendidos en <= %d ticks: %.1f%%\n", umbral, porcentaje_criticos);
}