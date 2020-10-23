#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <omp.h>
#include <time.h>
#include <sys/time.h>


#define MIN 1
#define MAX 1000000

#define TAM 2090000
//#define NBUCKETS 2000
#define TBUCKET 100000


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
        printf("%d\n",v[i]);
    }
}

int compare (const void * a, const void * b) {
    return ( *(int*)a - *(int*)b );
}



void bucket_sort(int *vector, int NBUCKETS) {

    double tstart, tstop, ttime;

    tstart = dtime();

    int val;
    int **buckets;

    buckets = malloc(sizeof(int*)*NBUCKETS);

    for(int i=0; i<NBUCKETS; i++)
        buckets[i] = calloc(TBUCKET, sizeof(int));

    int *indices = calloc(NBUCKETS, sizeof(int));
    int ibucket;
    int sizeb;

    //percorrer array
    //colocar elementos nos bucketss
    for(int i=0; i<TAM; i++) {

        val=vector[i];
        ibucket=val*(NBUCKETS-1)/MAX;
        sizeb = indices[ibucket];

        if (sizeb==TBUCKET || (sizeb!=0 && sizeb%TBUCKET==0)) {
            buckets[ibucket] = realloc(buckets[ibucket], sizeof(int)*2*sizeb);
        }
        //printf("value %3d   ||  ibucket %3d  ||  bucket size %3d\n", val, ibucket, sizeb);
        buckets[ibucket][sizeb]=val;
        indices[ibucket]++;
    }



    int x=0;
    int indx;
    //percorrer buckets
    //sort buckets
    for(int i=0; i<NBUCKETS; i++) {

        sizeb=indices[i];

        qsort(buckets[i], sizeb, sizeof(int), compare);

        for(int j=0; j<sizeb; j++) {
            vector[x++]=buckets[i][j];
        }
    }


    tstop = dtime();
    ttime = tstop - tstart;

    if ((ttime) > 0.0) {
        //printf("Using %d threads\n", nts);
        printf("Secs = %3.4lf\r\n", ttime);
    }


    //libertar memoria
    for(int i=0; i<NBUCKETS; i++) {
		free((void *)buckets[i]);
    }
	free((void *)buckets);
    free((void *)indices);

}


int main(int argc, char* argv[]){

    int nums[TAM];
    int nb=2000;
    if (argc==2) nb = atoi(argv[1]);
    //printf("1 ####### Generating array\n");
    generate_vector(nums);

    printf("Using %d buckets\n", nb);
    //printf("2 ####### Starting bucket sorting\n");
    bucket_sort(nums, nb);

    //printf("3 ####### Printing sorted array\n");
    if (argc>2)
        print_array(nums);

    return 0;
}
