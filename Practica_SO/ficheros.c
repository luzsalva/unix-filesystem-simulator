// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López

#include "ficheros.h"

///////////////////////////////////////////////////////////////////
//                                                               //
//                            NIVEL 5                            //
//                                                               //
///////////////////////////////////////////////////////////////////

/**
Función: Escribe el contenido de un buffer de memoria en un fichero/directorio
Parámetros: 
    - ninodo:número de inodo correspondiente al fichero/directorio que escribiremos 
    - *buf_original: contiene el texto a volcar. Puede tener el tamaño que sea
    - offset: desplazamiento
    - nbytes: bytes a escribir 
Salida: cantidad de bytes escritos REALMENTE
Llamado por: 
    - escribir.c
    - mi_write()
    - mi_unlink()
Llama a: 
    - leer_inodo()
    - traducir_bloque_inodo()
    - bread()
    - bwrite()
    - escribir_inodo()
**/
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes) {
    struct inodo inodo; 
    if (leer_inodo(ninodo, &inodo)==FALLO) return FALLO;
    
    // Revisamos que tenga permiso de escritura
    if ((inodo.permisos & 2) != 2) {
       fprintf(stderr, RED "No hay permisos de escritura\n" RESET);
       return FALLO;
    }
    
    // Calculamos cuál va a ser el primer bloque lógico donde hay que escribir
    int primerBL = offset / BLOCKSIZE;
    // Calculamos también el último bloque lógico donde hay que escribir
    int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;

    // Calculamos el desplazamiento desp1 en el bloque para el offset
    int desp1 = offset % BLOCKSIZE;
    // Calculamos el desplazamiento desp2 en el bloque para ver donde llegan los nbytes escritos a partir del offset
    int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    int nbfisico;
    unsigned char buf_bloque[BLOCKSIZE];
    int bytesEscritos=0;

    if (primerBL==ultimoBL) {
        // Obtenemos el nº de bloque físico
        mi_waitSem();
        nbfisico=traducir_bloque_inodo(ninodo, primerBL, 1);
        mi_signalSem();

        // Leemos ese bloque físico del dispositivo virtual y lo almacenamos en buf_bloque
        if (bread(nbfisico, buf_bloque)==FALLO) return FALLO;

        // Escribimos los nbytes en el buf_bloque
        if (memcpy(buf_bloque + desp1, buf_original, nbytes)==NULL) {
            fprintf(stderr, RED "Error al copiar los datos en el buffer \n" RESET);
            return FALLO;
        }

        // Escribimos buf_bloque modificado en el nº de bloque físico correspondiente
        if (bwrite(nbfisico,buf_bloque)==FALLO) return FALLO;
        bytesEscritos = desp2 - desp1 + 1;
    
    } else {
        // Obtenemos el nº de bloque físico
        mi_waitSem();
        nbfisico=traducir_bloque_inodo(ninodo, primerBL, 1);
        mi_signalSem();

        // Leemos ese bloque físico del dispositivo virtual y lo almacenamos en buf_bloque
        if (bread(nbfisico, buf_bloque)==FALLO) return FALLO;

        // Calculamos los bytes que vamos a escribir
        bytesEscritos = BLOCKSIZE - desp1;
        
        // Copiamos los bytes restantes en buf_bloque en la posición indicada por desp1
        if (memcpy (buf_bloque + desp1, buf_original, bytesEscritos)==NULL) {
            fprintf(stderr, RED "Error al copiar los datos en el buffer \n" RESET);
            return FALLO;
        }

        // Escribimos buf_bloque modificado en el nº de bloque físico correspondiente
        if(bwrite(nbfisico,buf_bloque) == FALLO) return FALLO;

        // Procedemos con los bloques intermedios
        for (int bl=primerBL+1; bl<ultimoBL; bl++) {
            // Traducimos cada bloque lógico a físico
            mi_waitSem();
            nbfisico=traducir_bloque_inodo(ninodo, bl, 1);
            mi_signalSem();
            
            // Y escribimos en cada uno
            if(bwrite(nbfisico,buf_original + (BLOCKSIZE - desp1) + (bl - primerBL - 1) * BLOCKSIZE) == FALLO) return FALLO;
            
            // Acumulamos los bytes escritos
            bytesEscritos = bytesEscritos+BLOCKSIZE;   
        }
        
        // Abordamos el último bloque lógico
        mi_waitSem();
        nbfisico=traducir_bloque_inodo(ninodo, ultimoBL, 1);
        mi_signalSem();
        if (bread(nbfisico, buf_bloque) == FALLO) return FALLO;

        // Copiamos los bytes de desp2 a buf_bloque
        if (memcpy(buf_bloque, buf_original + (nbytes - (desp2 + 1)), desp2 + 1) == NULL) {
            fprintf(stderr, RED "Error al copiar los datos en el buffer \n" RESET);
            return FALLO;
        }

        if (bwrite(nbfisico, buf_bloque) == FALLO) return FALLO;

        // Acumulamos los bytes escritos
        bytesEscritos = bytesEscritos + (desp2 + 1);
    }

    mi_waitSem();

    if (leer_inodo(ninodo,&inodo) == FALLO) return FALLO;
    // Actualizamos la metainformación del inodo
    if (inodo.tamEnBytesLog < (bytesEscritos + offset)) {
        inodo.tamEnBytesLog = bytesEscritos + offset;
        // Actualizamos el time de la última modificación del inodo 
        inodo.ctime=time(NULL);
    }
    // Actualizamos el time de la última modificación de datos
    inodo.mtime=time(NULL);

    // Guardamos el inodo
    if(escribir_inodo(ninodo,&inodo) == FALLO) return FALLO;

    mi_signalSem();
    
    return bytesEscritos;
}

