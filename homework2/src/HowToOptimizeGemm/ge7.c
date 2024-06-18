//requires additional changes to the code to make it work

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <immintrin.h>
/* Include PAPI */
#include <papi.h> 

/* PAPI macro helpers definitions */
#define NUM_EVENT 2
#define THRESHOLD 100000
#define ERROR_RETURN(retval) { fprintf(stderr, "Error %d %s:line %d: \n", retval,__FILE__,__LINE__);  exit(retval); }

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define BLKSIZE 16
#define IDX(i, j, n) (((j)+ (i)*(n)))

static double gtod_ref_time_sec = 0.0;

/* Adapted from the bl2_clock() routine in the BLIS library */

double dclock() {
    double the_time, norm_sec;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (gtod_ref_time_sec == 0.0)
        gtod_ref_time_sec = (double) tv.tv_sec;
    norm_sec = (double) tv.tv_sec - gtod_ref_time_sec;
    the_time = norm_sec + tv.tv_usec * 1.0e-6;
    return the_time;
}

int ge(double *A, int SIZE) {
    register unsigned i, j, k;
    register double multiplier;
    register __m256d mm_multiplier;
    register __m256d tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    for (k = 0; k < SIZE; k++) {
        for (i = k + 1; i < SIZE; i++) {
            multiplier = A[IDX(i, k, SIZE)] / A[IDX(k, k, SIZE)];
            mm_multiplier[0] = multiplier;
            mm_multiplier[1] = multiplier;
            mm_multiplier[2] = multiplier;
            mm_multiplier[3] = multiplier;
            for (j = k + 1; j < SIZE;) {
                if (j < max(SIZE - BLKSIZE, 0)) {
                    mm_multiplier = _mm256_set1_pd(multiplier);
                    tmp0 = _mm256_loadu_pd(A + IDX(i, j, SIZE));
                    tmp1 = _mm256_loadu_pd(A + IDX(k, j, SIZE));
                    tmp2 = _mm256_loadu_pd(A + IDX(i, j + 4, SIZE));
                    tmp3 = _mm256_loadu_pd(A + IDX(k, j + 4, SIZE));
                    tmp4 = _mm256_loadu_pd(A + IDX(i, j + 8, SIZE));
                    tmp5 = _mm256_loadu_pd(A + IDX(k, j + 8, SIZE));
                    tmp6 = _mm256_loadu_pd(A + IDX(i, j + 12, SIZE));
                    tmp7 = _mm256_loadu_pd(A + IDX(k, j + 12, SIZE));

                    tmp1 = _mm256_mul_pd(tmp1, mm_multiplier);
                    tmp3 = _mm256_mul_pd(tmp3, mm_multiplier);
                    tmp5 = _mm256_mul_pd(tmp5, mm_multiplier);
                    tmp7 = _mm256_mul_pd(tmp7, mm_multiplier);

                    tmp0 = _mm256_sub_pd(tmp0, tmp1);
                    tmp2 = _mm256_sub_pd(tmp2, tmp3);
                    tmp4 = _mm256_sub_pd(tmp4, tmp5);
                    tmp6 = _mm256_sub_pd(tmp6, tmp7);

                    _mm256_storeu_pd(A + IDX(i, j, SIZE), tmp0);
                    _mm256_storeu_pd(A + IDX(i, j + 4, SIZE), tmp2);
                    _mm256_storeu_pd(A + IDX(i, j + 8, SIZE), tmp4);
                    _mm256_storeu_pd(A + IDX(i, j + 12, SIZE), tmp6);
                    j += BLKSIZE;
                } else {
                    A[IDX(i, j, SIZE)] = A[IDX(i, j, SIZE)] - A[IDX(k, j, SIZE)] * multiplier;
                    j++;
                }
            }
        }
    }
    return 0;
}

int main(int argc, const char *argv[])
{
  int i, j, k, iret;
  double dtime;
  int SIZE = atoi(argv[1]);
  int measure = 0;

  /* PAPI FLOPS variables */
  float real_time, proc_time,mflops;
  long long flpops;
  float ireal_time, iproc_time, imflops;
  long long iflpops;
  int retval;
  
  /* PAPI counters variables */
  int tmp;
  int EventSet = PAPI_NULL;
  /*must be initialized to PAPI_NULL before calling PAPI_create_event*/

  int event_codes[NUM_EVENT]={PAPI_TOT_INS,PAPI_TOT_CYC}; 
  char errstring[PAPI_MAX_STR_LEN];
  long long values[NUM_EVENT];

  // allocate blocks of continous memory
  double *matrix_ = (double *)malloc(SIZE * SIZE * sizeof(double));
  // allocate 2D matrices
  double **matrix = (double **)malloc(SIZE * sizeof(double));
  // set pointers to continous blocks
  for (i = 0; i < SIZE; i++)
  {
    matrix[i] = matrix_ + i * SIZE;
  }
  srand(1);
  for (i = 0; i < SIZE; i++)
  {
    for (j = 0; j < SIZE; j++)
    {
      matrix[i][j] = rand();
    }
  }

  if (argc>2){
        measure = atoi(argv[2]);
    }
    
    /* measurments */
    if (measure == 0){
        dtime = dclock();
    }
    if (measure == 1){
        if((retval=PAPI_flops_rate(PAPI_FP_OPS,&ireal_time,&iproc_time,&iflpops,&imflops)) < PAPI_OK){ 
            printf("Could not initialise PAPI_flops \n");
            printf("Your platform may not support floating point operation event.\n"); 
            printf("retval: %d\n", retval);
            exit(1);
        }
    }
    if (measure == 2){
        /* initializing library */
        if((retval = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT ) {
            fprintf(stderr, "Error: %s\n", errstring);
            exit(1);            
        }
        /* Creating event set   */
        if ((retval=PAPI_create_eventset(&EventSet)) != PAPI_OK){
            ERROR_RETURN(retval);
        }
        /* Add the array of events PAPI_TOT_INS and PAPI_TOT_CYC to the eventset*/
        if ((retval=PAPI_add_events(EventSet, event_codes, NUM_EVENT)) != PAPI_OK){
            ERROR_RETURN(retval);
        }
        /* Start counting */
        if ( (retval=PAPI_start(EventSet)) != PAPI_OK){
            ERROR_RETURN(retval);
        }
    }
    
    iret = ge(matrix, SIZE);

    if (measure == 0){
        dtime = dclock()-dtime;
        printf( "Time: %le \n", dtime);
    }
    if (measure == 1){
        if((retval=PAPI_flops_rate(PAPI_FP_OPS,&real_time, &proc_time, &flpops, &mflops))<PAPI_OK) {
            printf("retval: %d\n", retval);
            exit(1);
        }
        printf("Real_time: %f Proc_time: %f flpops: %lld MFLOPS: %f\n", real_time, proc_time,flpops,mflops);
    }
    if (measure == 2){
        /* Stop counting, this reads from the counter as well as stop it. */
        if ( (retval=PAPI_stop(EventSet,values)) != PAPI_OK){
            ERROR_RETURN(retval);
        }
        printf("\nThe total instructions executed are %lld, total cycles %lld\n", values[0],values[1]);
    }
  
  fflush(stdout);
  free(matrix_);
  free(matrix);

  return iret;
}