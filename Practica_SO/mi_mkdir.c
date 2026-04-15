// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López
#include "directorios.h"

/**
Función: Programa (comando) que crea un directorio, llamando a la función mi_creat()
Parámetros:
    - argc: Número de argumentos pasados por Terminal
    - **argv: Argumentos pasados al programa
Salida: EXITO/FALLO
Llamado por: N/A (Por Terminal)
Llama a: 
    - bmount()
    - mi_creat()
    - bumount()
**/
int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, RED "Uso: %s <disco> <permisos> </ruta>\n" RESET, argv[0]);
        return FALLO;
    }
    // Convertimos a entero permisos
    int permisos=atoi(argv[2]);

    // Si permisos no es un número válido 
    if (!((permisos>=0) && (permisos<=7))) {
        fprintf(stderr, RED "Error: modo inválido <<%d>>\n" RESET, permisos);
        return FALLO;
    }
    
    char *ruta = argv[3];
    // Comprobamos si hay que crear un fichero o un directorio 
    if (ruta[strlen(ruta)-1] != '/') {
        fprintf(stderr, RED "Error: No es un directorio\n" RESET);
        return FALLO;
    }

    // Montamos el dispositivo a partir de los parámetros pasados por terminal
    char *disco = argv[1];
    if (bmount(disco) == FALLO) return FALLO;

    // Llamamos a la función mi_creat()
    if (mi_creat(ruta, permisos) == FALLO) return FALLO;

    // Cerramos el dispositivo virtual
    if(bumount() == FALLO) return FALLO;
    return EXITO;
}
