// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López

#include "ficheros_basico.h"

///////////////////////////////////////////////////////////////////
//                                                               //
//                            NIVEL 2                            //
//                                                               //
///////////////////////////////////////////////////////////////////

/**
Función: Determina la cantidad de bloques requerida para el mapa de bits
Parámetros: 
    - nbloques: Número total de bloques en el sistema de archivos
Salida: Devuelve el número de bloques necesarios para el mapa de bits
Llamado por: 
    - initSB()
    - initMB()
Llama a: N/A
**/
int tamMB(unsigned int nbloques) {
    // Calculamos el tamaño necesario para el mapa de bits
    int tamMB = (nbloques / 8) / BLOCKSIZE;
    
    // Comprobamos si necesita esa cantidad justa o 1 bloque adicional para el resto
    if (((nbloques / 8) % BLOCKSIZE) == 0) {
        return tamMB;
    }   
    return tamMB+1;
}

/**
Función: Determina la cantidad de bloques necesaria para el arreglo de inodos
Parámetros: 
    - ninodos: Número total de inodos que el sistema de archivos debe soportar
Salida: Devuelve el tamaño en bloques del array de inodos
Llamado por: 
    - leer_sf.c
    - initSB()
    - initMB()
Llama a: N/A
**/
int tamAI(unsigned int ninodos) {
    // Calculamos el tamano en bloques necesario para el array de inodos
    int tamAI = (ninodos * INODOSIZE) / BLOCKSIZE;

    // Comprobamos si necesita esa cantidad justa o 1 bloque adicional para el resto
    if (((ninodos * INODOSIZE) % BLOCKSIZE) == 0) {
        return tamAI;
    }   
    return tamAI+1;
}

/**
Función: Inicializa los valores iniciales del superbloque
Parámetros:
    - nbloques: Número total de bloques en el sistema de archivos
    - ninodos: Número total de inodos que el sistema de archivos debe soportar
Salida: Indica si la inicialización funcionó correctamente
Llamado por:
    - mi_mkfs.c
Llama a: 
    - tamMB()
    - tamAI()
    - bwrite()
**/
int initSB(unsigned int nbloques, unsigned int ninodos) {
    // Declaración de la variable local SB, un superbloque
    struct superbloque SB;

    // Calculamos sus campos
    SB.posPrimerBloqueMB = posSB + tamSB;     
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;
    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;
    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques-1;
    SB.posInodoRaiz = 0;
    SB.posPrimerInodoLibre = 0;
    SB.cantBloquesLibres = nbloques;
    SB.cantInodosLibres = ninodos;
    SB.totBloques = nbloques;
    SB.totInodos = ninodos;

    // Declaramos el buffer auxiliar para la escritura
    unsigned char buffer[BLOCKSIZE];
    // Inicializa el buffer con ceros
    if((memset(buffer, 0, BLOCKSIZE)) == NULL) {
        fprintf(stderr, RED "Error al inicializar el buffer \n" RESET);
        return FALLO;
    }
    // Copia los datos de SB en el buffer
    if((memcpy(buffer, &SB, sizeof(struct superbloque))) == NULL){
        fprintf(stderr, RED "Error al copiar los datos en el buffer \n" RESET);
        return FALLO;
    }

    // Escribimos los datos del buffer en el bloque señalado por posSB
    bwrite(posSB, buffer);

    return EXITO;
}

/**
Función: Inicializa el mapa de bits poniendo a 1 los bits correspondientes a los bitsMetadatosMB.
Parámetros: N/A
Salida: Indica si la inicialización funcionó correctamente
Llamado por: 
    - mi_mkfs.c
Llama a: 
    - tamMB()
    - tamAI()
    - bread()
    - bwrite()
**/

