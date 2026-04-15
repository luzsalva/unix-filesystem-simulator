// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López

#include "ficheros.h"

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
    - liberar_inodo()
    - mi_truncar_f()
    - mi_stat_f()
    - mi_read_f()
**/
int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, RED "Uso: %s truncar <nombre_dispositivo> <ninodo> <nbytes>\n" RESET, argv[0]);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    if (bmount(nombre_dispositivo) ==FALLO) return FALLO;

    unsigned int ninodo = atoi(argv[2]);
    unsigned int nbytes = atoi(argv[3]);

    // Si nbytes es 0, liberamos el inodo
    if (nbytes == 0) {
        if (liberar_inodo(ninodo) == FALLO) return FALLO;
        
    } else {
        // Si no, truncamos desde nbytes
        mi_truncar_f(ninodo, nbytes);
    }

    // Mostrar el tamaño en bytes lógico del inodo y el número de bloques ocupados
    struct STAT stat_inodo;
    if (mi_stat_f(ninodo, &stat_inodo) == FALLO) return FALLO;

    struct tm *ts;
    char atime[80], mtime[80], ctime[80];
    ts = localtime(&stat_inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&stat_inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&stat_inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);

    printf("\nDATOS INODO 1: \n");
    printf("tipo=%c\n", stat_inodo.tipo);
    printf("permisos=%d\n", stat_inodo.permisos);
    printf("atime=%s\n", atime);
    printf("ctime=%s\n", ctime);
    printf("mtime=%s\n", mtime);
    printf("nlinks=%d\n", stat_inodo.nlinks);
    printf("tamEnBytesLog=%d\n", stat_inodo.tamEnBytesLog);
    printf("numBloquesOcupados=%d\n", stat_inodo.numBloquesOcupados);
    
    // Cerramos el dispositivo virtual
    if(bumount() == FALLO) return FALLO;
    return EXITO;
}
