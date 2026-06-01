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

typedef struct doc {
    int id_medico;
    bool occu;
    struct patient *curr;
} doc_t;

/* Colas de espera */
typedef struct cola {
    patient_t *paciente;
    struct cola *next;
} cola_t;

typedef struct espera {
    cola_t *front;
    cola_t *final;
} espera_t;

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

/* FUNCION MAIN */
int main(int argc, char *argv[]) {
    if  (argc < 2) {
        printf("\033[1;33m:: SISTEMA DE GESTI\xe0N DE URGENCIAS HOSPITALARIAS ::\033[0m\n");
        printf("\033[1;32m:: > Uso correcto:\033[1;37m %s \033[1;33m<nombre_archivo.txt>\033[0m\n", argv[0]);
        return 1;
    }

    srand((unsigned int)time(NULL));
    /* Inicializamos estructuras */
    espera_t* pacientes_llegada = crearColaEspera();
    lista_colas_t* gestor_colas = crearListaColas();
    pila_t* pila_rollback = crearPila();
    historial_t* registro_historico = crearHistorial();
    
    int n_docs = cargar_pacientes(pacientes_llegada, argv[1]);
    if  (n_docs == -1) {
        printf("\033[1;31mHubo un error critico al cargar los pacientes.\033[0m\n");
        liberar_cola(pacientes_llegada);
        liberar_lista_colas(gestor_colas);
        liberar_pila(pila_rollback);
        liberar_historial(registro_historico);
        return 1; 
    }

    lista_docs_t* gestor_medicos = crearListaMedicos(n_docs);

    printf(".:: Iniciando  %d MEDICOS <<<\n\n", n_docs);

    int tick = 0;
    bool simulacion_activa = true;

    /* Ciclo principal de simulacion */
    while(simulacion_activa) {
        
        /* Llegada de nuevos pacientes */
        while(!colaVacia(pacientes_llegada) && pacientes_llegada->front->paciente->tiempo_llegada <= tick) {
            patient_t* p = desencolar(pacientes_llegada);
            espera_t* cola_dest = obtener_cola_prioridad(gestor_colas, p->prioridad_actual);
            encolar(cola_dest, p);
            printf("[Tick %03d] LLEGO paciente P%03d (Prio %d).\n", tick, p->id, p->prioridad_actual);
        }

        /* Evaluar triage (reubicacion) */
        nodo_lista_cola_t* nodo_c = gestor_colas->cabeza;
        while(nodo_c != NULL) {
            cola_t *actual = nodo_c->cola_espera->front;
            
            while(actual != NULL) {
                patient_t *p = actual->paciente;
                cola_t *siguiente_seguro = actual->next; /* Guardar puntero para iteracion segura */
                
                /* Ignorar pacientes recien reubicados */
                if  (p->ultimo_tick_triage != tick) {
                    float chance = generar_random();
                    int nueva_prio = p->prioridad_actual;
                    
                    if (chance < p->prob_empeora && p->prioridad_actual > 1) {
                        nueva_prio--; 
                    } else if  (chance > (1.0f - p->prob_mejora) && p->prioridad_actual < 4) {
                        nueva_prio++; 
                    }
                    
                    if (nueva_prio != p->prioridad_actual) {
                        push(pila_rollback, p->id, p->prioridad_actual);
                        
                        patient_t *extraido = extraer_paciente(nodo_c->cola_espera, p->id);
                        if (extraido) {
                            extraido->cambios_triage++; /* Contabilizar retriage */
                            extraido->ultimo_tick_triage = tick;
                            printf("[Tick %03d] TRIAGE: P%03d cambia de Prio %d a %d\n", 
                                tick, extraido->id, extraido->prioridad_actual, nueva_prio);
                            
                            extraido->prioridad_actual = nueva_prio;
                            encolar(obtener_cola_prioridad(gestor_colas, nueva_prio), extraido);
                            
                            /* Alerta si el paciente tiene varios retriage */
                            if  (extraido->cambios_triage >= 3) {
                                printf("[Tick %03d] ALERTA: P%03d ha cambiado de prioridad %d veces!\n", tick, extraido->id, extraido->cambios_triage);
                            }
                        }
                    }
                }
                actual = siguiente_seguro;
            }
            nodo_c = nodo_c->next;
        }

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
                    printf("[Tick %03d] ROLLBACK EXITOSO: P%03d restaurado a Prio %d\n", tick, p_revertir->id, retroceso.prioridad_anterior);
                    p_revertir->prioridad_actual = retroceso.prioridad_anterior;
                    encolar(obtener_cola_prioridad(gestor_colas, retroceso.prioridad_anterior), p_revertir);
                } else {
                    /* Verif icar si el paciente fue desencolado */
                    printf("[Tick %03d] ROLLBACK OMITIDO: P%03d ya se encuentra en atencion o finalizado.\n", tick, retroceso.id_paciente);
                }
            }
            printf("----------------------------------------------------------\n\n");
        }

        /* Finalizacion de atenciones medicas */
        nodo_doc_t* iter_med = gestor_medicos->cabeza;
        while (iter_med != NULL) {
            if (iter_med->medico.occu) {
                iter_med->medico.curr->tiempo_restante--;
                if (iter_med->medico.curr->tiempo_restante <= 0) {
                    printf("[Tick %03d] Medico %d TERMINO atencion de P%03d.\n", tick, iter_med->medico.id_medico, iter_med->medico.curr->id);
                    insertar_historial(registro_historico, iter_med->medico.curr, tick);
                    iter_med->medico.occu = false;
                    iter_med->medico.curr = NULL;
                }
            }
            iter_med = iter_med->next;
        }

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
                        printf("[Tick %03d] Medico %d EMPIEZA atencion de P%03d (Prio %d).\n", tick, iter_med->medico.id_medico, iter_med->medico.curr->id, iter_med->medico.curr->prioridad_actual);
                        break; /* Asignado, dejar de buscar */
                    }
                }
            }
            iter_med = iter_med->next;
        }

        /* Actualizar tiempos y alertas */
        nodo_c = gestor_colas->cabeza;
        while(nodo_c != NULL) {
            cola_t *actual = nodo_c->cola_espera->front;
            int cont_acumulados = 0;
            
            while(actual != NULL) {
                actual->paciente->tiempo_espera++;
                cont_acumulados++;
                
                if  (actual->paciente->tiempo_espera == 20) { /* Alerta por espera excesiva */
                    printf("[Tick %03d] ALERTA: P%03d lleva 20 ticks esperando en Prio %d!\n", tick, actual->paciente->id, nodo_c->prioridad_asignada);
                }
                actual = actual->next;
            }
            
            if  (cont_acumulados >= 10 && tick % 5 == 0) { /* Alerta por acumulacion */
                printf("[Tick %03d] ALERTA: Acumulacion de %d pacientes en Prio %d!\n", tick, cont_acumulados, nodo_c->prioridad_asignada);
            }
            nodo_c = nodo_c->next;
        }

        /* Mostrar snapshot del sistema */
        if  (tick > 0 && tick % 10 == 0) {
            imprimir_snapshot(tick, gestor_colas, gestor_medicos);
        }

        /* Condicion de fin de la simulacion */
        bool docs_trabajando = false;
        iter_med = gestor_medicos->cabeza;
        while (iter_med != NULL) {
            if (iter_med->medico.occu) docs_trabajando = true;
            iter_med = iter_med->next;
        }

        if (colaVacia(pacientes_llegada) && todas_colas_vacias(gestor_colas) && !docs_trabajando) {
            simulacion_activa = false;
        } else {
            tick++;
        }
    }

    printf("\n>>> SIMULACION FINALIZADA EN EL TICK %d <<<\n", tick);

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