int initMB() {
    // Declaramos una varialbe local para poder calcular tamMB
    struct superbloque SB;
    // Leemos el bloque y lo almacenamos en SB
    if (bread(posSB, &SB)==FALLO) return FALLO;
    
    // Almacenamos el número de bloques totales
    int nbloques=SB.totBloques;
    // Y calculamos el tamaño del mapa de bits
    int tamaMB=tamMB(nbloques);

    // Almacenamos el número de bloques necesarios para el array de inodos
    int ninodos=SB.totInodos;
    // Y calculamos el tamaño del array de inodos
    int tamaAI=tamAI(ninodos);

    // Calculamos el número de bloques de los bitsMetadatosMB
    //Bloques físicos en los que caben los metadatos. Cada bloque será un bit del MB
    int bitsMetadatosMB=tamSB+tamaMB+tamaAI; 
    //Bloques físicos DEL MB donde escribir el equivalente en bits ^
    int bloquesMetadatosMB=(bitsMetadatosMB/8)/BLOCKSIZE; 
    // Determinamos la posición del primer bloque de MB
    int posicion;

    // Llenamos los bloques de MB físicos correspondientes a los bits de metadatos
    // Iteramos dentro del número de bloques de metadatos dentro de MB, recorriendolos uno a uno
    for (posicion = SB.posPrimerBloqueMB; posicion < SB.posPrimerBloqueMB+bloquesMetadatosMB; posicion++) {
        // Declaramos el buffer auxiliar para la escritura
        unsigned char bufferAux[BLOCKSIZE];
        // Inicializa el buffer con unos
        if((memset(bufferAux, 255, BLOCKSIZE)) == NULL) {
            fprintf(stderr, RED "Error al inicializar el buffer \n" RESET);
            return FALLO;
        }
        // Escribimos los datos del buffer en el bloque señalado por posSB y sus contiguos
        bwrite(posicion, bufferAux);
    }

    // Calculamos el número de bytes del mapa a poner a 1, coincidiendo con el bloque parcial que queda
    int bytesMetadatos=(bitsMetadatosMB/8)%BLOCKSIZE;
    // Declaramos un array de caracteres para llevar el bloque a memoria
    unsigned char bufferMB[BLOCKSIZE];

    // Comprobamos el resto para saber si tenemos que añadir un bloque adicional del MB (parcialmente lleno)
    if (bytesMetadatos!=0) {
        //bloquesMetadatosMB++;
        if((memset(bufferMB, 0, BLOCKSIZE)) == NULL){
            fprintf(stderr, RED "Error al inicializar el buffer \n" RESET);
            return FALLO;
        }  
        // E iteramos asignando 255 (11111111) en cada espacio del bufferMB
        for (int nbyteMB=0; nbyteMB<bytesMetadatos; nbyteMB++) {
            bufferMB[nbyteMB]=255;
        }
        // Teniendo en cuenta el posible resto del paso a bytes
        int posibleResto=bitsMetadatosMB%8;

        // Si hay resto en la división entre 8
        if (posibleResto != 0) {
            int numeroFinal=0;
            // Calculamos a partir de potencias de 2 el número correspondiente
            // sabiendo que en los 'posibleResto' posiciones empezando por la 
            // izquierda, deberá haber 1s
            unsigned int auxiliar[7]={7, 6, 5, 4, 3, 2, 1};
            for (int nbitMB = 0; nbitMB < posibleResto; nbitMB++) {
                numeroFinal += 1 << auxiliar[nbitMB];
            }
            //Incorporamos al buffer el valor decimal 
            bufferMB[bytesMetadatos]=numeroFinal;

            // Preparamos para el siguiente paso el índice de bits de los bytesMetadatos
            // sabiendo que, al haber resto, necesitamos 1 bit más
            bytesMetadatos++;
        }
    }
    // Rellenamos de 0 las posiciones que quedan vacías del MB
    for (int nbyteMB=bytesMetadatos; nbyteMB<BLOCKSIZE; nbyteMB++) {
        bufferMB[nbyteMB]=0;
    }
    // Actualizamos el número de bloques libres
    SB.cantBloquesLibres -= SB.posUltimoBloqueAI + 1;

    // Escribimos los datos del buffer en el bloque señalado por posSB
    if (bwrite(posSB, &SB)==FALLO) return FALLO;
    // Escribimos el bloque en la posición correspondiente
    if (bwrite(posicion, bufferMB)==FALLO) return FALLO;
    
    return EXITO;
}

/**
Función: Inicializar la lista de inodos libres
Parámetros: N/A
Salida: Indica si la inicialización funcionó correctamente
Llamado por: 
    - mi_mkfs.c
Llama a: 
    - bread()
    - bwrite()
**/
int initAI() {
    // Declaramos una varialbe local
    struct superbloque SB;
    // Leemos el bloque y lo almacenamos en SB
    bread(posSB, &SB);
    // Almacenamos el contador de inodos
    int contInodos = SB.posPrimerInodoLibre + 1;

    struct inodo inodos [BLOCKSIZE/INODOSIZE];
    for (unsigned int nbloqueMB = SB.posPrimerBloqueAI; nbloqueMB <= SB.posUltimoBloqueAI; nbloqueMB++) {
        bread(nbloqueMB, &inodos);
        for (unsigned int nbloque = 0; nbloque < BLOCKSIZE / INODOSIZE;  nbloque++) {
            inodos[nbloque].tipo = 'l';  //libre
            if (contInodos < SB.totInodos) {  //si no hemos llegado al último inodo del AI
                inodos[nbloque].punterosDirectos[0] = contInodos;  //enlazamos con el siguiente
                contInodos++;
            } else {//hemos llegado al último inodo
                inodos[nbloque].punterosDirectos[0] = UINT_MAX;
                //hay que salir del bucle, el último bloque no tiene por qué estar completo !!! )
                break;
            }
        }
        bwrite(nbloqueMB, &inodos);
    }
    return EXITO;
}


///////////////////////////////////////////////////////////////////
//                                                               //
//                            NIVEL 3                            //
//                                                               //
///////////////////////////////////////////////////////////////////


