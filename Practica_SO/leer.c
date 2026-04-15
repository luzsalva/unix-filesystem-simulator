// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López

#include "ficheros.h"

#define tambuffer 1500

/**
Función: Lee el inodo indicado por el comando del dispositivo del comando
Parámetros: 
    - argc: Número de argumentos pasados por Terminal
    - argv: Argumentos pasados al programa
Salida: EXITO/FALLO
Llamado por: N/A (Por Terminal)
Llama a: 
    - bmount()
    - bumount()
    - leer_inodo()
    - mi_read_f()
    
**/
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, RED "./leer <nombre_dispositivo> <ninodo> \n" RESET);
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

    leidos = mi_read_f(ninodo, buffer_texto, offset, tambuffer);
    while (leidos > 0) {
        write(1, buffer_texto, leidos);
        total_leidos+=leidos;
        offset += tambuffer;
        // Limpiamos el buffer con cada llamada a mi_read_f()
        if (memset(buffer_texto, 0, tambuffer) == NULL) {
            fprintf(stderr, RED "Error al guardar memoria\n" RESET);
            return FALLO;
        }
        leidos = mi_read_f(ninodo, buffer_texto, offset, tambuffer);
    }

    // Mostrar información sobre la lectura
    char string[128];
    char string2[128];
    unsigned int tamEnBytesLog=inodo.tamEnBytesLog;
    
    sprintf(string, "\ntotal_leidos: %d\n", total_leidos);
    write(2, string, strlen(string));
    sprintf(string2,"Tamaño en bytes lógico del inodo: %u\n\n", tamEnBytesLog);
    write(2, string2, strlen(string2));

    if(bumount() == FALLO) return FALLO;
    return EXITO;
}
