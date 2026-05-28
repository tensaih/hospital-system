#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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
    patient_t *paciente; // Apuntador al dato del paciente
    struct cola *next;   // Apuntador al siguiente nodo
} cola_t;

typedef struct espera {
    cola_t *front;
    cola_t *final;
} espera_t;

espera_t* crearColaEspera();
int cargas_pacientes(espera_t *, const char *);
void encolar(espera_t *, patient_t*);
void imprimir_pacientes(espera_t *);

int main(){
    espera_t* colas[4];
    for (int i = 0; i < 4; i++){
        colas[i] = crearColaEspera();
    }
    espera_t* pacientes;
    pacientes = crearColaEspera();
    
    int n_docs = cargas_pacientes(pacientes, "pacientes.txt");

    imprimir_pacientes(pacientes);
    
    for (int i = 0; i < 4; i++){
        free(colas[i]);
    }
    free(pacientes);
    return 0;
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

void imprimir_pacientes(espera_t *cola) {
    // Verificamos que la cola exista y tenga al menos un paciente
    if (cola == NULL || cola->front == NULL) {
        printf("La cola de pacientes esta vacia.\n");
        return;
    }

    // Creamos un puntero temporal para "caminar" por la fila sin perder el frente
    cola_t *actual = cola->front;
    
    printf("\n--- LISTA DE PACIENTES CARGADOS ---\n");
    printf("ID\tEdad\tPrio\tLlegada\tAtencion\tP.Empeora\tP.Mejora\n");
    printf("------------------------------------------------------------------------\n");

    // Mientras no lleguemos al final de la fila (NULL)
    while (actual != NULL) {
        patient_t *p = actual->paciente; // Extraemos el paciente del nodo actual
        
        // Imprimimos sus datos
        printf("P%03d\t%d\t%d\t%d\t%d\t\t%.2f\t\t%.2f\n", 
               p->id, 
               p->edad, 
               p->prioridad_original, 
               p->tiempo_llegada, 
               p->tiempo_atencion, 
               p->prob_empeora, 
               p->prob_mejora);
        
        // Avanzamos al siguiente nodo en la fila
        actual = actual->next;
    }
    printf("------------------------------------------------------------------------\n\n");
}