#include <stdlib.h>

#define A( i,j ) a[ (j)*lda + (i) ]

#if !(_SVID_SOURCE || _XOPEN_SOURCE)
double drand48(void) {
    return rand() / (RAND_MAX + 1.0);
}
#endif

void random_matrix( int m, int n, double *a, int lda )
{
  double drand48();
  int i,j;

  for ( j=0; j<n; j++ )
    for ( i=0; i<m; i++ )
      A( i,j ) = 2.0 * drand48( ) - 1.0;
}