/**
Función: Escribe  0 (libre) ó 1 (ocupado) en un determinado bit del MB que representa el bloque nbloque. 
         La utilizaremos cada vez que necesitemos reservar o liberar un bloque.
Parámetros: 
    - nbloque: bloque físico que queremos reservar o liberar
    - bit: valor a escribir en el bit del MB 0 (libre) ó 1 (ocupado)
Salida: Indica si se ha escrito correctamente o no
Llamado por:
    - reservar_bloque()
    - liberar_bloque() 
Llama a: 
    - bread()
    - bwrite()
**/
int escribir_bit(unsigned int nbloque, unsigned int bit) {
    // Leemos el superbloque para obtener la localización del MB
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;
    int posMB=SB.posPrimerBloqueMB;

    // Calculamos la posición del byte que representa el bloque en el MB
    unsigned int posbyte = nbloque/8;
    // Calculamos la posición del bit dentro del byte anterior que representa el bloque físico dado
    unsigned int posbit = nbloque % 8;
    // Calculamos el bloque lógico en el que se encuentra el byte donde se encuentra el bloque que buscamos
    unsigned int nbloqueMB = posbyte / BLOCKSIZE;

    // Calculamos el bloque físico en el que se encuentra el byte donde se encuentra el bloque que buscamos
    unsigned int nbloqueAbs = posMB + nbloqueMB;

    // Leemos con bread() el bloque físico que lo contiene y cargamos el contenido en un buffer
    unsigned char bufferMB[BLOCKSIZE];
    if (bread(nbloqueAbs, bufferMB) == FALLO) return FALLO;

    // Actualizamos posbyte de acuerdo con el tamaño de blocksize para buscar la posición de nuestro bloque
    posbyte = posbyte % BLOCKSIZE;

    unsigned char mascara = 128;
    // Desplazamos a la derecha tantos bits como indique posbit
    mascara >>= posbit;

    if (bit==1) {
        // Ponemos a 1 el bit correspondiente en el bloque físico
        bufferMB[posbyte] |= mascara;
    } else if (bit == 0) {
        // Ponemos a 0 el bit correspondiente en el bloque físico
        bufferMB[posbyte] &= ~mascara;
    } else {
        fprintf(stderr, RED "Error en el formato de entrada, bit debe ser 1 o 0 \n" RESET);
        return FALLO;
    }

    // Escribimos ese buffer del MB en el dispositivo virtual en la posición nbloqueabs
    if(bwrite(nbloqueAbs, bufferMB) == FALLO) return FALLO;
    
    return EXITO;
}

/**
Función: Lee un determinado bit del MB
Parámetros: 
    - nbloque:bloque físico del cual queremos saber el valor del bit que lo representa en el MB 
Salida: Valor del bit leído
Llamado por:
    - leer_sf.c
Llama a: 
    - bread()
**/
char leer_bit(unsigned int nbloque) {
    // Leemos el superbloque para obtener la localización del MB
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;

    // Calculamos la posición del byte que representa el bloque en el MB
    unsigned int posbyte = nbloque/8;
    // Calculamos la posición del bit dentro del byte anterior que representa el bloque físico dado
    unsigned int posbit = nbloque % 8;
    // Calculamos el bloque lógico en el que se encuentra el byte donde se encuentra el bloque que buscamos
    unsigned int nbloqueMB = posbyte / BLOCKSIZE;

    // Calculamos el bloque físico en el que se encuentra el byte donde se encuentra el bloque que buscamos
    unsigned int nbloqueAbs = SB.posPrimerBloqueMB + nbloqueMB;

    // Leemos con bread() el bloque físico que lo contiene y cargamos el contenido en un buffer
    unsigned char bufferMB[BLOCKSIZE];
    if (bread(nbloqueAbs, bufferMB) == FALLO) return FALLO;

    unsigned char mascara = 128;
    // Desplazamos a la derecha tantos bits como indique posbit
    mascara >>= posbit;      
    // Ajustamos la posición del byte de acuerdo a BLOCKSIZE
    unsigned int posbyteAjustado = posbyte % BLOCKSIZE;   
    mascara &= bufferMB[posbyteAjustado];
    mascara >>= (7 - posbit);     

    #if DEBUGN3==1
        fprintf(stderr, GRAY "[leer_bit(%d)→ posbyte:%d, posbyte (ajustado): %d, posbit:%d, nbloqueMB:%d, nbloqueAbs:%d)]\n" RESET, nbloque, posbyte, posbyteAjustado, posbit, nbloqueMB, nbloqueAbs);
    #endif
    return mascara;
}

/**
Función: Reserva el primer bloque libre consultando el MB y lo llena con 1
Parámetros: N/A
Salida: Devuelve el número del bloque físico reservado
Llamado por: 
    - leer_sf.c
    - traducir_bloque_inodo()
Llama a: 
    - bread()
    - bwrite()
    - escribir_bit()
**/
int reservar_bloque() {
    // Leemos el superbloque para saber si nos quedan bloques libres
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;
    
    // Comprobamos si quedan bloques libres;
    if (SB.cantBloquesLibres==0) {
        fprintf(stderr, RED "Error, no quedan bloques libres\n" RESET);
        return FALLO;
    }
    
    // Declaramos un buffer auxiliar, bufferAux, inicializado con sus bits a 1s (255 en decimal)
    unsigned char bufferAux[BLOCKSIZE];
    if (memset(bufferAux, 255, BLOCKSIZE)==NULL) {
        fprintf(stderr, RED "Error al reservar memoria\n" RESET);
        return FALLO;
    }
    unsigned int posbyte=0;
    unsigned int nbloque;
    unsigned int posbit = 0;
    unsigned char bufferMB[BLOCKSIZE];
    unsigned int nbloqueMB=0;
    bool encontradoBit=false;

    // Entramos en el bucle, sin salirnos de los bytes que pertenecen al MB
    for (nbloqueMB=0; (nbloqueMB < SB.posUltimoBloqueMB);  nbloqueMB++) {
        if (bread(nbloqueMB + SB.posPrimerBloqueMB, bufferMB) == FALLO) return FALLO;
        // Comparamos el contenido de ambos buffers para encontrar el primer bloque con almenos un bit a 0
        if (memcmp(bufferAux, bufferMB, BLOCKSIZE) > 0) { 
            // Buscamos cuál es el 1r byte de ese bloque, posbyte, que contiene algún 0
            for (posbyte=0; (posbyte < BLOCKSIZE)&&(!encontradoBit); posbyte++) {
                if (bufferMB[posbyte]!=255) {
                    // Encontramos el primer bit a 0, posbit, en ese byte
                    unsigned char mascara = 128;
                    posbit = 0;
                    while (bufferMB[posbyte] & mascara) {
                        bufferMB[posbyte] <<= 1; 
                        posbit++;
                    }
                    encontradoBit=true;
                    break;
                }
            }
            if(encontradoBit) {
                break;
            }
        }
    }

    // Calculamos el bloque físico que queremos ocupar 
    nbloque = (nbloqueMB * BLOCKSIZE + posbyte) * 8 + posbit; //SEGURO
    // Reservamos el bloque poniendo su bit correspondiente a 1
    if (escribir_bit(nbloque, 1)==FALLO) return FALLO;
        
    // Decrementamos la cantidad de bloques libres
    SB.cantBloquesLibres--;
    if (bwrite(posSB, &SB)==FALLO) return FALLO; 
        
    // Limpiamos ese bloque en la zona de datos por si había basura
    unsigned char bufferBasurero[BLOCKSIZE];
    // Inicializa el buffer con ceros
    if((memset(bufferBasurero, 0, BLOCKSIZE)) == NULL) {
        fprintf(stderr, RED "Error al inicializar el buffer basurero\n" RESET);
        return FALLO;
    }
    if (bwrite(nbloque, bufferBasurero) == FALLO) return FALLO;
    return nbloque;
}

