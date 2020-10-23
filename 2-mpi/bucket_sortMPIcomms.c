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


// dtime
//
// returns the current wall clock time
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

            int *nums;
            int *indices;
            int **buckets;

            //gerar vetor aleatorio
            nums = (int*)malloc(sizeof(int)*TAM);
            generate_vector(nums);
            //printf("--------------------------\n");
            //print_array(nums);
            //printf("--------------------------\n");


            buckets=malloc(sizeof(int*)*nprocesses);

            //inicializar buckets
            for(int i=0; i<nprocesses; i++)
                buckets[i] = calloc(TBUCKET, sizeof(int));

            indices = calloc(nprocesses, sizeof(int));
            int ibucket;

            //percorrer array
            //colocar elementos nos bucketss
            //int val;

            int val;
            for(int i=0; i<TAM; i++) {

                val=nums[i];

                ibucket=val*(nprocesses)/(MAX+1);

                int sizeb = indices[ibucket];

                if (sizeb==TBUCKET || (sizeb!=0 && sizeb%TBUCKET==0)) {
                    //printf(">> Expanding bucket\n");
                    buckets[ibucket] = realloc(buckets[ibucket], sizeof(int)*2*sizeb);
                }
                //printf("value %3d   ||  ibucket %3d  ||  bucket size %3d\n", val, ibucket, sizeb);

                buckets[ibucket][sizeb]=val;
                //MPI_Send( msg, 1, MPI_INT, ibucket, 1, MPI_COMM_WORLD);

                indices[ibucket]++;
            }


            printf("comms teste\n");
            double timei = MPI_Wtime();

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
                //qsort(buckets[0], indices[0], sizeof(int), compare);
                memcpy(nums, buckets[0], indices[0]*sizeof(int));

                put+=indices[0];


                //MASTER recebe buckets ordenados dos slaves e coloca no array

                for(int i=1; i<nprocesses; i++) {

                    MPI_Recv( tam2, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status );

                    int *buf = (int*) malloc(sizeof(int)*tam2[0]);
                    MPI_Recv( nums+put, tam2[0], MPI_INT, i, 0, MPI_COMM_WORLD, &status );

    //              printf("%d ##########################\n", tam2[0]);
    //              print_array_tam(buf,tam2[0]);
    //              printf("##########################\n");

                    put+=tam2[0];

                    free((void*) buf);

                }


                printf("\nTime elapsed: %.3f secs\n", (MPI_Wtime()-timei));

                for(int i = 0; i < nprocesses; i++) {
                    free((void *)buckets[i]);
                }
            	  free((void *)buckets);
                free((void *)nums);
                free((void *)indices);

                //MPI_Gather(bucks, tam[0], MPI_INT, nums, indx, displ, MPI_INT, 0, MPI_COMM_WORLD);
                //printf("!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                //print_array(nums);
                //printf("!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        }



        //slaves (buckets sorts)
        if(myrank>0) {


            //recebe tamanho e bucket
            int tam[1];
            MPI_Recv( tam, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status );

            int *bucks = (int*) malloc(sizeof(int)*tam[0]);
            MPI_Recv( bucks, tam[0], MPI_INT, 0, 0, MPI_COMM_WORLD, &status );

            // sort buckets
            //qsort(bucks, tam[0], sizeof(int), compare);


            //enviar buckets ordenadados para MASTER

            MPI_Send( tam, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send( bucks, tam[0], MPI_INT, 0, 0, MPI_COMM_WORLD);
        }



    MPI_Finalize ();
}

    return 0;
}




// MPI_Scatter(numsorted, TAM, MPI_INT, bucks, size, MPI_INT, 0, MPI_COMM_WORLD);

// MPI_Gather(bucks, size, MPI_INT, numsorted, TAM, MPI_INT, 0, MPI_COMM_WORLD);

        /*

        nmin = NPTS/NPROC;
        nextra = NPTS%NPROC;
        k = 0;
        for (i=0; i<NPROC; i++) {
        if (i<nextra) sendcounts[i] = nmin+1;
        else sendcounts[i] = nmin;

        displs[i] = k;
        k+=sendcounts[i];
        }
        // need to set recvcount also ...
        MPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);*/
