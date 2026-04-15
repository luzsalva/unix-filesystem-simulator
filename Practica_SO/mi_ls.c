// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López
#include "directorios.h"

/**
Función: Lista el contenido de un directorio (nombres de las entradas), llamando a la función mi_dir()
         de la capa de directorios, que es quien construye el buffer que mostrará mi_ls.c.
         En caso de que no se trate de un directorio, mostrará los datos extendidos del fichero en una fila
Parámetros: 
    - argc: Número de argumentos pasados por Terminal
    - **argv: Argumentos pasados al programa
Salida: EXITO/FALLO
Llamado por: N/A (Por Terminal)
Llama a: 
    - bmount()
    - bumount()
    - mi_dir()
**/
int main(int argc, char *argv[]) {
    char buffer[TAMBUFFER];
    memset(buffer, 0, sizeof(buffer));
    char *disco;
    char *camino;
    
    // Comprobamos que se introduce bien el comando por terminal
    if (!((argc == 4) || (argc == 3))) {
        fprintf(stderr, RED "Uso: %s [-l] <disco> </ruta>\n" RESET, argv[0]);
        return FALLO;
    }

    // Comprobamos si se trata de la versión extendida (-l)
    if (strcmp(argv[1], "-l") == 0) {
        camino=argv[3];
        // Montamos el dispositivo
        disco = argv[2];
        if (bmount(disco) == FALLO) return FALLO;

        // Comprobamos si es un directorio o un fichero
        if(camino[strlen(camino)-1] != '/') {
           if (mi_dir(camino, buffer,'f', true) == FALLO) return FALLO;
        } else {
            if (mi_dir(camino, buffer,'d', true) == FALLO) return FALLO;
        }
        printf("%s\n", buffer);
          
    } else { 
        // Si, en cambio, se trata de la versión básica  
        camino=argv[2];     
        // Montamos el dispositivo
        disco = argv[1];
        if (bmount(disco) == FALLO) return FALLO;

        // Comprobamos si es un directorio o un fichero
        if(camino[strlen(camino)-1] != '/') {
           if (mi_dir(argv[2], buffer,'f', false) == FALLO) return FALLO;
        } else {
            if (mi_dir(argv[2], buffer,'d', false) == FALLO) return FALLO;
        }
        printf("%s\n", buffer);
    }
    
    // Cerramos el dispositivo virtual
    if(bumount() == FALLO) return FALLO;
    return EXITO;
}