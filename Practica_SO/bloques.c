// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López

#include "bloques.h"
#include "semaforo_mutex_posix.h"

static int descriptor = 0;
static sem_t *mutex;
static unsigned int inside_sc = 0;

/**
Función: Monta el dispositivo virtual 
Parámetros: 
    - camino: Fichero "dispositivo"
Salida: Devuelve el descriptor de la función open realizada o FALLO, si se da el caso
Llamado por: 
    - mi_mkfs.c
    - leer.c
    - leer_sf.c
    - escribir.c
    - permitir.c
    - truncar.c
    - mi_cat.c
    - mi_chmod.c
    - mi_escribir.c
    - mi_escribir_varios.c
    - mi_ls.c
    - mi_mkdir.c
    - mi_stat.c
    - mi_touch.c
    - prueba_cache_tabla.c
    - verificacion.c
    - simulacion.c
Llama a: N/A
**/
int bmount(const char *camino) {
    // Reseteamos la umask
    umask(000);
    // Abrimos el fichero del dispositivo virtual
    if (descriptor > 0) {
       close(descriptor);
    }
    descriptor = open(camino, O_RDWR | O_CREAT, 0666);
    // Siendo esto una llamada al sistema, manejamos el error adecuadamente
    if (descriptor == FALLO) {
        perror("Error");
        return FALLO;
    }

    //Inicializar el semaforo
    if (!mutex) { // el semáforo es único en el sistema y sólo se ha de inicializar 1 vez (padre)
        mutex = initSem(); 
        if (mutex == SEM_FAILED) {
            return FALLO;
        }
    }

    // Si se ha ejecutado correctamente el open, devolvemos el descriptor devuelto por la operación
    return descriptor;
}

/**
Función: Desmonta el dispositivo virtual
Parámetros: -
Salida: Devuelve EXITO si se ha cerrado el fichero correctamente, o FALLO en caso contrario
Llamado por:
    - mi_mkfs.c
    - leer.c
    - leer_sf.c
    - escribir.c
    - permitir.c
    - truncar.c
    - mi_cat.c
    - mi_chmod.c
    - mi_escribir.c
    - mi_escribir_varios.c
    - mi_ls.c
    - mi_mkdir.c
    - mi_stat.c
    - mi_touch.c
    - prueba_cache_tabla.c
    - verificacion.c
    - simulacion.c
Llama a: N/A
**/
int bumount() {
    // Restauramos el descriptor, cerramos el dispositivo virtual
    descriptor = close(descriptor);
    // Siendo esto una llamada al sistema, manejamos el error adecuadamente
    if (descriptor == FALLO) {
        perror("Error");
        return FALLO;
    }
    // Borrar el semáforo   
    deleteSem();     
    return EXITO;
}

/**
Función: Escribe un bloque de datos en el bloque físico especificado por 'nbloque' en un dispositivo 
         virtual
Parámetros: 
    - nbloque: ibloque del dispositivo al que señala el puntero
    - buf: puntero al buffer de memoria
Salida: Devuelve FALLO si se ha detectado un error durante el proceso, o return EXITO si se han podido
        realizar todas las operacione correctamente
Llamado por: 
    - mi_mkfs.c
    - initSB()
    - initMB()
    - initAI()
    - escribir_bit()
    - reservar_bloque()
    - liberar_bloque()
    - escribir_inodo()
    - reservar_inodo()
    - traducir_bloque_inodo()
    - mi_write_f()
    - liberar_inodo()
    - liberar_bloques_recursivo()
Llama a: N/A
**/
int bwrite(unsigned int nbloque, const void *buf) {
    // Declaramos el desplazamiento de cada bloque
    off_t desplazamiento = (nbloque * BLOCKSIZE);
    // Movemos el puntero del fichero en el offset correcto, ya calulado
    off_t puntero = lseek(descriptor, desplazamiento, SEEK_SET);
    if (puntero == FALLO) {
        perror("Error");
        return FALLO;
    }
    // Volcamos el contenido del buffer (de tamaño BLOCKSIZE) en la posición que marca el puntero 
    // del dispositivo virtual
    size_t bytesEscritos = write(descriptor, buf, BLOCKSIZE);
    if (bytesEscritos == FALLO) {
        perror("Error");
        return FALLO;
    }
    else if (bytesEscritos != BLOCKSIZE) {
        perror("Error");
        return FALLO;
    }
    return EXITO;
}

/**
Función: Lee un bloque del dispositivo virtual
Parámetros: 
    nbloque: bloque físico
    buf: puntero al buffer de memoria
Salida: bytes que ha podido leer. EXITO si es BLOCKSIZE, o FALLO si se produce un error
Llamado por: 
    - leer_sf.c
    - initMB()
    - initAI()
    - escribir_bit()
    - leer_bit()
    - reservar_bloque()
    - liberar_bloque()
    - escribir_inodo()
    - leer_inodo()
    - reservar_inodo()
    - traducir_bloque_inodo()
    - mi_write_f()
    - mi_read_f()
    - liberar_inodo()
    - liberar_bloques_recursivo()
    - buscar_entrada()
Llama a: N/A
**/
int bread(unsigned int nbloque, void *buf) {
    // Declaramos el desplazamiento de cada bloque
    off_t desplazamiento = (nbloque * BLOCKSIZE);
    // Movemos el puntero del fichero en el offset correcto, ya calulado
    off_t puntero = lseek(descriptor, desplazamiento, SEEK_SET);
    if (puntero == FALLO)
    {
        perror("Error");
        return FALLO;
    }
    // Volcamos el contenido del buffer (de tamaño BLOCKSIZE) en la posición que marca el puntero del
    // dispositivo virtual
    size_t bytesLeidos = read(descriptor, buf, BLOCKSIZE);
    if (bytesLeidos == FALLO)
    {
        perror("Error");
        return FALLO;
    }
    else if (bytesLeidos != BLOCKSIZE)
    {
        perror("Error");
        return FALLO;
    }
    return bytesLeidos;
}

void mi_waitSem() {
    if (!inside_sc) { // inside_sc==0, no se ha hecho ya un wait
        waitSem(mutex);
    }
    inside_sc++;
}

void mi_signalSem() {
    inside_sc--;
    if (!inside_sc) {
        signalSem(mutex);
    }
}
