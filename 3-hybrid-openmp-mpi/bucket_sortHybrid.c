#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <omp.h>
#include <unistd.h>

#define MIN 1
#define MAX 1000000

#define TAM 30000000
#define TBUCKET 400000

#define TBUCKET2 20000
#define NBUCKETS2 16


// dtime
//
// returns the current wall clock tim
//
double dtime() {
    double tseconds = 0.0;
    struct timeval mytime;
    gettimeofday(&mytime,(struct timezone*)0);
    tseconds = (double)(mytime.tv_sec + mytime.tv_usec*1.0e-6);
    return( tseconds );
}


void generate_vector(int *v) {
    srand(time(NULL));
    for(int i=0; i<TAM; i++) {
        v[i] = MIN + rand() % (MAX);
    }
}

void print_array(int *v) {
    for(int i=0; i<TAM; i++) {
        printf("%d ",v[i]);
    }
    printf("\n");
}

void print_array_tam(int *v, int size) {
    for(int i=0; i<size; i++) {
        printf("%d ",v[i]);
    }
    printf("\n");
}

int compare (const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}



int main(int argc, char **argv) {

    int nprocesses;
    int myrank;
    MPI_Status status;

    MPI_Init (&argc, &argv);
    MPI_Comm_size (MPI_COMM_WORLD, &nprocesses);
    MPI_Comm_rank (MPI_COMM_WORLD, &myrank);



    //nprocesses == NBUCKETS
    if (nprocesses > 0)  {

        //master
        if (myrank==0) {

            double timei = MPI_Wtime();

            int *nums;
            int *indices;
            int **buckets;

            //gerar vetor aleatorio
            nums = (int*)malloc(sizeof(int)*TAM);
            generate_vector(nums);

            printf("---------- init ---------\n");
            //print_array(nums);
            printf("-------------------------\n");

            buckets=malloc(sizeof(int*)*nprocesses);

            //inicializar buckets
            for(int i=0; i<nprocesses; i++)
                buckets[i] = calloc(TBUCKET, sizeof(int));

            indices = calloc(nprocesses, sizeof(int));
            int ibucket;

            //percorrer array
            //colocar elementos nos bucketss

            int val;
            for(int i=0; i<TAM; i++) {

                val=nums[i];

                ibucket=val*(nprocesses)/(MAX+1);

                int sizeb = indices[ibucket];

                if (sizeb==TBUCKET || (sizeb!=0 && sizeb%TBUCKET==0)) {

                    buckets[ibucket] = realloc(buckets[ibucket], sizeof(int)*2*sizeb);
                }
                buckets[ibucket][sizeb]=val;

                indices[ibucket]++;
            }


            int msg[1];

            for(int i=1; i<nprocesses; i++) {
                msg[0]=indices[i];

                //enviar tamanho e respetio bucket aos slaves(bucket sorts)
                MPI_Send( msg, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
                MPI_Send( buckets[i], indices[i], MPI_INT, i, 0, MPI_COMM_WORLD);
            }

                int tam2[1];
                int size2;
                int put=0;

                // sort buckets
                // MASTER

                printf("\n###### starting openmp code\n\n");

                /* ****************   add OpenMP here    *********************
                 *************************************************************
                 *************************************************************
                 *************************************************************
                 *************************************************************
                 *************************************************************
                 *************************************************************
                 *************************************************************
                */

                int **buckets2;

                buckets2 = malloc(sizeof(int*)*NBUCKETS2);

                #pragma omp simd
                for(int i=0; i<NBUCKETS2; i++)
                    buckets2[i] = calloc(TBUCKET2, sizeof(int));

                int *indices2 = calloc(NBUCKETS2, sizeof(int));
                int ibucket2;

                //percorrer array
                //colocar elementos nos bucketss

                for(int i=0; i<indices[0]; i++) {

                    int val=buckets[0][i];

                    ibucket=val*(NBUCKETS2-1)/MAX;

                    int sizeb = indices2[ibucket];

                    if (sizeb==TBUCKET2 || (sizeb!=0 && sizeb%TBUCKET2==0)) {
                        //printf(">> Expanding bucket\n");
                        buckets2[ibucket] = realloc(buckets2[ibucket], sizeof(int)*2*sizeb);
                    }
                    //printf("value %3d   ||  ibucket %3d  ||  bucket size %3d\n", val, ibucket, sizeb);
                    buckets2[ibucket][sizeb]=val;
                    indices2[ibucket]++;

                }

                //percorrer buckets
                //sort buckets
                #pragma omp parallel for schedule(dynamic, 1)
                for(int i=0; i<NBUCKETS2; i++) {

                    int sizeb2=indices2[i];

                    qsort(buckets2[i], sizeb2, sizeof(int), compare);

                    int init=0;
                    for(int k=0; k<i; k++) {
                        init+=indices2[k];
                    }

                    for(int j=0; j<sizeb2; j++) {
                        buckets[0][init+j]=buckets2[i][j];

                    }

                }


                //libertar memoria
                for(int i = 0; i < NBUCKETS2; i++) {
            		    free((void *)buckets2[i]);
                }
            	  free((void *)buckets2);
                free((void *)indices2);


                /* ****************    OpenMP ended    ***********************
                 *************************************************************
                 *************************************************************
                 *************************************************************
                */


                memcpy(nums, buckets[0], indices[0]*sizeof(int));

                put+=indices[0];


                //MASTER recebe buckets ordenados dos slaves e coloca no array

                for(int i=1; i<nprocesses; i++) {

                    MPI_Recv( tam2, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status );

                    int *buf = (int*) malloc(sizeof(int)*tam2[0]);
                    MPI_Recv( nums+put, tam2[0], MPI_INT, i, 0, MPI_COMM_WORLD, &status );

                    put+=tam2[0];

                    free((void*) buf);
                }


                printf("\nTime elapsed: %.3f secs\n", (MPI_Wtime()-timei));


                printf("**************** parallel computed ended ****************\n");
                //print_array(nums);
                printf("*********************************************************\n");
                printf("*********************************************************\n");

                for(int i = 0; i < nprocesses; i++) {
                    free((void *)buckets[i]);
                }
            	  free((void *)buckets);
                free((void *)nums);
                free((void *)indices);

        }



        //slaves (buckets sorts)
        if(myrank>0) {


            //recebe tamanho e bucket
            int tam[1];
            MPI_Recv( tam, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status );


            int *array = (int*) malloc(sizeof(int)*tam[0]);
            MPI_Recv( array, tam[0], MPI_INT, 0, 0, MPI_COMM_WORLD, &status );



            //divide array into
            // sort buckets

            //SLAVE

            /* ****************   add OpenMP here    *********************
             *************************************************************
             *************************************************************
             *************************************************************
             *************************************************************
             *************************************************************
             *************************************************************
             *************************************************************
            */



            int **buckets2;

            buckets2 = malloc(sizeof(int*)*NBUCKETS2);


            #pragma omp simd
            for(int i=0; i<NBUCKETS2; i++)
                buckets2[i] = calloc(TBUCKET2, sizeof(int));

            int *indices2 = calloc(NBUCKETS2, sizeof(int));
            int ibucket;

            //percorrer array
            //colocar elementos nos bucketss

            for(int i=0; i<tam[0]; i++) {

                int val=array[i];

                ibucket=val*(NBUCKETS2-1)/MAX;

                int sizeb = indices2[ibucket];

                if (sizeb==TBUCKET2 || (sizeb!=0 && sizeb%TBUCKET2==0)) {
                    //printf(">> Expanding bucket\n");
                    buckets2[ibucket] = realloc(buckets2[ibucket], sizeof(int)*2*sizeb);
                }
                //printf("value %3d   ||  ibucket %3d  ||  bucket size %3d\n", val, ibucket, sizeb);
                buckets2[ibucket][sizeb]=val;
                indices2[ibucket]++;

            }

            //percorrer buckets
            //sort buckets
            #pragma omp parallel for schedule(dynamic, 1)
            for(int i=0; i<NBUCKETS2; i++) {

                printf("%d\n",omp_get_num_threads)

                int sizeb2=indices2[i];

                qsort(buckets2[i], sizeb2, sizeof(int), compare);

                int init=0;
                for(int k=0; k<i; k++) {
                    init+=indices2[k];
                }

                for(int j=0; j<sizeb2; j++) {
                    array[init+j]=buckets2[i][j];

                }

            }

            //libertar memoria
            for(int i = 0; i < NBUCKETS2; i++) {
                free((void *)buckets2[i]);
            }
            free((void *)buckets2);
            free((void *)indices2);

            /* ****************    OpenMP ended    ***********************
             *************************************************************
             *************************************************************
             *************************************************************
            */


            //enviar buckets ordenadados para MASTER


            MPI_Send( tam, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send( array, tam[0], MPI_INT, 0, 0, MPI_COMM_WORLD);
        }



    MPI_Finalize ();
}

    return 0;
}