/**
Función: Lee la información de un fichero/directorio
Parámetros: 
    - ninodo: número de inodo a leer
    - buf_original: buffer donde se almacena la información leída
    - offset: posición de lectura inicial en bytes
    - nbytes: número de bytes a leer
Salida: Devuelve la cantidad de bytes leídos
Llamado por: 
    - leer.c
    - truncar.c
    - buscar_entrada()
    - mi_link()
    - mi_unlink()
Llama a: 
    - bread()
    - leer_inodo()
    - escribir_inodo()
    - traducir_bloque_inodo()
**/
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes) {
    struct inodo inodo; 
    
    mi_waitSem();

    if (leer_inodo(ninodo, &inodo)==FALLO) return FALLO;

    // Actualizamos atime
    inodo.atime=time(NULL);
    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;

    mi_signalSem();

    // Revisamos que tenga permiso de lectura
    if ((inodo.permisos & 4) != 4) {
       fprintf(stderr, RED "No hay permisos de lectura\n" RESET);
       return FALLO;
    }
    int bytesLeidos=0; 
    if (offset >= inodo.tamEnBytesLog) {
        // No podemos leer nada
        bytesLeidos = 0;
        return bytesLeidos;
    }
    // Quiere leer más allá de EOF
    if ((offset + nbytes) >= inodo.tamEnBytesLog) { 
        nbytes = inodo.tamEnBytesLog - offset;  
    }
        // Calculamos cuál va a ser el primer bloque lógico donde hay que escribir
        int primerBL = offset / BLOCKSIZE;
        // Calculamos también el último bloque lógico donde hay que escribir
        int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;

        // Calculamos el desplazamiento desp1 en el bloque para el offset
        int desp1 = offset % BLOCKSIZE;
        // Calculamos el desplazamiento desp2 en el bloque para ver donde llegan los nbytes escritos a partir del offset
        int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

        int nbfisico;
        unsigned char buf_bloque[BLOCKSIZE];

        if (primerBL==ultimoBL) {
            // Obtenemos el nº de bloque físico
            nbfisico=traducir_bloque_inodo(ninodo, primerBL, 0);
            
            // Si existe el bloque físico, nbfisico, asignado al bloque lógico que pretendemos leer
            if (nbfisico!=FALLO){
                // Leemos ese bloque físico del dispositivo virtual y lo almacenamos en buf_bloque
                if (bread(nbfisico, buf_bloque)==FALLO) return FALLO;

                // Copiar el contenido de buf_bloque+desp1 al buffer original
                if (memcpy(buf_original, buf_bloque + desp1,  nbytes)==NULL) {
                    fprintf(stderr, RED "Error al copiar los datos en el buffer \n" RESET);
                    return FALLO;
                }
            }
            bytesLeidos = nbytes;
        } else {
            // Obtenemos el nº de bloque físico
            nbfisico=traducir_bloque_inodo(ninodo, primerBL, 0);

            // Si existe el bloque físico, nbfisico, asignado al bloque lógico que pretendemos leer
            if (nbfisico!=FALLO){
                // Leemos ese bloque físico del dispositivo virtual y lo almacenamos en buf_bloque
                if (bread(nbfisico, buf_bloque)==FALLO) return FALLO;

                // Copiar el contenido de buf_bloque+desp1 al buffer original
                if (memcpy(buf_original, buf_bloque + desp1,  BLOCKSIZE - desp1)==NULL) {
                    fprintf(stderr, RED "Error al copiar los datos en el buffer \n" RESET);
                    return FALLO;
                }
            }
            // Calculamos los bytes que vamos a leer
            bytesLeidos=BLOCKSIZE - desp1;
            
            // Procedemos con los bloques intermedios
            for (int bl=primerBL+1; bl<ultimoBL; bl++) {
                // Traducimos cada bloque lógico a físico
                nbfisico=traducir_bloque_inodo(ninodo, bl, 0);
                
                if (nbfisico!=FALLO) {
                    // Y escribimos en cada uno
                    if(bread(nbfisico,buf_bloque) == FALLO) return FALLO;
                    // Copiamos los bytes restantes en buf_bloque en la posición indicada por desp1
                    if (memcpy(buf_original + (BLOCKSIZE - desp1) + (bl - primerBL - 1) * BLOCKSIZE, buf_bloque, BLOCKSIZE)==NULL) {
                        fprintf(stderr, RED "Error al copiar los datos en el buffer \n" RESET);
                        return FALLO;
                    }
                }
                // Acumulamos los bytes leidos
                bytesLeidos+=BLOCKSIZE; 
            }
            
            // Abordamos el último bloque lógico
            nbfisico=traducir_bloque_inodo(ninodo, ultimoBL, 0);

            if (nbfisico!=FALLO) {
                if (bread(nbfisico, buf_bloque) == FALLO) return FALLO;

                // Copiamos los bytes de desp2 a buf_bloque
                if (memcpy(buf_original + (nbytes - (desp2 + 1)), buf_bloque,  desp2 + 1) == NULL) {
                    fprintf(stderr, RED "Error al copiar los datos en el buffer \n" RESET);
                    return FALLO;
                }
            }
            // Acumulamos los bytes leidos
            bytesLeidos+= desp2 + 1;
    }
    return bytesLeidos;
}

