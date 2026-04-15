// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López
#include "directorios.h"
#define tambuffer BLOCKSIZE*4

/**
Función: Programa que muestra TODO el contenido de un fichero
Parámetros: 
    - argc: Número de argumentos pasados por Terminal
    - argv: Argumentos pasados al programa
Salida: EXITO/FALLO
Llamado por: N/A (Por Terminal)
Llama a: 
    - bmount()
    - bumount()
    - leer_inodo()
    - mi_read()
    - write()
**/

int main(int argc, char *argv[]) {   
    if (argc != 3) {
        fprintf(stderr, RED "Uso: %s <disco> </ruta_fichero> \n" RESET, argv[0]);
        return FALLO;
    }

    char *camino = argv[2];
    if (camino[strlen(camino)-1] == '/') {
        fprintf(stderr, RED"La ruta proporcionada no referencia un fichero"RESET);
        return FALLO;
    }
    
    char *nombre_dispositivo = argv[1];
    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    unsigned int ninodo = atoi(argv[2]);

    char buffer_texto[tambuffer];
    if (memset(buffer_texto, 0, tambuffer) == NULL) {
        fprintf(stderr, RED "Error al guardar memoria\n" RESET);
        return FALLO;
    }
    
    int offset = 0;
    int leidos = 0;
    int total_leidos=0;

    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;

    leidos = mi_read(camino, buffer_texto, offset, tambuffer);
    while (leidos > 0) {
        write(1, buffer_texto, leidos);
        total_leidos+=leidos;
        offset += tambuffer;
        // Limpiamos el buffer con cada llamada a mi_read_f()
        if (memset(buffer_texto, 0, tambuffer) == NULL) {
            fprintf(stderr, RED "Error al guardar memoria\n" RESET);
            return FALLO;
        }
        leidos = mi_read(camino, buffer_texto, offset, tambuffer);
    }

    // Mostrar información sobre la lectura
    char string[128];
    
    sprintf(string, "\n\ntotal_leidos: %d\n", total_leidos);
    write(2, string, strlen(string));

    if(bumount() == FALLO) return FALLO;
    return EXITO;
}