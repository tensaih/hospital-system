# 🏥 Sistema de Gestión Hospitalaria

¡Bienvenido al proyecto de **Sistema de Gestión Hospitalaria**! Este proyecto es una simulación programada en **C** diseñada para administrar la llegada, prioridad y atención de pacientes en un entorno médico.

---

## 📂 Archivos Principales

El proyecto se divide actualmente en dos módulos clave: un generador de datos para pruebas y el núcleo de la simulación.

### 1. `lector.c` - *Generador de Pacientes* 💉
Este programa actúa como una herramienta de configuración inicial. Se encarga de crear un entorno de pruebas generando aleatoriamente un listado de pacientes y exportándolo a un archivo de texto (`pacientes.txt`). 

**Características:**
- 🧑‍⚕️ Solicita interactivamente la cantidad de **médicos** y **pacientes** al usuario.
- 🎲 Utiliza funciones de generación aleatoria (`srand`, `rand`) para simular condiciones realistas de un hospital.
- 📝 Genera de forma automática los siguientes datos por cada paciente:
  - **Edad:** Entre 1 y 100 años.
  - **Prioridad inicial:** Nivel de 1 a 4.
  - **Tiempo de llegada:** Incremento progresivo (0 a 4 minutos entre la llegada de cada paciente).
  - **Tiempo de atención:** De 1 a 15 minutos estimados.
  - **Probabilidad de empeorar:** Entre 10% y 30%.
  - **Probabilidad de mejorar:** Entre 20% y 50%.
- 💾 Guarda los resultados en `pacientes.txt` utilizando un formato estandarizado delimitado por punto y coma (`;`), facilitando su posterior lectura.

---

### 2. `programa.c` - *Motor de Simulación (En desarrollo)* ⚙️
Es el sistema principal. Su propósito es leer los datos generados por `lector.c` y organizarlos en memoria dinámica mediante la implementación de **Colas (Queues)** como Tipos Abstractos de Datos.

**Estructuras de Datos (TADs) Implementadas:**
| Estructura | Descripción |
| :--- | :--- |
| `patient_t` | Almacena toda la información vital del paciente (ID, edad, tiempos, prioridades y probabilidades). |
| `doc_t` | Representa a un médico, indicando si está ocupado (`occu`) y qué paciente atiende actualmente. |
| `cola_t` | Nodo de la lista enlazada que contiene un puntero a los datos del paciente. |
| `espera_t` | Estructura de control de la cola, manteniendo los punteros al inicio (`front`) y al final (`final`). |

**Flujo de Funciones Actuales:**
- 🟢 `crearColaEspera()`: Asigna memoria e inicializa una nueva cola de espera vacía.
- 📥 `cargas_pacientes()`: Abre `pacientes.txt`, extrae la cantidad de médicos/pacientes, parsea la información individual y los inserta en la cola general.
- ➕ `encolar()`: Inserta un paciente nuevo al final de la cola mediante enlaces de punteros.
- 🖨️ `imprimir_pacientes()`: Recorre la cola y muestra en la terminal una tabla tabulada y limpia con todos los pacientes cargados.

> [!NOTE]
> **Estado del Módulo (`programa.c`):** 
> Actualmente, el programa carga exitosamente la información desde el archivo, crea las estructuras en memoria y permite visualizar a los pacientes en consola. La lógica de asignación a los médicos, la gestión de colas múltiples por prioridad y las variaciones de salud se encuentran **en desarrollo**.

---

## 🚀 Compilación y Ejecución

Para compilar y poner a prueba los archivos, asegúrate de tener `gcc` instalado y ejecuta los siguientes comandos en tu terminal:

```bash
# 1. Compilar y ejecutar el generador de datos (creará pacientes.txt)
gcc lector.c -o lector
./lector

# 2. Compilar y ejecutar la simulación de colas
gcc programa.c -o programa
./programa
```