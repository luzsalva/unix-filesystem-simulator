// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López
#include "directorios.h"

/**
Función: Programa que permite escribir texto en una posición de un fichero (offset)
Parámetros: 
    - argc: Número de argumentos pasados por Terminal
    - argv: Argumentos pasados al programa
Salida: EXITO/FALLO
Llamado por: N/A (Por Terminal)
Llama a: 
    - bmount()
    - bumount()
    - mi_write()
    - mi_stat_f()
**/

int main(int argc, char *argv[]) {   
    if (argc != 5) {
        fprintf(stderr, RED "Uso: %s <disco> </ruta_fichero> <texto> <offset>\n" RESET, argv[0]);
        return FALLO;
    }
    
    char *camino = argv[2]; 
    if (camino[strlen(camino)-1] == '/') {
        fprintf(stderr, RED"La ruta proporcionada no referencia un fichero"RESET);
        return FALLO;
    }

    // Montamos el dispositivo a partir de los parámetros pasados por terminal
    char *nombre_dispositivo = argv[1];
    if (bmount(nombre_dispositivo) == FALLO) return FALLO;

    char *texto = argv[3];    
    int offset= atoi(argv[4]);
    int longitud_texto = strlen(texto); 
    unsigned int ninodo=0;

    printf("Longitud texto: %d\n", longitud_texto);
        
    // Escribimos el texto en el inodo en el offset correspondiente
    int bytesEscritos = mi_write(camino, texto, offset, longitud_texto);
    if (bytesEscritos == FALLO) return FALLO;
    printf("Bytes escritos: %d\n", bytesEscritos);

    // Mostramos el tamaño en bytes lógico del inodo y el número de bloques ocupados
    if (DEBUGN9==1) {
        struct STAT stat_inodo;
        if (mi_stat_f(ninodo, &stat_inodo) == FALLO) return FALLO;
    }
    
    // Cerramos el dispositivo virtual
    if(bumount() == FALLO) return FALLO;
    return EXITO;
}