// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López
#include "directorios.h"

// Variable global que guarda la última entrada para escritura
static struct UltimaEntrada UltimaEntradaEscritura;

// Variable global que guarda la última entrada para lectura
static struct UltimaEntrada UltimaEntradaLectura;

// Variable global que marca la posición de la entrada que hay que eliminar/añadir
int posicion=0;

//tabla caché directorios
#if (USARCACHE==2 || USARCACHE==3)
    #define CACHE_SIZE 3 // cantidad de entradas para la caché
    static struct UltimaEntrada UltimasEntradas[CACHE_SIZE];
#endif

///////////////////////////////////////////////////////////////////
//                                                               //
//                            NIVEL 7                            //
//                                                               //
///////////////////////////////////////////////////////////////////

/**
Función: Dada una cadena de caracteres separa su contenido en dos 
Parámetros:
    - *camino: cadena de caracteres (comienza en '/')
    - *inicial: porción de *camino comprendida entre los dos primeros '/'
    - *final: directorio: resto de *camino a partir del segundo '/' inclusive
    - *tipo: ‘d’ o ‘f’ en función de si en *inicial hay un nombre de directorio o fichero
Salida: FALLO/EXITO
Llamado por: 
    - buscar_entrada()
Llama a: N/A
**/
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
    // Comprobamos que el primer caracter del camino sea "/"

    if (camino[0] != '/') {
        return FALLO;
    }

    int nCaracter = 1;
    // Avanzamos en el camino
    while (camino[nCaracter] != '/' && camino[nCaracter] != '\0') {
        nCaracter++;
    }

    // Copiamos el primer aarchivo en inicial
    strncpy(inicial, camino + 1, nCaracter - 1);
    inicial[nCaracter - 1] = '\0';  // Aseguramos la terminación nula

    // Si el último caracter es / o \0
    if (camino[nCaracter] == '/') {
        *tipo = 'd';
        strcpy(final, camino + nCaracter);
    } else {
        *tipo = 'f';
        *final = '\0';
    }

    return EXITO;
} 

