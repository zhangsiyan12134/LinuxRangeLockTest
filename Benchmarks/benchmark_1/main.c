#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#ifdef ctfs
#include "../ctfs.h"
#define OPEN        ctfs_open
#define PWRITE      ctfs_pwrite
#define PWRITEA		ctfs_pwrite_atomic
#define PREAD       ctfs_pread
#define CLOSE		close
#define INIT        ctfs_init(0)
#else
#include <unistd.h>
#define OPEN 		open
#define PWRITE 		pwrite64
#define PWRITEA		pwrite64
#define PREAD 		pread64
#define CLOSE		close
#define FOPEN		fopen
#define FCLOSE		fclose
#define FWRITE		fwrite
#define FREAD		fread
#define FFLUSH		fflush
#define INIT
#define print_debug(a)
#endif

#define _GB (1*1024*1024*(unsigned long)1024)

typedef struct{
    int tid;
    int tnum;
    int fd;
    int round;
    long long size;
    int blk_size;
    char* buffer;
    char* rnd_buf;
    long long *rnd_addrs;
    uint64_t total_time;
	uint64_t total_bytes;
}config;

long calc_diff(struct timespec start, struct timespec end){
	return (end.tv_sec * (long)(1000000000) + end.tv_nsec) -
	(start.tv_sec * (long)(1000000000) + start.tv_nsec);
}

enum test_mode{
	SEQ_READ,      //multi-threads sequential read
	SEQ_WRITE,
	RND_READ,      //multi-threads random read
	RND_WRITE
};

void *seq_write_test(void *vargp){
    config *conf = (config*) vargp;
    uint64_t ret = 0;
    long long portion = conf->size / conf->tnum;
    long long start_addr = conf->tid * portion;

    printf("Thread %d created, write range is: %lld - %lld\n", conf->tid, start_addr, start_addr + portion);
    for(uint64_t i = 0; i < conf->round; i++){
        ret += PWRITE(conf->fd, conf->buffer, portion, start_addr);
    }
    printf("Thread %d write %lu Bytes\n", conf->tid, ret);

    pthread_exit(NULL);
}

void *seq_read_test(void *vargp){
    config *conf = (config*) vargp;
    uint64_t ret = 0;
    long long portion = conf->size / conf->tnum;
    long long start_addr = conf->tid * portion;
    printf("Thread %d created, read range is: %lld - %lld\n", conf->tid, start_addr, start_addr + portion);
    for(uint64_t i = 0; i < conf->round; i++){
        ret += PREAD(conf->fd, conf->buffer, portion, start_addr);
    }
    printf("Thread %d read %lu Bytes\n", conf->tid, ret);
    pthread_exit(NULL);
}


void *rnd_read_test(void *vargp){
    config *conf = (config*) vargp;
    for(int i = 0; i < conf->round; i++){
        PREAD(conf->fd, conf->buffer, conf->blk_size, conf->rnd_addrs[i]);
    }

    pthread_exit(NULL);
}

void *rnd_write_test(void *vargp){
    config *conf = (config*) vargp;
    for(int i = 0; i < conf->round; i++){
        //write 1s to the pre-determined addresses
        PWRITE(conf->fd, conf->rnd_buf, conf->blk_size, conf->rnd_addrs[i]);
    }

    pthread_exit(NULL);
}


