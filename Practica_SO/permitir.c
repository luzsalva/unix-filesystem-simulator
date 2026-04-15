// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López

#include "ficheros.h"

/**
Función: Programa externo ficticio para probar temporalmente el cambio de permisos 
Parámetros: 
    - argc: Número de argumentos pasados por Terminal
    - argv: Argumentos pasados al programa
Salida: EXITO/FALLO
Llamado por: N/A (Por Terminal)
Llama a: 
    - bmount()
    - bumount()
    - mi_chmod_f()
**/
int main(int argc, char *argv[]) {
    // Validación de sintaxis. Comprueba si el número de argumentos es correcto y en caso contrario muestra la sintaxis.
    if (argc != 4) {
        fprintf(stderr, RED"./permitir <nombre_dispositivo> <ninodo> <permisos>\n"RESET);
        return FALLO;
    }
    // Montamos el dispositivo
    char *nombre_dispositivo = argv[1];
    if (bmount(nombre_dispositivo) == FALLO) return FALLO;
    unsigned int ninodo = atoi(argv[2]);
    unsigned int permisos= atoi(argv[3]);

    // Llamamos a mi_chmod_f() con los argumentos recibidos, convertidos a entero
    if (mi_chmod_f(ninodo, permisos) == FALLO) return FALLO;

    // Desmontamos el dispositivo
    if(bumount() == FALLO) return FALLO;
    return EXITO;
}