/**
Función: Libera el bloque pasado por parámetro
Parámetros: 
    - nbloque: bloque físico que queremos liberar
Salida: Devolvemos el número de bloque liberado
Llamado por: 
    - leer_sf.c
Llama a: 
    - bread()
    - bwrite()
    - escribir_bit()
    - liberar_bloques_recursivo()
**/
int liberar_bloque(unsigned int nbloque){
    // Liberamos el bloque poniendo su bit correspondiente a 0
    if (escribir_bit(nbloque, 0)==FALLO) return FALLO;

    //Incrementamos la cantidad de bloques libres
    struct superbloque SB;
    if (bread(posSB, &SB)==FALLO) return FALLO;
    SB.cantBloquesLibres++;
    if (bwrite(posSB, &SB)==FALLO) return FALLO;
    
    return nbloque;
}

/**
Función: Escribe el contenido de una variable de tipo struct inodo en un determinado inodo del array de inodos inodos
Parámetros:
    - ninodo: Índice del array de inodos
    - inodo: Inodo a escribir en el array de inodos en el índice indicado
Salida: EXITO/FALLO
Llamado por: 
    - reservar_inodo()
    - liberar_inodo()
    - mi_write_f()
    - mi_read_f()
    - mi_chmod_f()
    - mi_truncar_f()
    - buscar_entrada()
    - escribir.c
    - leer_sf.c
    - mi_link()
    - mi_unlink()
Llama a: 
    - bread()
    - bwrite()
**/
int escribir_inodo(unsigned int ninodo, struct inodo *inodo) {
    // Leemos el superbloque para obtener la posición del array de inodos
    struct superbloque SB;
    if (bread(posSB, &SB)==FALLO) return FALLO;
    unsigned int posAI=SB.posPrimerBloqueAI;
    
    // Obtenemos el número de bloque del array de inodos que tiene el inodo solicitado
    unsigned int nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;

    // Calculamos su posición absoluta del dispositivo y lo leemos usando como buffer de lectura un array de inodos
    unsigned int nbloqueabs = nbloqueAI+ posAI;
    struct inodo inodos[BLOCKSIZE/INODOSIZE];

    if (bread(nbloqueabs, inodos)==FALLO) return FALLO;

    //Escribimos el contenido del inodo en el lugar correspondiente del array 
    unsigned int posInodo = ninodo % (BLOCKSIZE / INODOSIZE);
    inodos[posInodo] = *inodo;
    if (bwrite(nbloqueabs, inodos)==FALLO) return FALLO;

    return EXITO;
}

/**
Función: Lee un determinado inodo del array de inodos para volcarlo en una variable de tipo struct inodo pasado por referencia
Parámetros:
    - ninodo: Índice del array de inodos
    - inodo: Inodo a leer del array de inodos en el índice indicado
Salida: EXITO/FALLO
Llamado por:
    - mi_mkfs.c
    - reservar_inodo()
    - leer_inodo()
    - liberar_inodo()
    - leer.c
    - leer_sf.c
    - mi_cat.c
    - mi_write_f()
    - mi_read_f()
    - mi_chmod_f()
    - mi_truncar_f()
    - mi_stat_f()
    - mi_chmod_f()
    - buscar_entrada()
    - mi_dir()
    - mi_unlink()
    - mi_link()
Llama a: 
    - bread()
**/
int leer_inodo(unsigned int ninodo, struct inodo *inodo) {
    // Leemos el superbloque para obtener la posición del array de inodos
    struct superbloque SB;
    if (bread(posSB, &SB)==FALLO) return FALLO;
    unsigned int posAI=SB.posPrimerBloqueAI;

    // Obtenemos el número de bloque del array de inodos que tiene el inodo solicitado
    unsigned int nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;

    // Calculamos su posición absoluta del dispositivo y lo leemos usando como buffer de lectura un array de inodos
    unsigned int nbloqueabs = nbloqueAI+ posAI;
    struct inodo inodos[BLOCKSIZE/INODOSIZE];

    if (bread(nbloqueabs, inodos)==FALLO) return FALLO;

    //Escribimos el contenido del inodo en el lugar correspondiente del array 
    unsigned int posInodo = ninodo % (BLOCKSIZE / INODOSIZE);
    *inodo=inodos[posInodo];

    return EXITO;
}