/**
Función: Busca una entrada determinada
Parámetros:
    - *camino_parcial: Ruta parcial de la entrada a buscar
    - *p_inodo_dir: Puntero al número de inodo del directorio padre
    - *p_inodo: Puntero al número de inodo
    - *p_entrada: Puntero al número de la entrada
    - reservar: Indica si se ha de crear una nueva entrada si no existe
    - permisos: Permisos de la entrada
Salida: EXITO/FALLO
Llamado por:
    - mostrar_buscar_entrada()
    - buscar_entrada()
    - mi_creat()
    - mi_dir()
    - mi_chmod()
    - mi_stat()
    - mi_write()
    - mi_read()
    - cacheFIFO()
    - cacheLRU()
    - mi_link()
    - mi_unlink()
Llama a:
    - bread()
    - extraer_camino()
    - leer_inodo()
    - mi_read_f()
    - reservar_inodo()
    - mi_write_f()
    - liberar_inodo()
    - buscar_entrada()
**/
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos) {
    struct entrada entrada;
    struct inodo inodo_dir;
    char inicial[sizeof(entrada.nombre)];
    char final[strlen(camino_parcial)+1];
    char tipo;
    int cant_entradas_inodo, num_entrada_inodo;
    unsigned int offset=0;

    //inicializar el buffer de lectura con 0s
    // struct entrada entradas[BLOCKSIZE/sizeof(struct entrada)];
    // if (memset(entradas, 0, sizeof(struct entrada)) == NULL) {
    //     fprintf(stderr, RED "Error al reservar memoria \n" RESET);
    //     return FALLO;
    // }

    struct superbloque SB;
    if (bread(posSB, &SB)==FALLO) return FALLO;
    
    // Camino_parcial es “/”
    if (strcmp(camino_parcial,"/") == 0) { 
        // Nuestra raiz siempre estará asociada al inodo 0
        *p_inodo=SB.posInodoRaiz;  
        *p_entrada=0;
        return EXITO;
    }
    memset(inicial, 0, sizeof(entrada.nombre));
    memset(final, 0, strlen(camino_parcial)+1);
    int errorCamino = extraer_camino(camino_parcial, inicial, final, &tipo);
    if (errorCamino == FALLO) {
        return ERROR_CAMINO_INCORRECTO;
    }
    #if DEBUGN8==1
        fprintf(stderr, GRAY "[buscar_entrada() → inicial: %s, final: %s, reservar: %d]\n" RESET,inicial,final,reservar);
    #endif

    // Buscamos la entrada cuyo nombre se encuentra en inicial
    leer_inodo(*p_inodo_dir, &inodo_dir);
    if ((inodo_dir.permisos & 4) != 4) {
        return ERROR_PERMISO_LECTURA;
    }
    
    // Cantidad de entradas que contiene el inodo
    cant_entradas_inodo = inodo_dir.tamEnBytesLog/sizeof(struct entrada); 
    // Nº de entrada inicial
    num_entrada_inodo = 0;  
    if (cant_entradas_inodo > 0) {
        // Leer entrada 
        mi_read_f(*p_inodo_dir, &entrada, offset, sizeof(struct entrada));
        // Leer el resto de entradas
        while ((num_entrada_inodo < cant_entradas_inodo) && strcmp(entrada.nombre, inicial)!=0) { 
            num_entrada_inodo++;
            offset+= sizeof(struct entrada);
            // Previamente volver a inicializar el buffer de lectura con 0s
            memset(entrada.nombre, 0, sizeof(struct entrada));
            mi_read_f(*p_inodo_dir, &entrada, offset, sizeof(struct entrada));
        }
    }

    if (strcmp(entrada.nombre, inicial) != 0 && (num_entrada_inodo == cant_entradas_inodo)) {
        // La entrada no existe
        switch (reservar) {
            case 0:  
                // Modo consulta. Como no existe retornamos error
                return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
                break;
            case 1:
                // Creamos la entrada en el directorio referenciado por *p_inodo_dir
                // si es fichero no permitir escritura
                if (inodo_dir.tipo == 'f') {
                    return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
                }
                // Si es directorio comprobar que tiene permiso de escritura
                if ((inodo_dir.permisos & 2) != 2) {
                    return ERROR_PERMISO_ESCRITURA;
                } else {
                    strcpy(entrada.nombre, inicial);
                    if (tipo == 'd') {
                        // Limpiar espacios o caracteres de nueva línea
                        if (strcmp(final, "/") == 0) {
                            // Reservar un inodo como directorio y asignarlo a la entrada
                            entrada.ninodo = reservar_inodo('d', permisos); 
                            #if DEBUGN8==1
                                fprintf(stderr, GRAY"[buscar_entrada() → reservar inodo %d tipo %c con permisos %d para %s]\n"RESET, entrada.ninodo, tipo, permisos, inicial);
                            #endif
                        // No es el final de la ruta
                        } else { 
                            return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
                        }
                    // Es un fichero
                    } else { 
                        // Reservar un inodo como fichero y asignarlo a la entrada
                        entrada.ninodo = reservar_inodo('f',6); 
                        #if DEBUGN8==1   
                            fprintf(stderr, GRAY"[buscar_entrada() → reservar inodo %d tipo %c con permisos %d para %s]\n"RESET, entrada.ninodo, tipo, permisos, inicial);
                        #endif
                    }
                    // Escribir la entrada en el directorio padre
                    int errorEscritura=mi_write_f(*p_inodo_dir, &entrada, num_entrada_inodo * sizeof(struct entrada), sizeof(struct entrada));
                    if (errorEscritura == FALLO) {
                        if (entrada.ninodo != -1) {
                            liberar_inodo(entrada.ninodo);
                        }
                        return FALLO;
                    }
                    #if DEBUGN8==1
                        fprintf(stderr, GRAY "[buscar_entrada() → creada entrada; %s, %d]\n" RESET, inicial, entrada.ninodo);
                    #endif
                } 
                break;
        }
    }
    
    // Si hemos llegado al final del camino
    if (!strcmp(final, "/") || !strcmp(final, "")) {
        if ((num_entrada_inodo < cant_entradas_inodo) && (reservar==1)) {
        // Modo escritura y la entrada ya existe
           return ERROR_ENTRADA_YA_EXISTENTE;
        }
        // Cortamos la recursividad
        *p_inodo = entrada.ninodo;
        *p_entrada = num_entrada_inodo;
        
        return EXITO;
    } else {
        *p_inodo_dir = entrada.ninodo;
        *p_inodo = 0;
        *p_entrada = 0;
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }
}

