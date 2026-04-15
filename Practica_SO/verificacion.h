// Antoni Navarro Moreno, Luz Salvá Castro y Laura Rodríguez López
//verificacion.h
#include "simulacion.h"

struct INFORMACION {
  int pid;
  unsigned int nEscrituras; //validadas 
  struct REGISTRO PrimeraEscritura;
  struct REGISTRO UltimaEscritura;
  struct REGISTRO MenorPosicion;
  struct REGISTRO MayorPosicion;
};