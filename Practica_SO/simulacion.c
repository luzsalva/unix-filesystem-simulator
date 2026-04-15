// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López
#include "simulacion.h"

// Variable global para llevar la cuenta del número de processos finalizados
static int acabados = 0;

/**
Función: Crea unos procesos de prueba que accedan de forma 
concurrente al sistema de ficheros
Parámetros:
    - argc: Número de argumentos pasados por Terminal
    - **argv: Argumentos pasados al programa
Salida: EXITO/FALLO
Llamado por: N/A (Por Terminal)
Llama a: 
    - bmount()
    - mi_creat()
    - mi_write()
    - bumount()
**/
int main(int argc, char *argv[]) {
    // En el main() asociar la señal SIGCHLD al enterrador
    signal(SIGCHLD, reaper);

    // Comprobar la sintaxis del comando.  // uso: ./simulacion <disco>
    if (argc != 2) {
        fprintf(stderr, RED "Uso: %s <disco>\n" RESET, argv[0]);
        return FALLO;
    }
    
    // Montar el dispositivo virtual del padre
    char *disco = argv[1];
    if (bmount(disco) == FALLO) return FALLO;

    // Crear el directorio de simulación /simul_aaaammddhhmmss/
    char simul_dir[64];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(simul_dir, sizeof(simul_dir), "/simul_%Y%m%d%H%M%S/", t);
    
    if (mi_creat(simul_dir, 6) == FALLO) {
        bumount();
        return FALLO;
    }; 

    for (int proceso = 1; proceso <= NUMPROCESOS; proceso++) {
        int pid=fork();
        if (pid == 0) { // Es el hijo
            int pidHijo = getpid();

            char dir [100];
            snprintf(dir, sizeof(dir), "%sproceso_%d/", simul_dir, pidHijo);
            // Montamos el disco y creamos el directorio del proceso hijo
            if (bmount(disco) == FALLO) return FALLO;
            if (mi_creat(dir, 6) == FALLO) return FALLO; 
            strcat(dir, "prueba.dat");
            if (mi_creat(dir, 6) == FALLO) return FALLO; 
            
            // Inicializamos la semilla de números aleatorios
            srand(time(NULL) + getpid());
            for (int nescritura = 1; nescritura <= NUMESCRITURAS; nescritura++) {
                // Inicializamos el struct registro 
                struct REGISTRO registro;
                registro.fecha=time(NULL);
                registro.pid = getpid();
                registro.nEscritura = nescritura;
                registro.nRegistro = rand() % REGMAX; //[0, 499.999]
                if (mi_write(dir, &registro, registro.nRegistro * sizeof(struct REGISTRO), sizeof(struct REGISTRO)) == FALLO) return FALLO;
#if DEBUGN12 == 1                
                fprintf(stderr, "[simulación.c → Escritura %d en %s]\n", nescritura, dir);
#endif
                // Esperamos 0.05 segundos antes de hacer la siguiente escritura
                usleep(50000);
            }
            // Desmontamos el dispositivo hijo
            if (bumount(disco) == FALLO) return FALLO;
#if DEBUGN12 == 1
        fprintf(stderr, "[Proceso %d: Completadas %d escrituras en %s]\n", proceso, NUMESCRITURAS, dir);   
#endif
            exit(0); //Necesario para que se emita la señal SIGCHLD
        }
        // Esperamos 0.15 para lanzar el siguiente proceso
        usleep(150000);
    }
    while (acabados < NUMPROCESOS) {
        pause();
    }

    // Desmontar el dispositivo del padre
    if (bumount(disco) == FALLO) return FALLO;
    return EXITO;
}

void reaper() {
    pid_t ended;
    signal(SIGCHLD, reaper);
    while ((ended=waitpid(-1, NULL, WNOHANG))>0) {
        acabados++;
    }
}