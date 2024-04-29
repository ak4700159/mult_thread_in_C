#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#define MAX_LOOP 5000000
#define MAX_THREADS 50
#define MAX_BUFFER 20
#define RUNNING 1
#define WAITING 0

typedef struct Data{
	int index;
	int total;
	int thread_status;
}Data;

pthread_mutex_t mtd[MAX_THREADS];
pthread_cond_t ctd = PTHREAD_COND_INITIALIZER;
pthread_t td[MAX_THREADS];
void* thread_function(void* arg);
void multi_thread(Data* thread_data);
void single_thread();
void norm_exit(int signal);

int main(void){
	Data thread_data[MAX_THREADS];
	signal(SIGINT, norm_exit);
	signal(SIGSEGV, norm_exit);
	for(int i = 0; i < MAX_THREADS; i++){
		pthread_mutex_init(&mtd[i], NULL);
		thread_data[i].index = i;
		thread_data[i].thread_status = WAITING;
		if(pthread_create(&td[i], NULL, thread_function, (void*)&thread_data[i]) != 0){
			fprintf(stderr, "thread create failed : index %d\n", i);
			exit(1);
		}
		else{
			printf("%2d thread create\n", i);
		}
	}
	printf("basic setting done ... \n");
	printf("Let's go go go !\n");

	// 1. 싱글스레드 상황일 때
	struct timeval before, after;
	gettimeofday(&before, NULL);
	
	single_thread();

	gettimeofday(&after, NULL);
	float single_dif = after.tv_sec - before.tv_sec + 1e-6 * (after.tv_usec - before.tv_usec);
	printf("single thread time : %.3f sec\n\n\n", single_dif);
	// ------ 상황 종료 ------

	// 2. 멀티스레드 상황일 때
	gettimeofday(&before, NULL);

	multi_thread(thread_data);

	gettimeofday(&after, NULL);
	float multi_dif = after.tv_sec - before.tv_sec + 1e-6 * (after.tv_usec - before.tv_usec);
	printf("multi thread time : %.3f sec\n\n\n", multi_dif);


	// ------ 상황 종료 ------
	printf("======================================================================\n");
	printf("결론 : ");
	if(multi_dif * 5 < single_dif){
		printf("멀티 스레드가 싱글 스레드보다 5배 이상의 성능을 보였습니다. \n");
	}
	else{
		printf("멀티 스레드가 싱글 스레드보다 5배 이상의 성능을 보이지 못했습니다.\n");
	}
	printf("======================================================================\n");

	pthread_cond_destroy(&ctd);
	for(int i = 0; i < MAX_THREADS; i++){
		pthread_mutex_destroy(&mtd[i]);
	}
}

// 스레드 함수
void* thread_function(void* arg){
	Data* thread_data = (Data*)arg;
	if(thread_data->index < 0 || thread_data->index > 49){
		printf("error thread exit : %d\n", thread_data->index);
		pthread_exit(NULL);
	}
	pthread_mutex_lock(&mtd[thread_data->index]);
	pthread_cond_wait(&ctd, &mtd[thread_data->index]);
	printf("%2d thread cond start\n", thread_data->index);

	int tmp[MAX_BUFFER];
	thread_data->total = 0;
	for(int m = 0; m < MAX_BUFFER; m++){
		tmp[m] = 0;
		for(int i = 0; i < MAX_LOOP; i++){
			if(i % 1000 == 0)
				tmp[m] += 1;
		}
		thread_data->total += tmp[m];
	}
	thread_data->thread_status = WAITING;
	pthread_mutex_unlock(&mtd[thread_data->index]);
}

// 싱글 스레드 실행 함수
void single_thread(){
	int total = 0;
	for(int j = 0; j < MAX_THREADS; j++){
		for(int m = 0; m < MAX_BUFFER; m++){
			for(int i = 0; i < MAX_LOOP; i++){
				if(i % 1000 == 0)
					total += 1;
			}
		}
	}

	printf("--- total sum : %d ---\n", total);
}

// 멀티 스레드 실행 함수
void multi_thread(Data* thread_data){
	int total_sum = 0;
	pthread_cond_broadcast(&ctd);

	for(int i = 0; i < MAX_THREADS; i++){
		thread_data[i].thread_status = RUNNING;
	}

	while(1){
		int count = 0;
		for(int i = 0; i < MAX_THREADS; i++)
			if(thread_data[i].thread_status == WAITING)
				count++;
		if(count == MAX_THREADS)
			break;
	}

	for(int i = 0; i < MAX_THREADS; i++){
		total_sum += thread_data[i].total;
	}
	printf("--- total sum : %d ---\n", total_sum);
}


// signal handler 함수
void norm_exit(int signal){
	pthread_cond_destroy(&ctd);
	for(int i = 0; i < MAX_THREADS; i++){
		pthread_detach(td[i]);
		pthread_mutex_destroy(&mtd[i]);
	}
	exit(1);
}
