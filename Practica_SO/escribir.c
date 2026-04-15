// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López

#include "ficheros.h"

/**
Función: Escribe texto en uno o varios inodos para obtener un nº de inodo, ninodo, que muestra por pantalla
Parámetros: 
    - argc: Número de argumentos pasados por Terminal
    - **argv: Argumentos pasados al programa
Salida: EXITO/FALLO
Llamado por: N/A (Por Terminal)
Llama a: 
    - bmount()
    - reservar_inodo()
    - mi_write_f()
    - mi_stat_f()
    - bumount()
**/
int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, RED "Uso: %s <nombre_dispositivo> <""$(cat fichero)""> <diferentes_inodos>\n" RESET, argv[0]);
        return FALLO;
    }

    // Montamos el dispositivo a partir de los parámetros pasados por terminal
    char *nombre_dispositivo = argv[1];
    if (bmount(nombre_dispositivo) ==FALLO) return FALLO;

    char *texto = argv[2];    
    int diferentes_inodos = atoi(argv[3]);
    int longitud_texto = strlen(texto); 
    unsigned int ninodo=0;
    // Variable booleana que determinará si diferentes_inodos es 0
    bool esCero=true;

    printf("\nlongitud texto: %d\n\n", longitud_texto);

    // Inicialización de variables para los diferentes offsets
    unsigned int offsets[5] = {9000, 209000, 30725000, 409605000, 480000000};
    
    for (int i = 0; i < 5; ++i) {
        // Si diferentes_inodos es 0, solo se reservará un inodo
        if ((diferentes_inodos==0) && (esCero)) {
            ninodo = reservar_inodo('f', 6);
            if (ninodo == FALLO) return FALLO;
            // Comprobamos que solo se reserva una vez
            esCero=false;
        } else if (diferentes_inodos == 1) {
            ninodo = reservar_inodo('f', 6);
            if (ninodo == FALLO) return FALLO;
        }
        
        printf("Nº inodo reservado: %d\n", ninodo);
        printf("offset: %d\n", offsets[i]);
        
        // Escribimos el texto en el inodo en el offset correspondiente
        int bytesEscritos = mi_write_f(ninodo, texto, offsets[i], longitud_texto);
        if (bytesEscritos == FALLO) return FALLO;
        printf("Bytes escritos: %d\n", bytesEscritos);

        // Mostramos el tamaño en bytes lógico del inodo y el número de bloques ocupados
        struct STAT stat_inodo;
        if (mi_stat_f(ninodo, &stat_inodo) == FALLO) return FALLO;


        printf("stat.tamEnBytesLog=%d\n", stat_inodo.tamEnBytesLog);
        printf("stat.numBloquesOcupados=%d\n\n\n", stat_inodo.numBloquesOcupados);
    }
    
    // Cerramos el dispositivo virtual
    if(bumount() == FALLO) return FALLO;
    return EXITO;
}
