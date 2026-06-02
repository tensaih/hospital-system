/*
 * Generador de archivo de entrada de pacientes para el sistema de gestión de urgencias hospitalarias.
 * Autores: Iván A.Maldonado Rodriguez, Carlos M. Navarro Villar
 * Fecha: 02-06-2026
 * Docente: Mg. Francisco P. Vásquez Iglesias
 * Descripción:
 *  Este programa genera un archivo de texto con datos aleatorios de pacientes, 
 *  siguiendo el formato requerido por el sistema de gestión de urgencias hospitalarias.
 * */
/* BIBLIOTECAS */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* DEFINICION DE CONSTANTES */
#define MIN_EDAD 1
#define MAX_EDAD 100

#define MIN_PRIORIDAD 1
#define MAX_PRIORIDAD 4

#define MIN_PROB_MEJORAR 0.02
#define MAX_PROB_MEJORAR 0.10

#define MIN_PROB_EMPEORAR 0.15
#define MAX_PROB_EMPEORAR 0.20

#define MIN_TIEMPO_ATENCION 1
#define MAX_TIEMPO_ATENCION 15

/* ESTRUCTURA DATOS PACIENTE */
typedef struct {
    int id;
    int edad;
    int prioridad;
    int tiempo_llegada;
    int tiempo_atencion;
    float prob_empeorar;
    float prob_mejorar;
} Paciente;

/* DECLARACION DE FUNCIONES */
void solicitar_datos(int *cant_medicos, int *cant_pacientes);
int generar_archivo_pacientes(const char *ruta_archivo, int cant_medicos, int cant_pacientes);
Paciente generar_paciente_aleatorio(int id_paciente, int *tiempo_llegada_acumulado);

/* FUNCION MAIN */
int main(int argc, char *argv[]) {
    int cant_medicos, cant_pacientes;
    const char *ruta_archivo;
    printf("..:: GENERADOR DE ARCHIVO DE PACIENTES ::..\n");
    if (argc == 1) {
        ruta_archivo = "pacientes.txt";
        printf("::: No se proporciono un nombre/ruta de archivo, se generara como '%s' por defecto.\n", ruta_archivo);
    } else if (argc == 2) {
        ruta_archivo = argv[1];
    }else {
        printf("::: ERROR: Se proporcionó una cantidad incorrecta de argumentos.\n");
        printf("::: Modo de uso: %s <ruta_del_archivo_salida>\n", argv[0]);
        printf("::: Ejemplo: %s pacientes.txt\n", argv[0]);
        return 0;
    }

    /* Solicitar cantidades de médicos y pacientes al usuario */
    solicitar_datos(&cant_medicos, &cant_pacientes);

    if (generar_archivo_pacientes(ruta_archivo, cant_medicos, cant_pacientes)) {
        printf("::: Archivo '%s' generado con exito.\n", ruta_archivo);
    } else {
        printf("::: ERROR: No se pudo generar el archivo.\n");
        return 0;
    }

    return 0;
}

/* DEFINICION DE FUNCIONES */
void solicitar_datos(int *cant_medicos, int *cant_pacientes) {
    printf(":: Ingrese la cantidad de medicos\n:: > ");
    scanf("%d", cant_medicos);
    printf(":: Ingrese la cantidad de pacientes\n:: > ");
    scanf("%d", cant_pacientes);
}

Paciente generar_paciente_aleatorio(int id_paciente, int *tiempo_llegada_acumulado) {
    Paciente p;
    int rango_empeorar;
    int rango_mejorar;

    rango_empeorar = (int)((MAX_PROB_EMPEORAR - MIN_PROB_EMPEORAR) * 100 + 1);
    rango_mejorar = (int)((MAX_PROB_MEJORAR - MIN_PROB_MEJORAR) * 100 + 1);

    p.id = id_paciente;
    p.edad = rand() % (MAX_EDAD - MIN_EDAD + 1) + MIN_EDAD;
    p.prioridad = rand() % (MAX_PRIORIDAD - MIN_PRIORIDAD + 1) + MIN_PRIORIDAD;
    p.tiempo_atencion = MIN_TIEMPO_ATENCION + (rand() % (MAX_TIEMPO_ATENCION - MIN_TIEMPO_ATENCION + 1));
    p.prob_empeorar = MIN_PROB_EMPEORAR + ((rand() % rango_empeorar) / 100.0);
    p.prob_mejorar = MIN_PROB_MEJORAR + ((rand() % rango_mejorar) / 100.0);
    
    /* Asignamos el tiempo de llegada actual y luego sumamos el incremento */
    p.tiempo_llegada = *tiempo_llegada_acumulado;
    *tiempo_llegada_acumulado += rand() % 5; 

    return p;
}

int generar_archivo_pacientes(const char *ruta_archivo, int cant_medicos, int cant_pacientes) {
    FILE *file;
    int tiempo_llegada_actual;
    int i;
    
    file = fopen(ruta_archivo, "w");
    if (file == NULL) {
        return 0;
    }
    
    fprintf(file, "%d\n", cant_medicos);
    fprintf(file, "%d\n", cant_pacientes);
    
    srand((unsigned int)time(NULL));
    tiempo_llegada_actual = 0;

    /* Generar pacientes segun cantidad ingresada con formato */
    for (i = 0; i < cant_pacientes; i++) {
        Paciente p;
        const char *formato_linea;

        p = generar_paciente_aleatorio(i + 1, &tiempo_llegada_actual);

        if (i == cant_pacientes - 1) {
            formato_linea = "P%03d;%d;%d;%d;%d;%.2f;%.2f";
        } else {
            formato_linea = "P%03d;%d;%d;%d;%d;%.2f;%.2f\n";
        }

        /* Guardado de datos del paciente en el archivo */
        fprintf(file, formato_linea, 
            p.id, p.edad, p.prioridad, p.tiempo_llegada, p.tiempo_atencion, p.prob_empeorar, p.prob_mejorar
        );
    }

    fclose(file);
    return 1;
}