void mostrar_error_buscar_entrada(int error) {
    switch (error) {
        case -2: fprintf(stderr, RED "Error: Camino incorrecto.\n" RESET); break;
        case -3: fprintf(stderr, RED "Error: Permiso denegado de lectura.\n" RESET); break;
        case -4: fprintf(stderr, RED "Error: No existe el archivo o el directorio.\n" RESET); break;
        case -5: fprintf(stderr, RED "Error: No existe algún directorio intermedio.\n" RESET); break;
        case -6: fprintf(stderr, RED "Error: Permiso denegado de escritura.\n" RESET); break;
        case -7: fprintf(stderr, RED "Error: El archivo ya existe.\n" RESET); break;
        case -8: fprintf(stderr, RED "Error: No es un directorio.\n" RESET); break;
    }
}

///////////////////////////////////////////////////////////////////
//                                                               //
//                            NIVEL 8                            //
//                                                               //
///////////////////////////////////////////////////////////////////
/**
Función: Crea un fichero o directorio y su entrada de directorio
Parámetros:
    - *camino: Ruta del fichero o directorio a crear
    - permisos: Permisos del fichero o directorio creado
Salida: Si ha habido algun error
Llamado por: 
    - mi_mkdir.c
    - mi_touch.c
    - verificacion.c
    - simulacion.c
Llama a:
    - buscar_entrada()
    - mostrar_error_buscar_entrada()
**/
int mi_creat(const char *camino, unsigned char permisos) {
    mi_waitSem();
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    
    int errorEntrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);
    if (errorEntrada < EXITO) {
        mostrar_error_buscar_entrada(errorEntrada);
        mi_signalSem();
        return FALLO;
    }

    mi_signalSem();
    return errorEntrada;
}

