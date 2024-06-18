#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <x86intrin.h>

#define IDX(i, j, n) (((j)+ (i)*(n)))
#define BLKSIZE 8


static double gtod_ref_time_sec = 0.0;

/* Adapted from the bl2_clock() routine in the BLIS library */

double dclock(){
    double the_time, norm_sec;
    struct timeval tv;
    gettimeofday( &tv, NULL );
    if ( gtod_ref_time_sec == 0.0 )
        gtod_ref_time_sec = ( double ) tv.tv_sec;
    norm_sec = ( double ) tv.tv_sec - gtod_ref_time_sec;
    the_time = norm_sec + tv.tv_usec * 1.0e-6;
    return the_time;
}



int chol(double * A, unsigned int n){
    register unsigned int i, j, k;
    register double tmp;
    register __m128d tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;

    for (j = 0; j < n; j++) {
        for (i = j; i < n; i++) {
            tmp = A[IDX(i, j, n)];
            for (k = 0; k < j; k++) {
                if (k < fmax(j - BLKSIZE, 0)) {
                    tmp0 = _mm_loadu_pd(A+IDX(i, k, n));
                    tmp1 = _mm_loadu_pd(A+IDX(i, k, n));
                    tmp2 = _mm_loadu_pd(A+IDX(i, k+2, n));
                    tmp3 = _mm_loadu_pd(A+IDX(i, k+2, n));
                    tmp4 = _mm_loadu_pd(A+IDX(i, k+4, n));
                    tmp5 = _mm_loadu_pd(A+IDX(i, k+4, n));
                    tmp6 = _mm_loadu_pd(A+IDX(i, k+6, n));
                    tmp7 = _mm_loadu_pd(A+IDX(i, k+6, n));

                    tmp0 = _mm_mul_pd(tmp0, tmp1);
                    tmp2 = _mm_mul_pd(tmp2, tmp3);
                    tmp4 = _mm_mul_pd(tmp4, tmp5);
                    tmp6 = _mm_mul_pd(tmp6, tmp7);

                    tmp0 = _mm_add_pd(tmp0, tmp2);
                    tmp0 = _mm_add_pd(tmp0, tmp4);
                    tmp0 = _mm_add_pd(tmp0, tmp6);
                    
                    tmp -= tmp0[0] + tmp0[1];

                    k += BLKSIZE;
                } else {
                    tmp -= A[IDX(i, k, n)] * A[IDX(j, k, n)];
                    k++;
                }
            }
        }

        if (A[IDX(j, j, n)] < 0.0) {
            return (1);
        }

        tmp = sqrt(A[IDX(j, j, n)]);
        for (i = j + 1; i < n; i++){
            A[IDX(i, j, n)] /= tmp;
        }
    }

    return (0);
}

int main(int argc, char ** argv){
    double* A;
    double t1, t2;
    int i, j, n, ret;
    double dtime;

    n = atoi(argv[1]);
    A = malloc(n * n * sizeof(double));
    assert(A != NULL);

    for (i = 0; i < n; i++) {
        A[IDX(i, i, n)] = 1.0;
    }

    dtime = dclock();
    if (chol(A, n)) {
      fprintf(stderr,"Matrix is not symmetric or not positive definite\n");
    }
    dtime = dclock()-dtime;
    printf( "Time: %le \n", dtime);

    double sum = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            sum += A[IDX(i, j, n)];
        }
    }
    printf( "Sum of array: %lf\n", sum);

    fflush( stdout );

    free(A);
    return 0;
}