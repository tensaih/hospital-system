#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>



typedef struct patient {
    int id;
    int edad;
    int prioridad;
    int tiempo_llegada;
    int tiempo_atencion;
    float prob_mejora;
    float prob_empeora;
} patient_t;

typedef struct doc {
    bool occu;
    struct patient *curr;
} doc_t;

typedef struct cola {
    struct patient;
    struct patient *mext;
} cola_t;

typedef struct espera {
    cola_t *front;
    cola_t *final;
} espera_t

espera_t* crearColaEspera();

int main(){
    espera_t* colas[4];
    for (int i = 0; i < 4; i++){
        colas[i] = crearColaEspera();
    }
    free(colas);
    return 0;
}

espera_t* creaColaEspera(){
    espera_t* cola = (espera_t *)malloc(sizeof(espera_t));
    if (cola == NULL){
        return NULL;
    }
    cola->front = cola->final = NULL;
    return cola;
}