/**
Función: Encuentra el primer inodo libre lo reserva, devuelve su número y actualiza la lista enlazada de inodos libres
Parámetros:
    - tipo: Campo de la estructura inodo a reservar ('l':libre, 'd':directorio o 'f':fichero)
    - permisos: Campo de la estructura inodo a reservar (lectura y/o escritura y/o ejecución)
Salida: Posición del inodo reservado
Llamado por:
    - mi_mkfs.c
    - leer_sf.c
    - escribir.c
Llama a: 
    - bread()
    - leer_inodo()
    - escribir_inodo()
    - bwrite()
**/
int reservar_inodo(unsigned char tipo, unsigned char permisos) {
    // Leemos el superbloque para obtener la cantidad de inodos libres
    struct superbloque SB;
    if (bread(posSB, &SB)==FALLO) return FALLO;
    // Comprobamos si quedan inodos libres
    if (SB.cantInodosLibres==0) {
        fprintf(stderr, RED "Error, no quedan inodos libres \n" RESET);
        return FALLO;
    }
        
    // Guardamos la posición del primer inodo libre
    unsigned int posInodoReservado = SB.posPrimerInodoLibre;

    // Leemos el inodo libre (reservado) para obtener el índice del siguiente inodo libre
    struct inodo inodoReservado;
    if(leer_inodo(posInodoReservado, &inodoReservado) == FALLO) return FALLO;

    // Actualizamos el superbloque para que apunte al siguiente inodo libre (inodoReservado)
    SB.posPrimerInodoLibre = inodoReservado.punterosDirectos[0];     
    // Decrementamos la cantidad de inodos libres
    SB.cantInodosLibres --;

    // Escribimos el superbloque actualizado de vuelta al dispositivo
    if (bwrite(posSB, &SB) == FALLO) return FALLO;

    // Inicializamos el inodo reservado con los valores correspondientes
    inodoReservado.tipo = tipo;
    inodoReservado.permisos = permisos;
    inodoReservado.nlinks = 1; 
    inodoReservado.tamEnBytesLog = 0; 
    inodoReservado.atime = time(NULL);
    inodoReservado.mtime = time(NULL);
    inodoReservado.ctime = time(NULL);
    inodoReservado.numBloquesOcupados = 0;
    for (int i = 0; i < 12; i++) inodoReservado.punterosDirectos[i] = 0;
    for (int j = 0; j < 3; j++) inodoReservado.punterosIndirectos[j] = 0;
        
    // Escribimos el inodo reservado de vuelta al dispositivo
    if (escribir_inodo(posInodoReservado, &inodoReservado) == FALLO) return FALLO;

    // Devolvemos la posición del inodo reservado
    return posInodoReservado;
}

///////////////////////////////////////////////////////////////////
//                                                               //
//                            NIVEL 4                            //
//                                                               //
///////////////////////////////////////////////////////////////////

/**
Función: Obtiene el rango de punteros en el que se sitúa el bloque lógico que buscamos (0:D, 1:I0, 2:I1, 3:I2), 
         y obtiene la dirección almacenada en el puntero correspondiente del inodo, ptr
Parámetros:
    - *inodo: Inodo en el que se encuntra el bloque lógico a tratar
    - nblogico: Bloque lógico a tratar
    - *ptr: puntero en el que se almacena la dirección al rango de inodos del bloque lógico a tratar
Salida: Rango de punteros del bloque lógico nblogico
Llamado por:
    - mi_mkfs.c
    - leer_sf.c
    - traducir_bloque_inodo()
    - liberar_bloques_inodo()
Llama a: N/A
**/
int obtener_nRangoBL (struct inodo *inodo, unsigned int nblogico, unsigned int *ptr) { 
    if (nblogico<DIRECTOS) {  // <12
        *ptr=inodo->punterosDirectos[nblogico];
        return 0;
    } else if (nblogico<INDIRECTOS0) {   // <268    
        *ptr=inodo->punterosIndirectos[0];  
        return 1;     
    } else if (nblogico<INDIRECTOS1) {   // <65.804     
        *ptr=inodo->punterosIndirectos[1];      
        return 2;       
    } else if(nblogico<INDIRECTOS2) {   // <16.843.020              
        *ptr=inodo->punterosIndirectos[2];             
        return 3;           
    } else {         
        *ptr=0;          
        fprintf(stderr, RED "Error, bloque lógico fuera de rango" RESET);      
        return FALLO;
    }
}


/**
Función:  Calcula el índice del bloque lógico dentro de los diferentes niveles de punteros
Parámetros:
    - nblogico: Número del bloque lógico que queremos obtener su índice correspondiente
    - nivel_punteros: Nivel de dirección del puntero (0 para directos, 1 para indirectos 0, 2 para indirectos 1, 3 para indirectos 2)
Salida: Devuelve el índice del bloque lógico
Llamado por: 
    - traducir_bloque_inodo()
Llama a: N/A
**/