int main(int argc, char ** argv){
	if(argc != 5){
		printf("usage: path_to_folder num_thread size round\n");
		return -1;
	}
	uint64_t total_bytes = 0, real_time;
    struct timespec stopwatch_start;
	struct timespec stopwatch_stop;
	int fd;
    char *buffer;       //the data source for seq tests
    char *rnd_buf;      //the data source for rnd tests
	char *path = argv[1];
	int num_thread = atoi(argv[2]);
	long long size = atoll(argv[3]);
	int round = atoll(argv[4]);
	int rnd_blk_size = 4096;
    long long *rnd_addrs;
    static uint16_t seeds[3] = { 182, 757, 21 };

	if(size % 1024 != 0){
        printf("Error: Size must be a multiple of 1024");
        exit(-1);
	}
    printf("*************************************************************************\n");
    printf("Preparing the testing data.\n");
    if(size >= 1 * _GB){
        buffer = malloc(1 * _GB);  //cap the max buffer size to 1GB
        for(uint64_t i = 0; i < (1 * _GB / sizeof(char)); i++){
            buffer[i] = i % 256;    //hash function for later data consistance check
        }
    } else {
        buffer = malloc(size);  //init the buffer to custom size
        for(uint64_t i = 0; i < (size / sizeof(char)); i++){
            buffer[i] = i % 256;    //hash function for later data consistance check
        }
    }
    rnd_buf = malloc(rnd_blk_size);
    for(uint64_t i = 0; i < (rnd_blk_size / sizeof(char)); i++){
            rnd_buf[i] = 255;     //any area that wrote by rnd test will be 1 filled
    }
    //preparing the random test address list
    rnd_addrs = malloc(sizeof(long long) * round);
    for(int i = 0; i < round; i++){
        rnd_addrs[i] = nrand48(seeds) % (size - rnd_blk_size + 1);
        printf("%09x\n", rnd_addrs[i]);
    }

    printf("Tesing data is ready.\n");
    printf("*************************************************************************\n\n");
    pthread_t threads[num_thread];
    config conf[num_thread];


    /*************************************************************************
    * beginning of the single file multi-thread sequential write test.
    *************************************************************************/
    printf("Starting %d thread(s) SEQ Write Test\n", num_thread);
    printf("*************************************************************************\n");
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
    fd = OPEN (path, O_WRONLY | O_CREAT | O_SYNC, S_IRWXU);
    for(int i = 0; i < num_thread; i++){
        conf[i].fd = fd;
        conf[i].tid = i;
        conf[i].round = round;
        conf[i].tnum = num_thread;
        conf[i].size = size;
        conf[i].buffer = buffer;
        pthread_create(&threads[i], NULL, seq_write_test, (void*)&conf[i]);
    }

    for(int i = 0; i < num_thread; i++)
    {
        pthread_join(threads[i], NULL);
    }
    //close file here for writeback delay
    CLOSE(fd);
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
    real_time = (uint64_t)calc_diff(stopwatch_start, stopwatch_stop);
    total_bytes = size * round;
    printf("Sequential Write Test Completed: \n");
    printf("\tTotal Time Used: %lu ns\n", real_time);
    printf("\tTotal Byte Write in %d rounds: %lu bytes\n", round, total_bytes);
    printf("\tAverage Write Speed: %f GB/s\n", (double)total_bytes / (double)real_time);
    printf("\n");


    /*************************************************************************
    * beginning of the single file multi-thread sequential read test.
    *************************************************************************/
    printf("Starting %d thread(s) SEQ Read Test\n", num_thread);
    printf("*************************************************************************\n");
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
    fd = OPEN(path, O_RDONLY | O_SYNC, S_IRWXU);
    for(int i = 0; i < num_thread; i++){
        conf[i].fd = fd;
        conf[i].tid = i;
        conf[i].round = round;
        conf[i].tnum = num_thread;
        conf[i].size = size;
        conf[i].buffer = buffer;
        pthread_create(&threads[i], NULL, seq_read_test, (void*)&conf[i]);
    }

    for(int i = 0; i < num_thread; i++)
    {
        pthread_join(threads[i], NULL);
    }
    //close file here for writeback time
    CLOSE(fd);
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
    real_time = (uint64_t)calc_diff(stopwatch_start, stopwatch_stop);
    total_bytes = size * round;
    printf("Sequential Read Test Completed: \n");
    printf("\tTotal Time Used: %lu ns\n", real_time);
    printf("\tTotal Byte Read in %d rounds: %lu bytes\n", round, total_bytes);
    printf("\tAverage Read Speed: %f GB/s\n", (double)total_bytes / (double)real_time);

    for(uint64_t i = 0; i < (size / sizeof(char)); i++)
    {
        if(buffer[i] != ((char)i % 256)){
            printf("Error: Data varification failed!\n");
            exit(-1);
        }
    }
    printf("\n");


    /*************************************************************************
    * beginning of the single file multi-thread random read test.
    *************************************************************************/
    printf("Starting %d thread(s) RND Read Test\n", num_thread);
    printf("*************************************************************************\n");
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
    fd = OPEN(path, O_RDONLY | O_SYNC, S_IRWXU);
    for(int i = 0; i < num_thread; i++){
        conf[i].fd = fd;
        conf[i].tid = i;
        conf[i].round = round;
        conf[i].tnum = num_thread;
        conf[i].size = size;
        conf[i].blk_size = rnd_blk_size;
        conf[i].buffer = buffer;
        conf[i].rnd_addrs = rnd_addrs;
        conf[i].rnd_buf = rnd_buf;
        pthread_create(&threads[i], NULL, rnd_read_test, (void*)&conf[i]);
    }

    for(int i = 0; i < num_thread; i++)
    {
        pthread_join(threads[i], NULL);
    }
    //close file here for writeback time
    CLOSE(fd);
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
    real_time = (uint64_t)calc_diff(stopwatch_start, stopwatch_stop);
    total_bytes = rnd_blk_size * round * num_thread;
    printf("Random Read Test Completed with %d Bytes Block: \n", rnd_blk_size);
    printf("\tTotal Time Used: %lu ns\n", real_time);
    printf("\tTotal Byte Read in %d rounds: %lu bytes\n", round, total_bytes);
    printf("\tAverage Read Speed: %f GB/s\n", (double)total_bytes / (double)real_time);
    printf("\n");


    /*************************************************************************
    * beginning of the single file multi-thread random write test.
    *************************************************************************/
    printf("Starting %d thread(s) RND Write Test\n", num_thread);
    printf("*************************************************************************\n");
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_start);
    fd = OPEN(path, O_WRONLY | O_SYNC, S_IRWXU);
    for(int i = 0; i < num_thread; i++){
        conf[i].fd = fd;
        conf[i].tid = i;
        conf[i].round = round;
        conf[i].tnum = num_thread;
        conf[i].size = size;
        conf[i].blk_size = rnd_blk_size;
        conf[i].buffer = buffer;
        conf[i].rnd_addrs = rnd_addrs;
        conf[i].rnd_buf = rnd_buf;
        pthread_create(&threads[i], NULL, rnd_write_test, (void*)&conf[i]);
    }

    for(int i = 0; i < num_thread; i++)
    {
        pthread_join(threads[i], NULL);
    }
    //close file here for writeback time
    CLOSE(fd);
    clock_gettime(CLOCK_MONOTONIC, &stopwatch_stop);
    real_time = (uint64_t)calc_diff(stopwatch_start, stopwatch_stop);
    total_bytes = rnd_blk_size * round * num_thread;
    printf("Random Write Test Completed with %d Bytes Block: \n", rnd_blk_size);
    printf("\tTotal Time Used: %lu ns\n", real_time);
    printf("\tTotal Byte Write in %d rounds: %lu bytes\n", round, total_bytes);
    printf("\tAverage Write Speed: %f GB/s\n", (double)total_bytes / (double)real_time);

    fd = OPEN(path, O_WRONLY | O_SYNC, S_IRWXU);
    for(int i = 0; i < round; i++)
    {
        pread(fd, rnd_buf, rnd_blk_size, rnd_addrs[i]);
        for(uint64_t j = 0; j < (rnd_blk_size / sizeof(char)); j++){
            if((unsigned)rnd_buf[j] != (char)255){
                printf("Error: Data varification failed!\n");
                printf("Expect: 255, but got: %i\n", (int)rnd_buf[j]);
                exit(-1);
            }
        }
    }
    CLOSE(fd);
    printf("\n");


    /*************************************************************************
    * beginning of the single file multi-thread random read/write test.
    *************************************************************************/


	return 0;
}
