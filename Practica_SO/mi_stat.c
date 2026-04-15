// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López
#include "directorios.h"

/**
Función: Programa que muestra la información acerca del inodo de un fichero o directorio
Parámetros: 
    - argc: Número de argumentos pasados por Terminal
    - argv: Argumentos pasados al programa
Salida: EXITO/FALLO
Llamado por: N/A (Por Terminal)
Llama a: 
    - bmount()
    - bumount()
    - mi_stat()
**/

int main(int argc, char *argv[]) {
    struct STAT stat;

    if (argc != 3) {
        fprintf(stderr, RED "Uso: %s <disco> </ruta>\n" RESET, argv[0]);
        return FALLO;
    } 

    // Montamos el dispositivo a partir de los parámetros pasados por terminal
    char *disco = argv[1];
    if (bmount(disco) == FALLO) return FALLO;

    char *ruta = argv[2];
    
    // Llamamos a la función mi_stat()
    if (mi_stat(ruta, &stat) == FALLO) return FALLO;

    struct tm *ts;
    char atime[100];
    char mtime[100];
    char ctime[100];
    ts = localtime(&stat.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&stat.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&stat.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);

    fprintf(stderr,"\ntipo: %c\n", stat.tipo);
    fprintf(stderr,"permisos: %d\n", stat.permisos);
    fprintf(stderr,"atime: %s\n", atime);
    fprintf(stderr,"ctime: %s\n", ctime);
    fprintf(stderr,"mtime: %s\n", mtime);
    fprintf(stderr,"nlinks: %d\n", stat.nlinks);
    fprintf(stderr,"tamEnBytesLog: %d\n", stat.tamEnBytesLog);
    fprintf(stderr,"numBloquesOcupados: %d\n", stat.numBloquesOcupados);

    // Cerramos el dispositivo virtual
    if(bumount() == FALLO) return FALLO;
    return EXITO;
}