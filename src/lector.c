#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MIN_EDAD 1
#define MAX_EDAD 100

#define MIN_PRIORIDAD 1
#define MAX_PRIORIDAD 4

#define MIN_PROB_MEJORAR 0.05
#define MAX_PROB_MEJORAR 0.10

#define MIN_PROB_EMPEORAR 0.15
#define MAX_PROB_EMPEORAR 0.20

#define MIN_TIEMPO_ATENCION 1
#define MAX_TIEMPO_ATENCION 15

int main() {

    int cant_medicos, cant_pacientes;
    int 
        i, 
        edad, 
        prioridad,
        tiempo_llegada = 0,
        tiempo_atencion;

    float 
        prob_empeorar,
        prob_mejorar;
    
    printf("Ingrese la cantidad de médicos: ");
    scanf("%d", &cant_medicos);
    printf("Ingrese la cantidad de pacientes: ");
    scanf("%d", &cant_pacientes);

    FILE * file;
    file = fopen("bin/pacientes.txt", "w");
    if (file == NULL) {
        printf("Error al abrir el archivo.\n");
        return 1;
    }
    
    /* Cantidad medicos */
    fprintf(file, "%d\n", cant_medicos);
    /* Cantidad de pacientes */
    fprintf(file, "%d\n", cant_pacientes);
    
    srand((unsigned int)time(NULL));
    int rango_empeorar = (int)((MAX_PROB_EMPEORAR - MIN_PROB_EMPEORAR) * 100 + 1);
    int rango_mejorar = (int)((MAX_PROB_MEJORAR - MIN_PROB_MEJORAR) * 100 + 1);

    for (i = 0; i < cant_pacientes; i++) {
        /* Edad entre 1 y 100 */
        edad = rand() % (MAX_EDAD - MIN_EDAD + 1) + MIN_EDAD;
        
        /* Prioridad entre 1 y 4 */
        prioridad = rand() % (MAX_PRIORIDAD - MIN_PRIORIDAD + 1) + MIN_PRIORIDAD;
        
        /* Tiempo de atención entre 1 y 15 minutos */
        tiempo_atencion = MIN_TIEMPO_ATENCION + (rand() % (MAX_TIEMPO_ATENCION - MIN_TIEMPO_ATENCION + 1)); 
        
        /* Probabilidades de empeorar y mejorar */
        prob_empeorar = MIN_PROB_EMPEORAR + ((rand() % rango_empeorar) / 100.0);
        prob_mejorar = MIN_PROB_MEJORAR + ((rand() % rango_mejorar) / 100.0);

        /* Escribir los datos del paciente en el archivo */
        if (i == cant_pacientes - 1) {
            fprintf(
                file, 
                "P%03d;%d;%d;%d;%d;%.2f;%.2f", 
                i + 1, edad, prioridad, tiempo_llegada, tiempo_atencion, prob_empeorar, prob_mejorar
            );
        } else {
            fprintf(
                file, 
                "P%03d;%d;%d;%d;%d;%.2f;%.2f\n", 
                i + 1, edad, prioridad, tiempo_llegada, tiempo_atencion, prob_empeorar, prob_mejorar
            );
        }
        
        /* Incrementar el tiempo de llegada para el próximo paciente */
        tiempo_llegada += rand() % 5;
    }
    fclose(file);
    printf("Archivo 'pacientes.txt' generado con éxito.\n");
    return 0;
}