// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López

#include "ficheros_basico.h"

struct STAT {
    unsigned char tipo;
    unsigned char permisos; 
    unsigned int nlinks;            
    unsigned int tamEnBytesLog; 
    // Fecha y hora del último acceso a datos
    time_t atime; 
    // Fecha y hora de la última modificación de datos
    time_t mtime; 
    // Fecha y hora de la última modificación del inodo
    time_t ctime; 
    unsigned int numBloquesOcupados;
};

//----------  NIVEL 5  --------------------------------------------------------------------------------------//
int mi_write_f(unsigned int ninodo, const void *buf_original, unsigned int offset, unsigned int nbytes);
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset, unsigned int nbytes);
int mi_stat_f(unsigned int ninodo, struct STAT *p_stat);
int mi_chmod_f(unsigned int ninodo, unsigned char permisos);

//----------  NIVEL 6  --------------------------------------------------------------------------------------//
int mi_truncar_f(unsigned int ninodo, unsigned int nbytes);
