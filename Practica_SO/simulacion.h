// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López
#include <sys/wait.h>
#include <signal.h>
#include "directorios.h"

struct REGISTRO { //sizeof(struct REGISTRO): 24 bytes
   time_t fecha; //Precisión segundos [opcionalmente microsegundos con struct timeval]
   pid_t pid; //PID del proceso que lo ha creado
   int nEscritura; //Entero con el nº de escritura, de 1 a 50 (orden por tiempo)
   int nRegistro; //Entero con el nº del registro dentro del fichero: [0..REGMAX-1] (orden por posición)
};

#define NUMPROCESOS 100
#define NUMESCRITURAS 50
#define REGMAX 500000 // (((12+256+256²+256³)-1)*BLOCKSIZE) /sizeof(struct registro)

//----------  NIVEL 12  -------------------------------------------------------------------------------------//
void reaper();