/**
Función: Devuelve la metainformación de un fichero/directorio
Parámetros: 
    - nbloques: Número total de bloques en el sistema de archivos
    - *p_stat: struct STAT donde se guardan los datos
Salida: EXITO/FALLO
Llamado por: 
    - escribir.c
    - truncar.c
Llama a:
    - leer_inodo()
**/
int mi_stat_f(unsigned int ninodo, struct STAT *p_stat) {
    struct inodo inodo; 
    if (leer_inodo(ninodo, &inodo)==FALLO) return FALLO;
    struct tm *ts;
    char atime[100];
    char mtime[100];
    char ctime[100];
    ts = localtime(&inodo.atime);
    strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.mtime);
    strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
    ts = localtime(&inodo.ctime);
    strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
    
    // Asignamos las stats del inodo a la estructura STAT
    p_stat->tipo = inodo.tipo;
    p_stat->permisos = inodo.permisos;
    p_stat->atime= inodo.atime; 
    p_stat->mtime=inodo.mtime; 
    p_stat->ctime= inodo.ctime; 
    p_stat->nlinks= inodo.nlinks;           
    p_stat->tamEnBytesLog = inodo.tamEnBytesLog; 
    p_stat->numBloquesOcupados = inodo.numBloquesOcupados;
    
    return EXITO;
}

