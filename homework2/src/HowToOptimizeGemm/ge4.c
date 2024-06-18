//requires additional changes to the code to make it work

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
/* Include PAPI */
#include <papi.h> 

/* PAPI macro helpers definitions */
#define NUM_EVENT 2
#define THRESHOLD 100000
#define ERROR_RETURN(retval) { fprintf(stderr, "Error %d %s:line %d: \n", retval,__FILE__,__LINE__);  exit(retval); }

#define BLKSIZE 8
#define MAX(a,b) (((a)>(b))?(a):(b))

static double gtod_ref_time_sec = 0.0;

/* Adapted from the bl2_clock() routine in the BLIS library */

double dclock()
{
  double the_time, norm_sec;
  struct timeval tv;
  gettimeofday( &tv, NULL );
  if ( gtod_ref_time_sec == 0.0 )
    gtod_ref_time_sec = ( double ) tv.tv_sec;
  norm_sec = ( double ) tv.tv_sec - gtod_ref_time_sec;
  the_time = norm_sec + tv.tv_usec * 1.0e-6;
  return the_time;
}

int ge(double ** A, int SIZE)
{
  register int i,j,k;
  register double multiplier;
  for (k = 0; k < SIZE; k++) { 
    for (i = k+1; i < SIZE; i++) { 
      multiplier = A[i][k]/A[k][k];
      for (j = k+1; j < SIZE;) { 
        if (j < MAX(SIZE - BLKSIZE, 0)) {
          A[i][j] = A[i][j]-A[k][j]*multiplier;
          A[i][j+1] = A[i][j+1]-A[k][j+1]*multiplier;
          A[i][j+2] = A[i][j+2]-A[k][j+2]*multiplier;
          A[i][j+3] = A[i][j+3]-A[k][j+3]*multiplier;
          A[i][j+4] = A[i][j+4]-A[k][j+4]*multiplier;
          A[i][j+5] = A[i][j+5]-A[k][j+5]*multiplier;
          A[i][j+6] = A[i][j+6]-A[k][j+6]*multiplier;
          A[i][j+7] = A[i][j+7]-A[k][j+7]*multiplier;
          j += BLKSIZE;
        } else {
          A[i][j] = A[i][j]-A[k][j]*multiplier;
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