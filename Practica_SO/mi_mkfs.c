// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López

#include "directorios.h"

/**
Función: Formatea el dispositivo virtual con el tamaño adecuado de bloques, nbloques. 
Parámetros: 
    - argc: Número de argumentos pasados por Terminal
    - argv: Argumentos pasados al programa
Salida: EXITO/FALLO
Llamado por: N/A (Por Terminal)
Llama a: 
    - bmount()
    - bwrite()
    - bumount()
    - initSB()
    - initMB()
    - initAI()
    - reservar_inodo()
**/
int main(int argc, char **argv)
{
    // Comprobamos que solo se han introducido 3 argumentos, con la sintaxis correcta
    if (argc != 3)
    {
        fprintf(stderr, RED "Error de Sintaxis: $./mi_mkfs <nombre_dispositivo> <nbloques> \n" RESET);
    }
    // Declaramos las variables a partir de la información de argv
    char *nombre_dispositivo = argv[1];
    int nbloques = atoi(argv[2]);
    int ninodos=nbloques/4;

    // Montamos el dispositivo virtual
    bmount(nombre_dispositivo);
    
    // Llamamos a las funciones para inicializar el sistema de archivos

    // Declaramos el buffer y reservamos memoria 
    unsigned char buffer[BLOCKSIZE];
    if((memset(buffer, 0, BLOCKSIZE)) == NULL) {
        fprintf(stderr, RED "Error al inicializar el buffer \n" RESET);
        return FALLO;
    }

    for (int i = 0; i < nbloques; i++)
    {
        // Escribimos en el ibloque lo que hay en el buffer
        if (bwrite(i, buffer) == FALLO) {
            // Aún habiendo un error, salimos del dispositivo virtual
            bumount();
            return FALLO;
        }    
    }
    initSB(nbloques, ninodos);
    initMB();
    initAI();

    // Creamos el directorio raíz
    reservar_inodo ('d', 7);

    // Cerramos el dispositivo virtual
    bumount();
    return EXITO;
}