/**
Función: Pone el contenido del directorio en un buffer de memoria (el nombre de cada entrada puede venir separado por '|' o por un tabulador)
Parámetros:
    - *camino: Camino correspondiente a la entrada a examinar
    - *buffer: Buffer donde se guarda el texto a mostrar por pantalla
    - tipo: Directorio (d) o Fichero (f)
    - extendido: Booleano que marca si es la versión extendida de mi_ls.c
Salida: EXITO/FALLO
Llamado por:
    - mi_ls.c
Llama a:
    - buscar_entrada()
    - mostrar_error_buscar_entrada()
    - leer_inodo()
    - mi_read_f()
**/
int mi_dir(const char *camino, char *buffer, char tipo, bool extendido) {
    struct inodo inodo;
    struct inodo inodoEntrada;
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int num_entrada_inodo;
    int cant_entradas_inodo;
    struct entrada entrada;
    char bytes[TAMFILA];

    // Buscamos la entrada correspondiente a *camino para comprobar que existe
    int errorEntrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 6);
    if (errorEntrada < EXITO)  {
        mostrar_error_buscar_entrada(errorEntrada);
        return FALLO;
    }

    // Leemos su inodo, comprobando que se trata de un directorio y que tiene permisos de lectura
    if (leer_inodo(p_inodo, &inodo) == FALLO) return FALLO;
    if ((inodo.permisos & 4) != 4) {
        fprintf(stderr, RED"Error: El inodo no tiene permisos de lectura\n"RESET);
        return FALLO;
    }
    if (inodo.tipo != tipo){
        fprintf(stderr, "Error: El tipo de la entrada no concuerda, sintaxis incorrecta");
        return FALLO;
    }
    // Calculamos la cantidad de entradas que contiene el inodo
    cant_entradas_inodo = inodo.tamEnBytesLog/sizeof(struct entrada);
    // Calculamos el número de entrada incial del inodo
    num_entrada_inodo = 0;
    char tmp[TAMFILA]; 
    
    // Si es de tipo Directorio
    if (inodo.tipo=='d') {
        fprintf(stderr, "TOTAL: %d\n", cant_entradas_inodo);

        // Guardamos en el buffer la línea correspondiente al formato
        if ((cant_entradas_inodo > 0) && (extendido)) {
            strcat(buffer, "Tipo\tPermisos\tmTime\t\t\tTamaño\tNombre\n");
            strcat(buffer, "--------------------------------------------------------------------------------\n");
        }

        // Mientras el número de entrada tratado no sobrepase el número total de entradas del inodo
        while (num_entrada_inodo < cant_entradas_inodo) { 
            // Calculamos el offset
            int offset = num_entrada_inodo * sizeof(entrada);

            // Leemos la entrada
            if (mi_read_f(p_inodo, &entrada, offset, sizeof(entrada)) == FALLO) return FALLO;
            if (entrada.ninodo >= 0) {
                // Leemos el inodo correspondiente a la entrada a tratar
                if (leer_inodo(entrada.ninodo, &inodoEntrada) == FALLO) return FALLO;
                // Guardamos en el buffer el tipo: si es Directorio o Fichero
                if (extendido) {
                    if (inodoEntrada.tipo == 'd') {
                        strcat(buffer, "d");
                    } else if (inodoEntrada.tipo == 'f') {
                        strcat(buffer, "f");
                    }
                    strcat(buffer, "\t");
                
                    // Guardamos en el buffer el modo (permisos)
                    if (inodoEntrada.permisos & 4) strcat(buffer, "r"); else strcat(buffer, "-");
                    if (inodoEntrada.permisos & 2) strcat(buffer, "w"); else strcat(buffer, "-");
                    if (inodoEntrada.permisos & 1) strcat(buffer, "x"); else strcat(buffer, "-");
                    strcat(buffer, "\t\t");

                    // Guardamos en el buffer el mTime
                    struct tm *tm; 
                    tm = localtime(&inodoEntrada.mtime);
                    char tmp[100];
                    sprintf(tmp, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,  tm->tm_sec);
                    strcat(buffer, tmp);
                    strcat(buffer, "\t");
                    
                    // Guardamos en el buffer el tamaño de la entrada
                    memset(bytes, '\0', sizeof(bytes));
                    sprintf(bytes, "%d", inodoEntrada.tamEnBytesLog);
                    strcat(buffer, bytes);
                    strcat(buffer, "\t");
                }
            
                // Guardamos en el buffer el nombre de la entrada
                if(inodoEntrada.tipo == 'd') {   
                    sprintf(tmp, ROSE"%s"RESET,entrada.nombre);
                    strcat(buffer, tmp);
                } else if(inodoEntrada.tipo == 'f') {
                    sprintf(tmp, YELLOW"%s"RESET,entrada.nombre);
                    strcat(buffer, tmp);
                }
                strcat(buffer, "\n");

                // Actualizamos el número de entrada
                num_entrada_inodo++;
            } else {
                return EXITO;
            }
        }
        // En cambio, si es de tipo Fichero
    } else if (inodo.tipo=='f') {
        // Guardamos en el buffer la línea correspondiente al formato
        if (extendido) {
            strcat(buffer, "Tipo\tPermisos\tmTime\t\t\tTamaño\tNombre\n");
            strcat(buffer, "------------------------------------------------------\n");
        }
        // Calculamos el offset
        int offset = num_entrada_inodo * sizeof(entrada);
        // Leemos la entrada
        if (mi_read_f(p_inodo_dir, &entrada, offset, sizeof(entrada)) == FALLO) return FALLO;
        // Leemos el inodo correspondiente a la entrada a tratar
        if (leer_inodo(entrada.ninodo, &inodoEntrada) == FALLO) return FALLO;
        // Si es el formato extendido, guardamos en el buffer el resto de información, igual que se hizo anteriormente en el caso Directorio
        if (extendido) {
            strcat(buffer, "f");
            strcat(buffer, "\t");
            // Imprimimos por pantalla el modo (permisos)
            if (inodoEntrada.permisos & 4) strcat(buffer, "r"); else strcat(buffer, "-");
            if (inodoEntrada.permisos & 2) strcat(buffer, "w"); else strcat(buffer, "-");
            if (inodoEntrada.permisos & 1) strcat(buffer, "x"); else strcat(buffer, "-");
            strcat(buffer, "\t\t");

            // Guardamos en el buffer el mTime
            struct tm *tm; 
            tm = localtime(&inodoEntrada.mtime);
            char tmp[100];
            sprintf(tmp, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min,  tm->tm_sec);
            strcat(buffer, tmp);
            strcat(buffer, "\t");

            // Guardamos en el buffer el tamaño de la entrada
            memset(bytes, '\0', sizeof(bytes));
            sprintf(bytes, "%d", inodoEntrada.tamEnBytesLog);
            strcat(buffer, bytes);
            strcat(buffer, "\t");
        }    
        // Guardamos en el buffer el nombre de la entrada
        sprintf(tmp, YELLOW"%s"RESET,entrada.nombre);
        strcat(buffer, tmp);
    }
    return num_entrada_inodo;
}

