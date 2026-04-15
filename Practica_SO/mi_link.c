// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López
#include "directorios.h"
#define tambuffer BLOCKSIZE*4

/**
Función: crea un enlace a un fichero, llamando a la función mi_link() de la capa de directorios
Parámetros: 
    - argc: Número de argumentos pasados por Terminal
    - argv: Argumentos pasados al programa
Salida: EXITO/FALLO
Llamado por: N/A (Por Terminal)
Llama a: 
    - bmount()
    - bumount()
    - mi_link()
**/

int main(int argc, char *argv[]) {   
    if (argc != 4) {
        fprintf(stderr, RED "Uso: %s <disco> </ruta_fichero> </ruta_enlace>\n" RESET, argv[0]);
        return FALLO;
    }

    char *camino = argv[2];
    if (camino[strlen(camino)-1] == '/') {
        fprintf(stderr, RED"Error: La ruta proporcionada no referencia un fichero\n"RESET);
        return FALLO;
    }

    char *caminoEnlace = argv[3];
    if (caminoEnlace[strlen(camino)-1] == '/') {
        fprintf(stderr, RED"Error: La ruta proporcionada del enlace no referencia un fichero\n"RESET);
        return FALLO;
    }
    
    char *nombre_dispositivo = argv[1];
    if (bmount(nombre_dispositivo) == FALLO) return FALLO;
    
    // Llamamos a la función mi_link()
    if (mi_link(camino, caminoEnlace) == FALLO) return FALLO;

    if(bumount() == FALLO) return FALLO;
    return EXITO;
}