/* DEFINICIÓN DE FUNCIONES */
float generar_random() {
    return (float)rand() / (float)RAND_MAX;
}

/* Manejo de colas */
espera_t* crearColaEspera(){
    espera_t* cola = (espera_t *)malloc(sizeof(espera_t));
    if  (cola == NULL) return NULL;
    cola->front = cola->final = NULL;
    return cola;
}

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

patient_t *desencolar(espera_t *cola) {
    if  (cola->front == NULL) return NULL;
    cola_t *aux = cola->front;
    patient_t *paciente = aux->paciente;
    cola->front = (cola->front)->next;
    if  (cola->front == NULL) cola->final = NULL;
    free(aux);
    return paciente;
}

patient_t* extraer_paciente(espera_t *cola, int id) {
    if  (cola->front == NULL) return NULL;
    
    cola_t *actual = cola->front; 
    cola_t *anterior = NULL;

    while (actual != NULL && actual->paciente->id != id) {
        anterior = actual;
        actual = actual->next;
    }

    if  (actual == NULL) return NULL;
    patient_t *p = actual->paciente;

    if  (anterior == NULL) { 
        cola->front = actual->next;
        if  (cola->front == NULL) cola->final = NULL;
    } else { 
        anterior->next = actual->next;
        if  (actual->next == NULL) cola->final = anterior;
    }
    
    free(actual);
    return p;
}

bool colaVacia(espera_t* cola) {
    return (cola->front == NULL);
}

/* Vaciar cola liberando pacientes */
void liberar_cola(espera_t* cola) {
    while (!colaVacia(cola)) {
        patient_t* p = desencolar(cola);
        free(p); 
    }
    free(cola);
}