/**
Función: Cambia los permisos de un fichero o directorio
Parámetros:
    - *camino: Ruta del fichero o directorio para cambiar los permisos
    - permisos: Permisos del nuevo fichero o directorio
Salida: EXITO/FALLO
Llamado por:
    - mi_chmod.c
Llama a:
    - buscar_entrada()
    - mostrar_error_buscar_entrada()
    - mi_chmod_f()
**/
int mi_chmod(const char *camino, unsigned char permisos) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;

    int errorEntrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, permisos);
    if (errorEntrada < EXITO)  {
        mostrar_error_buscar_entrada(errorEntrada);
        return FALLO;
    }

    // Asignamos los permisos pasados por parámetro al inodo correspondiente
    if (mi_chmod_f(p_inodo, permisos) == FALLO) return FALLO;

    return EXITO;
}

/**
Función: Busca la entrada *camino con buscar_entrada() para obtener el p_inodo
Parámetros:
    - *camino: Camino de la entrada a mostrar
    - *p_stat: Estructura STAT para mostrar los detalles del p_inodo
Salida: El p_inodo
Llamado por:
    - verificacion.c
Llama a:
    - buscar_entrada()
    - mostrar_error_buscar_entrada()
    - mi_stat_f()
**/
int mi_stat(const char *camino, struct STAT *p_stat) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;

    // Buscamos la entrada correspondiente al camino pasado por parámetro
    int errorEntrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
    if (errorEntrada < EXITO)  {
        mostrar_error_buscar_entrada(errorEntrada);
        return FALLO;
    }

    // Imprimimos por pantalla el p_inodo
    fprintf(stderr, "Nº de inodo: %d", p_inodo);

    // Mostramos su información vía un p_stat
    if (mi_stat_f(p_inodo, p_stat) == FALLO) return FALLO;
    
    return p_inodo;
}

///////////////////////////////////////////////////////////////////
//                                                               //
//                            NIVEL 9                            //
//                                                               //
///////////////////////////////////////////////////////////////////

/**
Función: Ecribe un contenido en un fichero
Parámetros:
    - *camino: Ruta del fichero a escribir
    - *buf: Contenido a escribir
    - offset: Posición de escritura inicial
    - nbytes: Número de bytes a escribir
Salida: Número de bytes escritos
Llamado por:
    - mi_escribir.c
    - simulacion.c
Llama a:
    - buscar_entrada()
    - mostrar_error_buscar_entrada()
    - cacheFIFO()
    - cacheLRU()
    - mi_write_f()
**/
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int bytesEscritos=0;

    //SIN MEJORAS
    if (USARCACHE==0){
        int errorEntrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (errorEntrada < EXITO)  {
            mostrar_error_buscar_entrada(errorEntrada);
            return FALLO;
        }
    }
    // MEJORA L/E
    else if (USARCACHE==1) {
        if (strcmp(UltimaEntradaLectura.camino, camino) == 0) {
            p_inodo = UltimaEntradaEscritura.p_inodo;
            #if DEBUGN9==1
                fprintf(stderr,ORANGE"[mi_write() → Utilizamos la caché de escritura en vez de llamar a buscar_entrada()]\n"RESET);
            #endif
        } else {
            int errorEntrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
            if (errorEntrada < EXITO)  {
                mostrar_error_buscar_entrada(errorEntrada);
                return FALLO;
            }
            UltimaEntradaEscritura.p_inodo=p_inodo;
            strcpy(UltimaEntradaEscritura.camino, camino);
            #if DEBUGN9==1
                fprintf(stderr, ORANGE"[mi_write() → Actualizamos la caché de escritura]\n"RESET);
            #endif
        }
    // MEJORA FIFO
    } else if (USARCACHE==2) {
        p_inodo=cacheFIFO(camino, p_inodo_dir, p_inodo, p_entrada, "mi_write()");
        if (p_inodo==FALLO) return FALLO;
    }
    // MEJORA LRU
    else if (USARCACHE == 3) {
        p_inodo=cacheLRU(camino, p_inodo_dir, p_inodo, p_entrada, "mi_write()");
    }
    // Si la entrada existe
    bytesEscritos = mi_write_f(p_inodo, buf, offset, nbytes);
    if (bytesEscritos == FALLO) return FALLO;

    return bytesEscritos;
}

