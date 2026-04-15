// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López

#include "directorios.h"
#include <time.h>

void mostrar_buscar_entrada(char *camino, char reservar);

/**
Función: Muestra el texto pertinente a buscar_entrada
Parámetros: 
    - camino: Camino de la entrada
    - reservar: Determina si tenemos que reservar la entrada en caso de no existir
Salida: N/A
Llamado por:
    - buscar_entrada()
Llama a: 
    - buscar_entrada()
    - mostrar_error_buscar_entrada()
    - cacheFIFO()
    - cacheLRU()
*/
void mostrar_buscar_entrada(char *camino, char reservar) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    printf("\ncamino: %s, reservar: %d\n", camino, reservar);
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, reservar, 6)) < 0) {
        mostrar_error_buscar_entrada(error);
    }
    printf("**********************************************************************\n");
    return;
}


/**
Función: Ayuda a determinar la información almacenada en el superbloque, en el mapa de bits o en el array de inodos 
Parámetros: 
    - argc: Número de argumentos pasados por Terminal
    - argv: Argumentos pasados al programa
Salida: EXITO/FALLO
Llamado por: N/A (Por Terminal)
Llama a: 
    - bmount()
    - bread()
    - tamAI()
    - reservar_bloque()
    - liberar_bloque()
    - leer_bit()
    - reservar_inodo()
    - leer_inodo()
    - traducir_bloque_inodo()
    - bumount()
    - mostrar_buscar_entrada()
**/
int main(int argc, char **argv) {

    // Comprobamos que solo se han introducido 2 argumentos, con la sintaxis correcta
    if (argc != 2)
    {
        fprintf(stderr, RED "Error de Sintaxis: $./leer_sf <nombre_dispositivo>\n" RESET);
        return FALLO;
    }

    char *nombre_dispositivo = argv[1];
    
    // Montamos el dispositivo virtual
    bmount(nombre_dispositivo);

    // Leemos el superbloque
    struct superbloque SB;
    if (bread(posSB, &SB)==FALLO) return FALLO;

    printf("\nDATOS DEL SUPERBLOQUE\n");
    printf("posPrimerBloqueMB = %x\n", SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB = %d\n", SB.posUltimoBloqueMB);
    printf("posPrimerBloqueAI = %d\n", SB.posPrimerBloqueAI);
    printf("posUltimoBloqueAI = %d\n", SB.posUltimoBloqueAI);
    printf("posPrimerBloqueDatos = %d\n", SB.posPrimerBloqueDatos);
    printf("posUltimoBloqueDatos = %d\n", SB.posUltimoBloqueDatos);
    printf("nposInodoRaiz = %d\n", SB.posInodoRaiz);
    printf("posPrimerInodoLibre = %d\n", SB.posPrimerInodoLibre);
    printf("cantBloquesLibres = %d\n", SB.cantBloquesLibres);
    printf("cantInodosLibres = %d\n", SB.cantInodosLibres);
    printf("totBloques = %d\n", SB.totBloques);
    printf("totInodos = %d\n", SB.totInodos);

    if (DEBUGN2==1) { 
        printf ("sizeof struct superbloque: %lu\n",sizeof(struct superbloque));
        printf ("sizeof struct inodo: %lu\n",sizeof(struct inodo));
        
        printf ("\nRECORRIDO LISTA ENLAZADA DE INODOS LIBRES\n");
        int contador = 0;
        unsigned int bloqueLeido;
        // Recorremos el array de bloques de inodos
        for(int i = 0 ; i < tamAI(SB.totInodos); i++ ) {
            // Declaramos un buffer para guardar el número de inodos que hay en un bloque
            struct inodo buffer[BLOCKSIZE/INODOSIZE]; 

            // Actualizamos el bloque leído
            bloqueLeido=SB.posPrimerBloqueAI+i;
            if (bread(bloqueLeido,&buffer)==FALLO) return FALLO;

            // Recorremos los inodos del bloque leído
            for (int j=0; (j < BLOCKSIZE/INODOSIZE && contador<SB.totInodos); j++) {
                printf("%d ",buffer[j].punterosDirectos[0]);
                contador++;
            }
        }
        printf("\n");
    }

    struct inodo inodo;
    int ninodo=0;

    if (DEBUGN3==1) {     
        printf ("\nRESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n");
        unsigned int bloqueReservado=reservar_bloque();
        printf ("Se ha reservado el bloque físico nº %d que era el 1º libre indicado por el MB\n", bloqueReservado);
        if (bread(posSB,&SB)==FALLO) return FALLO;
        printf ("SB.cantBloquesLibres = %d\n",SB.cantBloquesLibres);
        if (liberar_bloque(bloqueReservado)==FALLO) return FALLO;
        if (bread(posSB,&SB)==FALLO) return FALLO;
        printf ("Liberamos ese bloque y después SB.cantBloquesLibres = %d\n", SB.cantBloquesLibres);

        
        printf ("\nMAPA DE BITS CON BLOQUES DE METADATOS OCUPADOS\n");
        
        printf("posSB: %d → leer_bit(%d) = %d\n", posSB, posSB, leer_bit(posSB));
        printf("SB.posPrimerBloqueMB: %d → leer_bit(%d) = %d\n", SB.posPrimerBloqueMB, SB.posPrimerBloqueMB, leer_bit(SB.posPrimerBloqueMB));
        printf("SB.posUltimoBloqueMB: %d → leer_bit(%d) = %d\n", SB.posUltimoBloqueMB, SB.posUltimoBloqueMB, leer_bit(SB.posUltimoBloqueMB));
        printf("SB.posPrimerBloqueAI: %d → leer_bit(%d) = %d\n", SB.posPrimerBloqueAI, SB.posPrimerBloqueAI, leer_bit(SB.posPrimerBloqueAI));
        printf("SB.posUltimoBloqueAI: %d → leer_bit(%d) = %d\n", SB.posUltimoBloqueAI, SB.posUltimoBloqueAI, leer_bit(SB.posUltimoBloqueAI));
        printf("SB.posPrimerBloqueDatos: %d → leer_bit(%d) = %d\n", SB.posPrimerBloqueDatos, SB.posPrimerBloqueDatos, leer_bit(SB.posPrimerBloqueDatos));
        printf("SB.posUltimoBloqueDatos: %d → leer_bit(%d) =  %d\n", SB.posUltimoBloqueDatos, SB.posUltimoBloqueDatos, leer_bit(SB.posUltimoBloqueDatos));

        printf("\nDATOS DEL DIRECTORIO RAIZ\n");
        struct tm *ts;
        char atime[80];
        char mtime[80];
        char ctime[80];
        if (leer_inodo(ninodo, &inodo)==FALLO) return FALLO;
        ts = localtime(&inodo.atime);
        strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
        ts = localtime(&inodo.mtime);
        strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
        ts = localtime(&inodo.ctime);
        strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);

        if (leer_inodo(ninodo, &inodo)==FALLO) return FALLO;
        
        printf("tipo: %c\n", inodo.tipo);
        printf("permisos: %d\n", inodo.permisos);
        printf("atime: %s\n", atime);
        printf("ctime: %s\n", ctime);
        printf("mtime: %s\n", mtime);
        printf("nlinks: %d\n", inodo.nlinks);
        printf("tamEnBytesLog: %d\n", inodo.tamEnBytesLog);
        printf("numBloquesOcupado: %d\n", inodo.numBloquesOcupados);
    }
  
    if (DEBUGN4==1) {
        ninodo = reservar_inodo('f', 6);
        
        printf("\nINODO 1. TRADUCCION DE LOS BLOQUES LOGICOS 8, 204, 30.004, 400.004 y 468.750\n");
        // Leemos el inodo para obtener sus datos actualizados
        if (bread(posSB,&SB)==FALLO) return FALLO;
        
        // Asumiendo que la función traducir_bloque_inodo está bien definida
        traducir_bloque_inodo(ninodo, 8, 1);
        traducir_bloque_inodo(ninodo, 204, 1);
        traducir_bloque_inodo(ninodo, 30004, 1);
        traducir_bloque_inodo(ninodo, 400004, 1);
        traducir_bloque_inodo(ninodo, 468750, 1);

        if (leer_inodo(ninodo, &inodo) == FALLO) {
            return FALLO;
        }
        
        printf("\nDATOS DEL INODO RESERVADO 1\n");
        printf("tipo: %c\n", inodo.tipo);
        printf("permisos: %d\n", inodo.permisos);
        printf("atime: %ld\n", inodo.atime);
        printf("ctime: %ld\n", inodo.ctime);
        printf("mtime: %ld\n", inodo.mtime);
        printf("nlinks: %d\n", inodo.nlinks);
        printf("tamEnBytesLog: %d\n", inodo.tamEnBytesLog);
        printf("numBloquesOcupados: %d\n", inodo.numBloquesOcupados);
        printf("\nSB.posPrimerInodoLibre = %d\n", SB.posPrimerInodoLibre);
    }
    
    if (DEBUGN7==1){
        //Mostrar creación directorios y errores
        mostrar_buscar_entrada("pruebas/", 1); //ERROR_CAMINO_INCORRECTO
        mostrar_buscar_entrada("/pruebas/", 0); //ERROR_NO_EXISTE_ENTRADA_CONSULTA
        mostrar_buscar_entrada("/pruebas/docs/", 1); //ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO
        mostrar_buscar_entrada("/pruebas/", 1); // creamos /pruebas/
        mostrar_buscar_entrada("/pruebas/docs/", 1); //creamos /pruebas/docs/
        mostrar_buscar_entrada("/pruebas/docs/doc1", 1); //creamos /pruebas/docs/doc1
        mostrar_buscar_entrada("/pruebas/docs/doc1/doc11", 1);  
        //ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO
        mostrar_buscar_entrada("/pruebas/", 1); //ERROR_ENTRADA_YA_EXISTENTE
        mostrar_buscar_entrada("/pruebas/docs/doc1", 0); //consultamos /pruebas/docs/doc1
        mostrar_buscar_entrada("/pruebas/docs/doc1", 1); //ERROR_ENTRADA_YA_EXISTENTE
        mostrar_buscar_entrada("/pruebas/casos/", 1); //creamos /pruebas/casos/
        mostrar_buscar_entrada("/pruebas/docs/doc2", 1); //creamos /pruebas/docs/doc2
    }

    // Cerramos el dispositivo virtual
    bumount();
    return EXITO;
}