/* Listas de manejo dinamico */
lista_colas_t* crearListaColas() {
    lista_colas_t* lista = (lista_colas_t*)malloc(sizeof(lista_colas_t));
    lista->cabeza = NULL;
    /* Inicializar prioridades invertidas para mantener orden */
    for  (int i = 4; i >= 1; i--) {
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

lista_docs_t* crearListaMedicos(int n) {
    lista_docs_t* lista = (lista_docs_t*)malloc(sizeof(lista_docs_t));
    lista->cabeza = NULL;
    /* Lista de medicos (insercion hacia atras) */
    for  (int i = n; i >= 1; i--) {
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
        /* Liberar paciente en atencion */
        if  (temp->medico.curr != NULL) free(temp->medico.curr);
        free(temp);
    }
    free(lista);
}

/* Carga desde archivo */
int cargar_pacientes(espera_t* pacientes, const char *filename){
    FILE *file = fopen(filename, "r");
    if  (file == NULL) return -1;
    
    int cant_docs, cant_pacientes;
    
    /* Comprobar lectura correcta */
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
        if  (p == NULL) { fclose(file); return -1; }
        
        int leidos = fscanf(file, " P%d;%d;%d;%d;%d;%f;%f", 
            &p->id, &p->edad, &p->prioridad_original, 
            &p->tiempo_llegada, &p->tiempo_atencion, 
            &p->prob_empeora, &p->prob_mejora);
        
        if  (leidos != 7) {
            printf("Error: for mato corrupto en el archivo. Paciente %d.\n", i+1);
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

/* Manejo de pila */
pila_t* crearPila() {
    pila_t* p = (pila_t*)malloc(sizeof(pila_t));
    if  (p) p->tope = NULL;
    return p;
}

void push(pila_t* pila, int id_paciente, int prioridad_anterior) {
    nodo_pila_t* nuevo_nodo = (nodo_pila_t*)malloc(sizeof(nodo_pila_t));
    if  (nuevo_nodo == NULL) return;
    nuevo_nodo->datos.id_paciente = id_paciente;
    nuevo_nodo->datos.prioridad_anterior = prioridad_anterior;
    nuevo_nodo->siguiente = pila->tope;
    pila->tope = nuevo_nodo;
}

registro_cambio_t pop(pila_t* pila) {
    registro_cambio_t value = {-1, -1};
    if  (pila->tope == NULL) return value;
    
    nodo_pila_t* aux = pila->tope;
    value = aux->datos;
    pila->tope = pila->tope->siguiente;
    free(aux);
    return value;
}

/* Vaciar y liberar la pila */
void liberar_pila(pila_t* pila) {
    while (pila->tope != NULL) {
        pop(pila); 
    }
    free(pila);
}

/* Historial y metricas */
historial_t* crearHistorial() {
    historial_t *h = (historial_t *)malloc(sizeof(historial_t));
    if (h) h->cabeza = h->cola = NULL;
    return h;
}

/* Liberar nodos junto al paciente */
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

    for (int i = 0; i < 4; i++) {
        /* Casteo a float antes de dividir */
        float prom = (conteo_prio[i] > 0) ? (float)sum_espera[i] / (float)conteo_prio[i] : 0.0f;
        printf("Prioridad %d | Atendidos: %d | Espera Promedio: %.2f ticks | Maxima: %d ticks\n", i + 1, conteo_prio[i], prom, max_espera[i]);
    }
    
    float porcentaje_criticos = (conteo_prio[0] > 0) ? ((float)criticos_a_tiempo / (float)conteo_prio[0]) * 100.0f : 0.0f;
    /* Porcentaje de pacientes criticos atendidos */
    printf("\nPacientes Criticos (Prio 1) atendidos en <= %d ticks: %.1f%%\n", umbral, porcentaje_criticos);
}

/* Recorrido en reversa para la lista doble */
void imprimir_historial_inverso(historial_t *h) {
    if  (h->cola == NULL) return;
    
    printf("\n--- HISTORIAL DE ATENCION (Mas recientes primero) ---\n");
    nodo_historial_t *actual = h->cola; /* Iniciar en la cola */
    while (actual != NULL) {
        patient_t *p = actual->paciente;
        printf("Alta en Tick %03d -> P%03d (Prio Orig: %d | Prio Final: %d | Espera: %d ticks)\n", 
               actual->tick_salida, p->id, p->prioridad_original, p->prioridad_actual, p->tiempo_espera);
        actual = actual->prev; /* Desplazarse por prev */
    }
    printf("-------------------------------------------------------\n");
}

/* Snapshot de urgencias */
void imprimir_snapshot(int tick, lista_colas_t* l_colas, lista_docs_t* l_docs) {
    printf("\n==== [SNAPSHOT TICK %03d] ====\n", tick);
    
    /* Listado medicos */
    nodo_doc_t* doc = l_docs->cabeza;
    while(doc != NULL) {
        if (doc->medico.occu) printf("  Medico %d: Atendiendo a P%03d (Restante: %d)\n", doc->medico.id_medico, doc->medico.curr->id, doc->medico.curr->tiempo_restante);
        else printf("  Medico %d: Libre\n", doc->medico.id_medico);
        doc = doc->next;
    }
    
    /* Manejo de colas */
    nodo_lista_cola_t* c = l_colas->cabeza;
    while(c != NULL) {
        int conteo = 0;
        cola_t* p = c->cola_espera->front;
        while(p != NULL) { conteo++; p = p->next; }
        printf("  Fila Prio %d: %d en espera\n", c->prioridad_asignada, conteo);
        c = c->next;
    }
    printf("=============================\n\n");
}