/**
Función: Lee los nbytes del fichero indicado
Parámetros:
    - *camino: Fichero a leer
    - *buf: Buffer donde se copia el texto leído
    - offset: Offset a partir del cuál leer
    - nbytes: Bytes que leer
Salida: Los bytes leídos (bytesLeidos)
Llamado por:
    - mi_cat.c
    - - verificacion.c
Llama a:
    - buscar_entrada()
    - cacheFIFO()
**/
int mi_read(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int bytesLeidos=0;

    // Caso Básico
    if (USARCACHE==0){
        int errorEntrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
        if (errorEntrada < EXITO)  {
            mostrar_error_buscar_entrada(errorEntrada);
            return FALLO;
        }
    }
    // Caso L/E
    else if (USARCACHE==1) {
        // Compromamos si el camino leído ya se ha leído antes
        if ((strcmp(UltimaEntradaLectura.camino, camino) == 0) && (USARCACHE==1)) {
            p_inodo = UltimaEntradaLectura.p_inodo;
            #if DEBUGN9==1
                fprintf(stderr, LBLUE"[mi_read() → Utilizamos la caché de lectura en vez de llamar a buscar_entrada()]\n"RESET);
            #endif
        } else {
            // Si no se había leído antes, buscamos la entrada
            int errorEntrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
            if (errorEntrada < EXITO)  {
                mostrar_error_buscar_entrada(errorEntrada);
                return FALLO;
            }
            //  Actualizamos la variable de la última entrada leída
            UltimaEntradaLectura.p_inodo=p_inodo;
            strcpy(UltimaEntradaLectura.camino, camino);
            #if DEBUGN9==1
                fprintf(stderr, ORANGE"[mi_read() → Actualizamos la caché de lectura]\n"RESET);
            #endif
        }
    } 
    // Caso FIFO
    else if (USARCACHE == 2) {
        // Llamamos a la función auxiliar
        p_inodo=cacheFIFO(camino, p_inodo_dir, p_inodo, p_entrada, "mi_read()");
        if (p_inodo==FALLO) return FALLO;
    } 
    // Caso LRU
    else if (USARCACHE == 3) {
        // Llamamos a la función auxiliar
        p_inodo=cacheLRU(camino, p_inodo_dir, p_inodo, p_entrada, "mi_read()");
        if (p_inodo==FALLO) return FALLO;
    }
    // Si la entrada existe
    bytesLeidos = mi_read_f(p_inodo, (void *)buf, offset, nbytes);
    if (bytesLeidos == FALLO) return FALLO;

    return bytesLeidos;
}

/**
Función: Trata el caso en que USARCACHE==2
Parámetros:
    - *camino: Camino de la entrada a tratar
    - p_inodo_dir: Número de inodo del directorio padre
    - p_inodo: Número de inodo de la entrada que buscamos
    - p_entrada: Número de entrada
    - *tipo: Indica si es para lectura o escritura
Salida: Número de inodo para leer o escribir el fichero
Llamado por:
    -  mi_read()
Llama a:
    - buscar_entrada()
    - mostrar_error_buscar_entrada()
**/
#if USARCACHE==2
    int cacheFIFO(const char *camino, unsigned int p_inodo_dir, unsigned int p_inodo, unsigned int p_entrada, char *tipo) {
        int encontrado=0;
        // Recorremos las últimas entradas registradas en nuestro "caché"
        for(int i=0; (i<CACHE_SIZE) && (encontrado==0); i++) {
            if (strcmp(UltimasEntradas[i].camino, camino) == 0) {
                // Si encontramos el camino a tratar ya registrado, lo devolvemos
                encontrado=1;
                p_inodo = UltimasEntradas[i].p_inodo;
                #if DEBUGN9==1
                   fprintf(stderr, LBLUE"[%s → Utilizamos cache[%d]: %s]\n"RESET, tipo, i, camino);
                #endif
            }
        }
        // Si, en cambio, no lo encontramos
        if (encontrado==0) {
            // Buscamos la entrada pertinente al camino
            int errorEntrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
            if (errorEntrada < EXITO)  {
                mostrar_error_buscar_entrada(errorEntrada);
                return FALLO;
            }
            // Actualizamos el "caché"            
            UltimasEntradas[posicion].p_inodo=p_inodo;
            strcpy(UltimasEntradas[posicion].camino, camino);
            #if DEBUGN9==1
                fprintf(stderr, ORANGE"[%s → Reemplazamos cache[%d]: %s]\n"RESET, tipo, posicion, camino);
            #endif
            // Actualizamos la posición en la tabla de "caché"
            posicion=((posicion+1) % CACHE_SIZE);
        }
        return p_inodo;
    }