/**
Función: Cambia los permisos de un fichero/directorio con el valor que indique el argumento permisos
Parámetros: 
    - nbloques: Número total de bloques en el sistema de archivos
    - permisos: Los permisos que se deben modificar del fichero
Salida: EXITO/FALLO
Llamado por: 
    - permitir.c
Llama a: 
    - leer_inodo()
    - escribir_inodo()
**/
int mi_chmod_f(unsigned int ninodo, unsigned char permisos) {
    struct inodo inodo; 
    
    mi_waitSem();
    
    if (leer_inodo(ninodo, &inodo)==FALLO) return FALLO;

    // Actualizamos ctime
    inodo.ctime=time(NULL);
    // Actualizamos los permisos del inodo
    inodo.permisos=permisos;
    
    //Guardamos el inodo
    if (escribir_inodo(ninodo, &inodo) == FALLO) return FALLO;

    mi_signalSem();

    return EXITO;
}

///////////////////////////////////////////////////////////////////
//                                                               //
//                            NIVEL 6                            //
//                                                               //
///////////////////////////////////////////////////////////////////

/**
Función: Trunca un fichero/directorio apartir del byte indicado
Parámetros: 
    - ninodo: número de inodo que se quiere truncar
    - nbytes: número de byte donde se quiere empezar a liberar
Salida: Devuelve la cantidad de bloques liberados
Llamado por: 
    - truncar.c
    - mi_unlink()
Llama a: 
    - leer_inodo()
    - escribir_inodo()
    - liberar_bloques_inodo()
    
**/
int mi_truncar_f(unsigned int ninodo, unsigned int nbytes) {
    struct inodo inodo;
    if (leer_inodo(ninodo, &inodo) == FALLO) return FALLO;

    // Revisamos que tenga permiso de escritura
    if ((inodo.permisos & 2) != 2) {
       fprintf(stderr, RED "No hay permisos de escritura\n" RESET);
       return FALLO;
    }
    // Comprobamos que no truncamos más allá del EOF
    if (inodo.tamEnBytesLog < nbytes) {
        fprintf(stderr, RED "No se puede truncar más allá del EOF\n" RESET);
        return FALLO;
    }
    unsigned int primerBL;
    // Calculamos el número del primer bloque lógico a liberar (nBL)
    if (nbytes % BLOCKSIZE == 0) primerBL = nbytes/BLOCKSIZE;
    else primerBL = (nbytes/BLOCKSIZE) + 1;

    // Llamamos a liberar_bloques_inodo para truncar
    int liberados=liberar_bloques_inodo(primerBL, &inodo);
    if (liberados==FALLO) {
        fprintf(stderr, RED "No se han podido liberar los bloques\n" RESET);
        return FALLO;
    }

    // Actualizamos mtime y ctime del inodo
    inodo.mtime=time(NULL);
    inodo.ctime=time(NULL);

    // Actualizamos el tamaño de bytes y el número de bloques ocupados
    inodo.tamEnBytesLog = nbytes;
    inodo.numBloquesOcupados -= liberados;

    // Salvamos el inodo
    if (escribir_inodo(ninodo, &inodo)==FALLO) return FALLO;

    return liberados; 
}