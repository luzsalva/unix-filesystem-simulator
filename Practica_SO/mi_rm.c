// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López

#include "directorios.h"

/**
Función: borra un fichero, llamando a la función mi_unlink()
Parámetros: 
    - argc: Número de argumentos pasados por Terminal
    - **argv: Argumentos pasados al programa
Salida: EXITO/FALLO
Llamado por: N/A (Por Terminal)
Llama a: 
    - bmount()
    - reservar_inodo()
    - mi_unlink()
    - bumount()
**/
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, RED "Uso: %s <disco> </ruta>\n" RESET, argv[0]);
        return FALLO;
    }
    
    char *ruta = argv[2];
    // Comprobamos si la ruta es un fichero
    if (ruta[strlen(ruta)-1] == '/') {
        fprintf(stderr, RED "La ruta proporcionada no referencia un fichero\n" RESET);
        return FALLO;
    }

    // Montamos el dispositivo a partir de los parámetros pasados por terminal
    char *disco = argv[1];
    if (bmount(disco) == FALLO) return FALLO;

    // Llamamos a la función mi_creat()
    if (mi_unlink(ruta) == FALLO) return FALLO;

    // Cerramos el dispositivo virtual
    if(bumount() == FALLO) return FALLO;
    return EXITO;
}