int obtener_indice (unsigned int nblogico, int nivel_punteros) {
    if (nblogico < DIRECTOS) {
        return nblogico;
    } else if (nblogico < INDIRECTOS0) {
        return (nblogico - DIRECTOS);
    } else if (nblogico < INDIRECTOS1) {
        if (nivel_punteros == 2) {
            return ((nblogico - INDIRECTOS0) / NPUNTEROS); 
        } else if (nivel_punteros == 1) {
            return ((nblogico - INDIRECTOS0) % NPUNTEROS);
        }                     
    } else if (nblogico < INDIRECTOS2) {   //ej. nblogico=400.004     
        if (nivel_punteros == 3) { 
            return ((nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS));              
        } else if (nivel_punteros == 2) {       
            return (((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS);            
        } else if (nivel_punteros == 1) {    
            return (((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS);
        }    
    }
    // Si no ha entrado a ninguno de los ifs anteriores, devolvemos FALLO
    fprintf(stderr, RED "No se ha podido obtener ningún índice" RESET);
    return FALLO;
}

/**
Función: Obtiene el nº de bloque físico correspondiente a un bloque lógico determinado del inodo indicado
Parámetros:
    - inodo: inodo en el que se encuntra el bloque lógico a tratar
    - nblogico: bloque de datos lógico
    - reservar: si vale 0 utilizaremos esta función únicamente para consultar
                Si reservar vale 1 la utilizaremos además, 
                si no existe bloque físico,para reservar
Salida: nº de bloque físico correspondiente al bloque de datos lógico, nblogico
Llamado por:
    - mi_read_f()
    - mi_write_f()
    - leer_sf.c
    - mi_link()
Llama a: 
    - obtener_nRangoBL()
    - reservar_bloque()
    - bwrite()
    - bread()
    - obtener_indice()
**/
int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, unsigned char reservar) {
    unsigned int ptr, ptr_ant;
    int nRangoBL, nivel_punteros, indice;
    unsigned int buffer[NPUNTEROS]; 
    struct inodo inodo;

    if(leer_inodo(ninodo, &inodo) == FALLO) return FALLO;

    ptr = 0;
    ptr_ant = 0;
    indice=0;
     //0:D, 1:I0, 2:I1, 3:I2
    nRangoBL = obtener_nRangoBL(&inodo, nblogico, &ptr);
    //el nivel_punteros más alto es el que cuelga directamente del inodo
    nivel_punteros = nRangoBL; 
    //iterar para cada nivel de punteros indirectos
    while (nivel_punteros>0) {
        //no cuelgan bloques de punteros
        if (ptr == 0) { 
            if (reservar == 0) {
               // bloque inexistente
               return FALLO; 
            //reservar bloques de punteros y crear enlaces desde el  inodo hasta el bloque de datos
            } else { 
                //de punteros   
                ptr = reservar_bloque();                
                inodo.numBloquesOcupados++;
                //fecha actual
                inodo.ctime = time(NULL); 
                //el bloque cuelga directamente del inodo
                if (nivel_punteros == nRangoBL) { 
                    inodo.punterosIndirectos[nRangoBL-1] = ptr;
                    #if DEBUGN4==1
                        fprintf(stderr, GRAY "[traducir_bloque_inodo()→ inodo.punterosIndirectos[%d] = %d (reservado BF %d para punteros_nivel%d)]\n" RESET, nRangoBL-1, ptr, ptr, nRangoBL);
                    #endif
                //el bloque cuelga de otro bloque de punteros
                } else {   
                    buffer[indice] = ptr; 
                    #if DEBUGN4==1
                        fprintf(stderr, GRAY "[traducir_bloque_inodo()→ punteros_nivel%d [%d] = %d (reservado BF %d para punteros_nivel%d)]\n" RESET, nivel_punteros+1,indice,ptr,ptr,nivel_punteros);
                    #endif
                    //salvamos en el dispositivo el buffer de punteros modificado 
                    bwrite(ptr_ant, buffer);           
                }
                //ponemos a 0 todos los punteros del buffer
                memset(buffer, 0, BLOCKSIZE);  
            }
        } else {
            //leemos del dispositivo el bloque de punteros ya existente
            if (bread(ptr, buffer) == FALLO) return FALLO;
        } 
        indice = obtener_indice(nblogico, nivel_punteros);
        //guardamos el puntero actual
        ptr_ant = ptr; 
        // y lo desplazamos al siguiente nivel 
        ptr = buffer[indice]; 
        nivel_punteros--;   
    } //al salir de este bucle ya estamos al nivel de datos

    if (ptr == 0) {  
        //no existe bloque de datos
        if (reservar == 0){
            return FALLO; 
        } else {
            ptr = reservar_bloque();
            inodo.numBloquesOcupados++;
            inodo.ctime = time(NULL);
            //si era un puntero Directo 
            if (nRangoBL == 0){
                //asignamos la direción del bl. de datos en el inodo 
                inodo.punterosDirectos[nblogico] = ptr; 
                #if DEBUGN4==1
                    fprintf(stderr, GRAY "[traducir_bloque_inodo()→ inodo.punterosDirectos[%d] = %d (reservado BF %d para BL %d)]\n" RESET, nblogico, ptr, ptr, nblogico);
                #endif
            } else{
                //asignamos la dirección del bloque de datos en el buffer
                buffer[indice] = ptr; 
                #if DEBUGN4==1
                    fprintf(stderr, GRAY "[traducir_bloque_inodo()→ punteros_nivel%d [%d] = %d (reservado BF %d para BL %d)]\n" RESET, nivel_punteros+1,indice,ptr,ptr,nblogico);
                #endif
                //salvamos en el dispositivo el buffer de punteros modificado 
                bwrite(ptr_ant, buffer); 
            }
        }
    }

    if(escribir_inodo(ninodo,&inodo) == FALLO) return FALLO;

    //mi_write_f() se encargará de salvar los cambios del inodo en disco
    return ptr;
}

///////////////////////////////////////////////////////////////////
//                                                               //
//                            NIVEL 6                            //
//                                                               //
///////////////////////////////////////////////////////////////////

/**
Función: Libera un inodo y, con él, todos aquellos bloques de datos que estaba ocupando
Parámetros:
    - ninodo: Número de inodo a eliminar
Salida: Número de inodo liberado
Llamado por:
    - truncar.c
    - mi_link()
    - mi_unlink()
    - buscar_entrada()
    - truncar.c
Llama a: 
    - leer_inodo()
    - liberar_bloques_inodo()
    - bread()
    - bwrite()
    - escribir_inodo()
**/
int liberar_inodo (unsigned int ninodo)  {
    // Leemos el inodo
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;

    // Llamamos a liberar_bloques_inodo()
    unsigned int bloquesLiberados = liberar_bloques_inodo(0, &inodo);
    if (bloquesLiberados==FALLO) return FALLO;
    
    // Calculamos que se hayan liberado todos los bloques ocupados
    inodo.numBloquesOcupados=inodo.numBloquesOcupados-bloquesLiberados;
    
    if (inodo.numBloquesOcupados != 0) {
        fprintf(stderr, RED "No se han liberado todos los bloques ocupados" RESET);
        return FALLO;
    }
    
    // Actualizar el inodo
    inodo.tipo='l';
    inodo.tamEnBytesLog=0;

    // Actualizamos la lista entrelazada de inodos libres
    // Leemos el super bloque
    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) return FALLO;
    
    // Apuntamos la dirección del primer inodo libre al nº de inodo pasado por parámetro
    inodo.punterosDirectos[0]=SB.posPrimerInodoLibre;
    SB.posPrimerInodoLibre=ninodo;

    // Actualizar la cantidad de inodos y bloques libres
    SB.cantInodosLibres++;
    
    // Escribimos el superbloque en el dispositivo virtual
    if (bwrite(posSB, &SB) == FALLO) return FALLO;

    // Actualizamos el ctime
    inodo.ctime = time(NULL);

    // Escribimos el inodo actualizado en el dispositivo virtual
    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;

    return ninodo;
}

/**
Función:
    <<VERSIÓN ITERATIVA CON AUXILIAR RECURSIVA>>
    Libera todos los bloques ocupados a partir del BL pasado por parámetrro
Parámetros:
    - primerBL: bloque lógico a partir del cual liberar los bloques ocupados
    - *inodo: inodo dondde se encuentran los bloques 
Salida: cantidad de bloques liberados
Llamado por:
    - mi_truncar_f()
Llama a: 
    - liberar_bloques_recursivo()
    - obtener_nRangoBL()
    - liberar_bloque()
    - obtener_indice()
**/
int contadorRead, contadorWrite;
int liberar_bloques_inodo (unsigned int primerBL, struct inodo *inodo) {
    // Declaramos las variables del método
    unsigned int nivel_punteros = 0;
    unsigned int nBL = primerBL; 
    unsigned int ultimoBL;
    unsigned int ptr = 0;
    int nRangoBL = 0;
    int liberados = 0;
    int eof = 0;

    // Comprobamos si el fichero está totalmente vacío
    if (inodo->tamEnBytesLog == 0) {
        fprintf(stderr, LGREEN "El fichero está vacío, no hay bloques por liberar" RESET);
        return EXITO;
    }
    // Calculamos el último BL del inodo
    if ((inodo->tamEnBytesLog % BLOCKSIZE) == 0) {
        ultimoBL = (inodo->tamEnBytesLog / BLOCKSIZE) - 1;
    } else {
        ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE;
    }

    #if DEBUGN6==1
        fprintf(stderr, NEGRITA BLUE"[liberar_bloques_inodo()→ primer BL: %d, último BL: %d]\n" RESET, nBL, ultimoBL);
    #endif
    // Obtenemos el rango del bloque lógico
    nRangoBL=obtener_nRangoBL(inodo, nBL, &ptr);
    // Si nos encontramos en el rango de los Directos
    if (nRangoBL == 0) {
        liberados+=liberar_directos(&nBL, ultimoBL, inodo, &eof);
    }
    // Mientras no hayamos llegado al final del fichero
    while (!eof){
        nRangoBL=obtener_nRangoBL(inodo, nBL, &ptr);
        nivel_punteros=nRangoBL;
        // Liberamos los bloques del rango de los Indirectos de manera recursiva
        liberados += liberar_indirectos_recursivo(&nBL, primerBL, ultimoBL, inodo, nRangoBL, nivel_punteros, &ptr,  &eof);
    }
    #if DEBUGN6==1
        fprintf(stderr, NEGRITA BLUE "[liberar_bloques_inodo()→ total bloques liberados: %d, total_breads: %d, total_bwrites: %d]\n" RESET, liberados, contadorRead, contadorWrite);
    #endif
    return liberados;
}
// Declaramos variables auxiliares para los comentarios por pantalla
int numBF, numBL;

/**
Función: Auxiliar recursiva a la función liberar_bloques_inodo que libera los directos
Parámetros:
    - *nBL: Puntero al bloque lógico tratado
    - *inodo: Inodo tratado
    - *eof: Puntero al End Of File
    - ultimoBL: Valor del último bloque lógico
Llamado por:
    - liberar_bloques_inodo()
Llama a: 
    - liberar_bloque
Salida: Los bloques liberados
**/
int liberar_directos(unsigned int *nBL, unsigned int ultimoBL, struct inodo *inodo, int *eof) {
    int liberados=0;
    // Recorremos todos los Directos
    for (int d = *nBL; d < DIRECTOS && !*eof; d++) {
        // Comprobamos si el puntero NO está vacío
        if (inodo->punterosDirectos[*nBL] != 0) {
            numBF=liberar_bloque(inodo->punterosDirectos[*nBL]);
            numBL=*nBL;
            #if DEBUGN6==1
                fprintf(stderr, GRAY "[liberar_bloques_inodo()→liberado BF %d de datos para BL %d]\n" RESET, numBF, *nBL);
            #endif
            inodo->punterosDirectos[*nBL] = 0;
            liberados++;
        }
        // Avanzamos en los punteros Directos
        *nBL = *nBL+1;  
        // Si llegamos al fin de los punteros, ponemos eof a 1
        if (*nBL > ultimoBL) *eof = 1;
    }
    return liberados;
}

/**
Función: Auxiliar recursiva a la función liberar_bloques_inodo
Parámetros:
    - *nBL: Puntero al bloque lógico tratado
    - primerBL: Valor del primer bloque lógico
    - ultimoBL: Valor del último bloque lógico
    - *inodo: Inodo tratado
    - nRangoBL: Rango del bloque lógico tratado
    - nivel_punteros: Valor que indica el nivel de los punteros tratados
    - *ptr: Puntero
    - *eof: Puntero al End Of File
Salida: Los bloques liberados
**/
int liberar_indirectos_recursivo(unsigned int *nBL, unsigned int primerBL, unsigned int ultimoBL, 
 struct inodo *inodo, int nRangoBL, unsigned int nivel_punteros, unsigned int *ptr, int *eof) { 
    // Declaramos las variables del método
    int liberados = 0 , indice_inicial = 0;
    unsigned int bloquePunteros[NPUNTEROS], bloquePunteros_Aux[NPUNTEROS], bufferCeros[NPUNTEROS];

    // Inicializamos a 0s los buffers
    // if (memset(bloquePunteros, 0, BLOCKSIZE) == NULL) {
    //     fprintf(stderr, RED "Error al reservar memoria \n" RESET);
    //     return FALLO;
    // } 
    if (memset(bufferCeros, 0, BLOCKSIZE) == NULL) {
        fprintf(stderr, RED "Error al reservar memoria \n" RESET);
        return FALLO;
    }
    // Si cuelga un bloque de punteros
    if (*ptr != 0) {
        indice_inicial=obtener_indice(*nBL,nivel_punteros);
        // Solo leemos bloque si no estaba cargado
        if ((indice_inicial==0)||(*nBL==primerBL)) {
            if (bread(*ptr, bloquePunteros) == FALLO) return FALLO;
            // Actualizamos el contador de bread()
            contadorRead++;
            // Guardamos copia del bloque en cuestión para no perder los datos
            if (memcpy(bloquePunteros_Aux, bloquePunteros, BLOCKSIZE) == NULL) {
                fprintf(stderr, RED "Error al copiar al buffer auxiliar \n" RESET);
                return FALLO;
            }
        }
        // Exploramos el bloque de punteros iterando el índice
        for (int i=indice_inicial; i < NPUNTEROS && !*eof;  i++) {
            // Si el puntero no está vacío
            if (bloquePunteros[i] != 0) {              
                if (nivel_punteros == 1) {
                    // Liberamos el bloque
                    numBF=liberar_bloque(bloquePunteros[i]);
                    numBL=*nBL;
                    #if DEBUGN6==1
                        fprintf(stderr, GRAY "[liberar_bloques_inodo()→liberado BF %d de datos para BL %d]\n" RESET, numBF, numBL);
                    #endif
                    bloquePunteros[i]=0;
                    liberados++;
                    *nBL = *nBL+1;
                } else {
                    // Llamada recursiva para explorar el nivel siguiente de punteros hacia los datos
                    liberados += liberar_indirectos_recursivo(nBL, primerBL, ultimoBL, inodo, nRangoBL, nivel_punteros-1, &bloquePunteros[i], eof);
                }
            } else {
                // Actualizamos el bloque lógico tratado
                switch (nivel_punteros) {
                    case 1: *nBL = *nBL+1; break;
                    case 2: *nBL += NPUNTEROS; break;
                    case 3: *nBL += NPUNTEROS*NPUNTEROS; break;
                }   
            }
            // Comprobamos si hemos llegado al fin del archivo
            if (*nBL > ultimoBL) *eof = 1;
        }
        // Si el bloque de punteros es distinto al original
        if (memcmp(bloquePunteros, bloquePunteros_Aux, BLOCKSIZE) != 0) {  
            // Si quedan punteros != 0 en el bloque lo salvamos
            if (memcmp(bloquePunteros, bufferCeros, BLOCKSIZE) != 0) {
                bwrite(*ptr, bloquePunteros);
                // Actualizamos el contador de bwrite()
                contadorWrite++;
            } else {
                // Liberamos el bloque al que apunta ptr
                numBF=liberar_bloque(*ptr);
                numBL=*nBL;
                #if DEBUGN6==1
                    fprintf(stderr, GRAY "[liberar_bloques_inodo()→liberado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, *ptr, nivel_punteros, numBL);
                #endif
                // Ponemos a 0 el puntero que apuntaba al bloque liberado
                *ptr=0; 
                liberados++;
            }
        }
    } else {
        // Sólo entrará si es un puntero del inodo, resto de casos sólo se llama recursivamente con *ptr!=0
        // Actualizamos el bloque lógico tratado
        switch (nRangoBL) {
            case 1: *nBL = INDIRECTOS0; break;
            case 2: *nBL = INDIRECTOS1; break;
            case 3: *nBL = INDIRECTOS2; break;
        }
    }
    return liberados;
}