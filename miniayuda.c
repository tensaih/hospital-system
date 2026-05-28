int cargar_pacientes(Cola* pacientes, const char* filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: No se pudo abrir el archivo %s\n", filename);
        return -1;
    }

    int cant_medicos, cant_pacientes;

    fscanf(file, "%d", &cant_medicos);
    fscanf(file, "%d", &cant_pacientes);

    for (int i = 0; i < cant_pacientes; i++) {
        Paciente* p = (Paciente*)malloc(sizeof(Paciente));
        if (p == NULL) {
            printf("Error: No se pudo asignar memoria para el paciente %d\n", i);
            fclose(file);
            return -1;
        }
        fscanf(
            file, "P%d;%d;%d;%d;%d;%f;%f", 
            &p->id, 
            &p->edad, 
            &p->prioridad_original, 
            &p->tiempo_llegada, 
            &p->duracion_atencion, 
            &p->prob_empeorar, 
            &p->prob_mejorar
        );

        p->prioridad_actual = p->prioridad_original;
        p->tiempo_espera = 0;
        p->tiempo_atencion_restante = p->duracion_atencion;

        encolar(pacientes, p);
    }

    fclose(file);
    printf("Archivo '%s' cargado con exito. %d pacientes esperando ingresar.\n", filename, cant_pacientes);
    return cant_medicos; 
}