#endif

/**
Función: Gestión de la caché utilizando LRU con sellos de tiempo
Parámetros:
    - *camino: Ruta del fichero
    - p_inodo_dir: Número de inodo del directorio padre
    - p_inodo: Número de inodo de la entrada que buscamos
    - p_entrada: Número de entrada
    - *tipo: Indica si es para lectura o escritura
Salida: Número de inodo para leer o escribir el fichero
Llamado por:
    - mi_write()
    - mi_read()
Llama a:
    - buscar_entrada()
    - mostrar_error_buscar_entrada()
**/
#if USARCACHE==3
    int cacheLRU(const char *camino, unsigned int p_inodo_dir, unsigned int p_inodo, unsigned int p_entrada, char *tipo) {
        int encontrado=0;
        for(int i=0; (i<CACHE_SIZE) && (encontrado==0); i++){
            if (strcmp(UltimasEntradas[i].camino, camino) == 0) {
                encontrado=1;
                p_inodo=UltimasEntradas[i].p_inodo;
                // Guardamos el timestamp de ultima consulta 
                gettimeofday(&UltimasEntradas[i].ultima_consulta, NULL);            
                fprintf(stderr, LBLUE"[%s → Utilizamos cache[%d]: %s]\n"RESET, tipo, i, camino);
            }
        }
        if (encontrado==0) {
            int errorEntrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
            if (errorEntrada < EXITO)  {
                mostrar_error_buscar_entrada(errorEntrada);
                return FALLO;
            }
            

            for (int i=0; i<CACHE_SIZE; i++) {
                if (UltimasEntradas[i].ultima_consulta.tv_sec < UltimasEntradas[posicion].ultima_consulta.tv_sec ||
                (UltimasEntradas[i].ultima_consulta.tv_sec == UltimasEntradas[posicion].ultima_consulta.tv_sec &&
                UltimasEntradas[i].ultima_consulta.tv_usec < UltimasEntradas[posicion].ultima_consulta.tv_usec)) {
                    // Encontramos una nueva entrada menos recientemente usada
                    posicion = i;
                }
            }
            UltimasEntradas[posicion].p_inodo=p_inodo;
            strcpy(UltimasEntradas[posicion].camino, camino);
            struct timeval tv;
            int tiempo = gettimeofday(&tv, NULL);
            if (tiempo == 0) {
                UltimasEntradas[posicion].ultima_consulta = tv;
            }
            #if DEBUGN9==1
                fprintf(stderr, ORANGE"[%s → Reemplazamos cache[%d]: %s]\n"RESET, tipo, posicion, camino);
            #endif
        }
        return p_inodo;
    }
#endif

///////////////////////////////////////////////////////////////////
//                                                               //
//                            NIVEL 10                           //
//                                                               //
///////////////////////////////////////////////////////////////////

