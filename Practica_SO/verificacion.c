// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López
#include "verificacion.h"

/**
Función: recorre secuencialmente el fichero "prueba.dat" de cada proceso y generará 
un informe reuniendo la siguiente información de cada uno:
Proceso, Número de escrituras, Primera escritura, Última escritura, Menor posición
y Mayor posición
Parámetros:
    - argc: Número de argumentos pasados por Terminal
    - **argv: Argumentos pasados al programa
Salida: EXITO/FALLO
Llamado por: N/A (Por Terminal)
Llama a: 
    - bmount
    - mi_stat
    - mi_creat
    - mi_read
    - bumount
**/
int main(int argc, char *argv[]) {
    // Comprobar la sintaxis del comando.  //Uso: verificacion <nombre_dispositivo> <directorio_simulación>
    if (argc != 3) {
        fprintf(stderr, RED "Uso: %s <nombre_dispositivo> <directorio_simulación>\n" RESET, argv[0]);
        return FALLO;
    }

    // Montar el dispositivo virtual
    char *disco = argv[1];
    if (bmount(disco) == FALLO) return FALLO;

    //  Calculamos el nº de entradas del directorio de simulación a partir del stat de su inodo
    struct STAT stat;
    char *dir = argv[2];
    if (mi_stat(dir, &stat) == FALLO) return FALLO;
    int numEntradas = stat.tamEnBytesLog / sizeof(struct entrada);
    
#if DEBUGN13 == 1
    fprintf(stderr, "dir_sim: %s\n", dir);
#endif

#if DEBUGN13 == 1
    fprintf(stderr, "numentradas: %d NUMPROCESOS: %d\n", numEntradas, NUMPROCESOS);
#endif   
    if (numEntradas != NUMPROCESOS) {
        fprintf(stderr, RED"El número de entradas no coincide con el número de procesos"RESET);
        bumount(disco);
        return FALLO;
    } 

    // Crear el fichero informe.txt dentro del directorio
    char informe_path[256];
    snprintf(informe_path, sizeof(informe_path), "%sinforme.txt", dir);
    if (mi_creat(informe_path, 7) == FALLO) {
        bumount(disco);
        return FALLO;
    }
    
    // Leer todas las entradas del directorio de simulación de una vez
    struct entrada entradas[NUMPROCESOS*sizeof(struct entrada)];
    if (mi_read(dir, entradas, 0, NUMPROCESOS*sizeof(struct entrada)) == FALLO) {
        bumount(disco);
        return FALLO;
    }

    // Posición del informe
    int posicionInforme = 0;

    for (int i = 0; i < NUMPROCESOS; i++) {
        struct INFORMACION informacion;
        char prueba[256];
        snprintf(prueba, sizeof(prueba), "%s%s/prueba.dat", dir, entradas[i].nombre);
        char *nombreEntrada = strchr(entradas[i].nombre,'_');
        nombreEntrada++;

        // Inicializamos el struct INFORMACION y obtenemos el PID de la entrada
        informacion.pid = atoi(nombreEntrada);
        informacion.nEscrituras = 0;
        informacion.PrimeraEscritura.fecha = time(NULL);
        informacion.PrimeraEscritura.nEscritura = NUMESCRITURAS;
        informacion.PrimeraEscritura.nRegistro = 0;
        informacion.UltimaEscritura.fecha = 0;
        informacion.UltimaEscritura.nEscritura = 0;
        informacion.UltimaEscritura.nRegistro = 0;
        informacion.MenorPosicion.fecha = time(NULL);
        informacion.MenorPosicion.nEscritura = 0;
        informacion.MenorPosicion.nRegistro = REGMAX;
        informacion.MayorPosicion.fecha = time(NULL);
        informacion.MayorPosicion.nEscritura = 0;
        informacion.MayorPosicion.nRegistro = 0;

        int cant_registros_buffer_escrituras = 256; 
        struct REGISTRO buffer_escrituras [cant_registros_buffer_escrituras];
        if (memset(buffer_escrituras, 0, sizeof(buffer_escrituras)) == NULL) {
            fprintf(stderr, RED "Error al reservar memoria\n" RESET);
            return FALLO;
        }
        int offset = 0;
        // Leer una escritura
        while (mi_read(prueba, buffer_escrituras, offset, sizeof(buffer_escrituras)) > EXITO) {
            for (int j=0; j < cant_registros_buffer_escrituras; j++) {
                // Comprobamos si la escritura es válida
                if (buffer_escrituras[j].pid == informacion.pid) {
                    informacion.nEscrituras++;
                    // Comprobar si es la primera escritura válida
                    if (buffer_escrituras[j].nEscritura < informacion.PrimeraEscritura.nEscritura) {
                        informacion.PrimeraEscritura.fecha=buffer_escrituras[j].fecha;
                        informacion.PrimeraEscritura.nEscritura=buffer_escrituras[j].nEscritura;
                        informacion.PrimeraEscritura.nRegistro=buffer_escrituras[j].nRegistro;
                    } else {
                        // Comparar la fecha para obtener la primera y última escritura
                        if ((difftime(buffer_escrituras[j].fecha, informacion.PrimeraEscritura.fecha) <= 0) &&
                                (buffer_escrituras[j].nEscritura < informacion.PrimeraEscritura.nEscritura)) {
                            informacion.PrimeraEscritura = buffer_escrituras[j];
                        }

                        if ((difftime(buffer_escrituras[j].fecha, informacion.UltimaEscritura.fecha) >= 0)  &&
                            (buffer_escrituras[j].nEscritura > informacion.UltimaEscritura.nEscritura)) {
                            informacion.UltimaEscritura = buffer_escrituras[j];
                        } 
                        
                        // Miramos la mayor y menor de las escrituras con nRegistro
                        if (buffer_escrituras[j].nRegistro < informacion.MenorPosicion.nRegistro) {
                            informacion.MenorPosicion = buffer_escrituras[j];
                        }

                        if (buffer_escrituras[j].nRegistro > informacion.MayorPosicion.nRegistro) {
                            informacion.MayorPosicion = buffer_escrituras[j];
                        }
                    }
                    
                }
            }
            memset(&buffer_escrituras, 0, sizeof(buffer_escrituras));
            offset+=sizeof(buffer_escrituras);
        }
        
#if DEBUGN13 == 1
    fprintf(stderr, "[%d) %d escrituras validadas en %s]\n", i+1, informacion.nEscrituras, prueba);
#endif
        // Añadir la información del struct info al fichero informe.txt por el final
        char buffer[1024]; // Buffer que ira recopilando toda la información para luego escribirla al fichero
        memset(buffer, 0, sizeof(buffer));

        // Recopilamos toda la información en el buffer
        sprintf(buffer, "PID: %d\n", informacion.pid);
        sprintf(buffer + strlen(buffer), "Numero escrituras: %d\n", informacion.nEscrituras);
        sprintf(buffer + strlen(buffer), "Primera escritura\t%d\t%d\t%s", informacion.PrimeraEscritura.nEscritura, informacion.PrimeraEscritura.nRegistro, asctime(localtime(&informacion.PrimeraEscritura.fecha)));
        sprintf(buffer + strlen(buffer), "Ultima escritura\t%d\t%d\t%s", informacion.UltimaEscritura.nEscritura, informacion.UltimaEscritura.nRegistro, asctime(localtime(&informacion.UltimaEscritura.fecha)));
        sprintf(buffer + strlen(buffer), "Menor Posicion\t\t%d\t%d\t%s", informacion.MenorPosicion.nEscritura, informacion.MenorPosicion.nRegistro, asctime(localtime(&informacion.MenorPosicion.fecha)));
        sprintf(buffer + strlen(buffer), "Mayor Posicion\t\t%d\t%d\t%s\n", informacion.MayorPosicion.nEscritura, informacion.MayorPosicion.nRegistro, asctime(localtime(&informacion.MayorPosicion.fecha)));

        // Escribimos el contenido del buffer al fichero
        posicionInforme += mi_write(informe_path, buffer, posicionInforme, strlen(buffer));
    }
    if (bumount() == FALLO) return FALLO;
    return EXITO;
}