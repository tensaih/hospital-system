#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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


/* Function declarations */
espera_t* crearColaEspera();
void encolar(espera_t *, patient_t*);
int cargas_pacientes(espera_t *, const char *);
void imprimir_pacientes(espera_t *);

/* Main function */
int main(){
    espera_t* pacientes;
    espera_t* colas[4];

    pacientes = crearColaEspera();

    for (int i = 0; i < 4; i++){
        colas[i] = crearColaEspera();
    }
    
    int n_docs = cargas_pacientes(pacientes, "pacientes.txt");
    if (n_docs == -1){
        printf("Hubo un error al cargar los pacientes.\n");
        return 0;
    }
    imprimir_pacientes(pacientes);
    
    for (int i = 0; i < 4; i++){
        free(colas[i]);
    }
    free(pacientes);
    return 0;
}

/* Functions */
void push(pila_t* pila, int id_paciente, int prioridad_anterior) {
    // crear nodo
    nodo_pila_t* nuevo_nodo = (nodo_pila_t*)malloc(sizeof(nodo_pila_t));
    if (nuevo_nodo == NULL) {
        printf("Error: No se pudo asignar memoria para el nuevo nodo de la pila.\n");
        return;
    }
    // inicializar nodo con los datos del cambio
    nuevo_nodo->datos.id_paciente = id_paciente;
    nuevo_nodo->datos.prioridad_anterior = prioridad_anterior;

    // agregarlo al tope de la pila
    nuevo_nodo->siguiente = pila->tope; // El nuevo nodo apunta al actual tope
    pila->tope = nuevo_nodo; // El nuevo nodo ahora es el tope
}

registro_cambio_t pop(pila_t* pila) {
    registro_cambio_t value = {-1, -1};
    if (pila->tope == NULL) {
        printf("La pila está vacía, no hay cambios para revertir.\n");
        return value;
    }
    nodo_pila_t* aux = pila->tope;
    value = aux->datos;
    pila->tope = pila->tope->siguiente;
    free(aux);
    return value;
}

espera_t* crearColaEspera(){
    espera_t* cola = (espera_t *)malloc(sizeof(espera_t));
    if (cola == NULL){
        return NULL;
    }
    cola->front = cola->final = NULL;
    return cola;
}

int cargas_pacientes(espera_t* pacientes, const char *filename){
    FILE *file = fopen(filename, "r");
    if (file == NULL){
        return -1;
    }
    int cant_docs, cant_pacientes;

    fscanf(file, "%d", &cant_docs);
    fscanf(file, "%d", &cant_pacientes);

    for(int i = 0; i < cant_pacientes; i++){
        patient_t* p = (patient_t *)malloc(sizeof(patient_t));
        if (p == NULL){
            fclose(file);
            return -1;
        }
        fscanf(file, " P%d;%d;%d;%d;%d;%f;%f", &p->id, &p->edad, &p->prioridad_original, &p->tiempo_llegada, &p->tiempo_atencion, &p->prob_empeora, &p->prob_mejora);

        p->prioridad_actual = p->prioridad_original;
        p->tiempo_espera = 0;
        p->tiempo_restante = p->tiempo_atencion;

        encolar(pacientes, p);
    }
    fclose(file);
    return cant_docs;
}

void encolar(espera_t * cola, patient_t *paciente){
    cola_t *nuevo = (cola_t *)malloc(sizeof(cola_t));
    if(nuevo == NULL) 
        return;

    nuevo->paciente = paciente;
    nuevo->next = NULL;

    if(cola->final == NULL) {
        cola->front = nuevo;
        cola->final = nuevo;
    }
    else {
        cola->final->next = nuevo;
        cola->final = nuevo;
    }
}

patient_t *desencolar(espera_t *cola) {
    /* Verificamos que la cola no esté vacía */
    if (cola->front == NULL) {
        return NULL;
    }
    
    cola_t *aux = cola->front;
    patient_t *paciente = aux->paciente;
    
    cola->front = (cola->front)->next;
    if (cola->front == NULL) {
        cola->final = NULL;
    }
    free(aux);
    return paciente;
}

/* Verifica si todas las colas de espera están vacías */
bool colasEsperaVacias(espera_t** colas, int n) {
    for (int i = 0; i < n; ++i) {
        if (colas[i]->front != NULL) {
            return false;
        }
    }
    return true;
}

bool colaVacia(espera_t* cola) {
    return (cola->front == NULL);
}

void imprimir_pacientes(espera_t *cola) {
    if (cola == NULL || colaVacia(cola)) {
        printf("La cola de pacientes esta vacia o aun no ha sido creada.\n");
        return;
    }

    cola_t *actual = cola->front;
    
    printf("\n--- LISTA DE PACIENTES CARGADOS ---\n");
    printf("ID\tEdad\tPrio\tLlegada\tAtencion\tP.Empeora\tP.Mejora\n");
    printf("------------------------------------------------------------------------\n");

    while (actual != NULL) {
        patient_t *p = actual->paciente; 
        
        printf("P%03d\t%d\t%d\t%d\t%d\t\t%.2f\t\t%.2f\n", 
               p->id, 
               p->edad, 
               p->prioridad_original, 
               p->tiempo_llegada, 
               p->tiempo_atencion, 
               p->prob_empeora, 
               p->prob_mejora);
        actual = actual->next;
    }
    printf("------------------------------------------------------------------------\n\n");
}