/**
Función: Crea el enlace de una entrada de directorio camino2 al inodo especificado por otra entrada de directorio camino1.
Parámetros:
    - *camino1: Ruta de la entrada a enlazar
    - *camino2: Ruta del inodo a enlazar
Salida: EXITO/FALLO
Llamado por:
    - mi_link.c
Llama a:
    - mi_waitSem()
    - buscar_entrada()
    - mostrar_error_buscar_entrada()
    - mi_signalSem()
    - mi_read_f()
    - liberar_inodo()
    - leer_inodo()
    - escribir_inodo()
**/
int mi_link(const char *camino1, const char *camino2) {
    mi_waitSem();

    unsigned int p_inodo1=0;
    unsigned int p_inodo_dir1=0;
    unsigned int p_entrada1=0;
    int errorEntrada = buscar_entrada(camino1, &p_inodo_dir1, &p_inodo1, &p_entrada1, 0, 4);
    if (errorEntrada < EXITO)  {
        mostrar_error_buscar_entrada(errorEntrada);
        mi_signalSem();
        return FALLO;
    }

    unsigned int p_inodo2=0;
    unsigned int p_inodo_dir2=0;
    unsigned int p_entrada2=0;
    int errorEntrada2 = buscar_entrada(camino2, &p_inodo_dir2, &p_inodo2, &p_entrada2, 1, 6);
    if (errorEntrada2 < EXITO)  {
        mostrar_error_buscar_entrada(errorEntrada2);
        mi_signalSem();
        return FALLO;
    }
    struct entrada entrada;
    if (mi_read_f(p_inodo_dir2,&entrada,p_entrada2*sizeof(struct entrada),sizeof(struct entrada)) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    if (liberar_inodo(p_inodo2) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    entrada.ninodo=p_inodo1;
    
    if(mi_write_f(p_inodo_dir2,&entrada,p_entrada2*sizeof(struct entrada),sizeof(struct entrada)) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    struct inodo inodo;
    if(leer_inodo(p_inodo1,&inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    inodo.nlinks++;
    inodo.ctime=time(NULL);

    if(escribir_inodo(p_inodo1,&inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    mi_signalSem();
    return EXITO;
}

/**
Función: borra la entrada de directorio especificada y en caso de que fuera 
el último enlace existente, borrar el propio fichero/directorio.
Parámetros:
    - *camino: Ruta del fichero o directorio a deselazar
Salida: EXITO/FALLO
Llamado por:
    - mi_rm.c
    - mi_rmdir.c
Llama a:
    - mi_waitSem()
    - buscar_entrada()
    - mostrar_error_buscar_entrada()
    - mi_signalSem()
    - leer_inodo()
    - mi_truncar_f()
    - mi_read_f()
    - mi_write_f()
    - liberar_inodo()
    - escribir_inodo()
**/
int mi_unlink(const char *camino) {

    mi_waitSem();

    unsigned int p_inodo=0;
    unsigned int p_inodo_dir=0;
    unsigned int p_entrada=0;
    int errorEntrada = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 6);
    if (errorEntrada < EXITO)  {
        mostrar_error_buscar_entrada(errorEntrada);
        mi_signalSem();
        return FALLO;
    }

    // Comprobamos si el camino leído pertenece a un directorio no vacío, en cuyo caso no podrá ser borrado
    struct inodo inodo;
    if (leer_inodo(p_inodo,&inodo) == FALLO) return FALLO;
    if ((camino[strlen(camino)-1] == '/') && (inodo.tamEnBytesLog > 0)) {
        fprintf(stderr, RED"Error: El directorio %s no está vacío\n"RESET, camino);
        mi_signalSem();
        return FALLO;
    }
    
    // Leemos el inodo asociado al directorio que contiene la entrada
    struct inodo inodoDirectorio;
    if (leer_inodo(p_inodo_dir,&inodoDirectorio) == FALLO) {
        mi_signalSem();
        return FALLO;
    }
    int numEntradas=inodoDirectorio.tamEnBytesLog/sizeof(struct entrada);

    struct entrada entrada;
    // Comprobamos si la entrada leída es la última entrada del directorio
    if (p_entrada == (numEntradas-1)) {
        if (mi_truncar_f(p_inodo_dir, inodoDirectorio.tamEnBytesLog-sizeof(struct entrada)) == FALLO) {
            mi_signalSem();
            return FALLO;
        }
    } else {
        // Leemos la última entrada
        if (mi_read_f(p_inodo_dir, &entrada, (numEntradas-1)*sizeof(struct entrada), sizeof(struct entrada)) == FALLO) {
            mi_signalSem();
            return FALLO;
        }
        
        // Escribimos la última entrada en la posición de aquella que queremos eliminar (p_entrada)
        if (mi_write_f(p_inodo_dir, &entrada, p_entrada*sizeof(struct entrada), sizeof(struct entrada)) == FALLO) {
            mi_signalSem();
            return FALLO;
        }
        
        // Truncamos la p_entrada
        if (mi_truncar_f(p_inodo_dir, inodoDirectorio.tamEnBytesLog-sizeof(struct entrada)) == FALLO) {
            mi_signalSem();
            return FALLO;
        }
    }

    // Leemos el inodo de la entrada eliminada para actualizar nlinks
    if (leer_inodo(p_inodo,&inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }
    inodo.nlinks--;
    // Comprobamos si quedan enlaces (nlink)    
    if (inodo.nlinks == 0) {
        if (liberar_inodo(p_inodo) == FALLO) {
            mi_signalSem();
            return FALLO;
        }
    } else {
        inodo.ctime=time(NULL);
        if (escribir_inodo(p_inodo,&inodo) == FALLO) {
            mi_signalSem();
            return FALLO;
        }
    }

    mi_signalSem();
    return EXITO;
}
