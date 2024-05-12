#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#define MAX_FILE_LINES 500000
#define MAX_THREADS 40
#define SINGLE_THREAD 1
#define MULTI_THREAD 0
#define MAX_BUFFER 20

typedef struct Data{
	int index;
	int total;
}Data;

pthread_mutex_t mtd[MAX_THREADS];
pthread_cond_t ctd = PTHREAD_COND_INITIALIZER;
pthread_t td[MAX_THREADS];
void* thread_function(void* arg);
void multi_thread(Data* thread_data);
void single_thread();
void norm_exit(int signal);
int joint_routine(int status, int index);
int read_file(int index);
void edit_title(char* title, int index);
void init(Data* thread_data);

int main(void){
	Data thread_data[MAX_THREADS];
	init(thread_data);

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
		printf("성능 : %.3f 배\n", single_dif / multi_dif);
	}
	else{
		printf("멀티 스레드가 싱글 스레드보다 5배 이상의 성능을 보이지 못했습니다.\n");
		printf("성능 : %.3f 배\n", single_dif / multi_dif);
	}
	printf("======================================================================\n");

	pthread_cond_destroy(&ctd);
	for(int i = 0; i < MAX_THREADS; i++)
		pthread_mutex_destroy(&mtd[i]);
}

// 스레드 함수
void* thread_function(void* arg){
	Data* thread_data = (Data*)arg;
	if(thread_data->index < 0 || thread_data->index >= MAX_THREADS){
		printf("error thread exit : %d\n", thread_data->index);
		pthread_exit(NULL);
	}
	pthread_mutex_lock(&mtd[thread_data->index]);
	pthread_cond_wait(&ctd, &mtd[thread_data->index]);
	//printf("%2d thread cond start\n", thread_data->index);

	thread_data->total = joint_routine(MULTI_THREAD, thread_data->index);
	pthread_mutex_unlock(&mtd[thread_data->index]);
}

// 싱글 스레드 실행 함수
void single_thread(){
	int total = joint_routine(SINGLE_THREAD, -1);
	printf("--- total sum : %d ---\n", total);
}

// 멀티 스레드 실행 함수
void multi_thread(Data* thread_data){
	int total_sum = 0;
	int ret;
	pthread_cond_broadcast(&ctd);

	for(int i = 0; i < MAX_THREADS; i++){
		if(ret = pthread_join(td[i], (void**)NULL) == 0){
			//printf("thread %2d : pthread_join success ! \n", thread_data[i].index);
		}
		else if(ret == EDEADLK){
			printf("thread %d : DEADLOCK detected ! \n", thread_data[i].index);
			norm_exit(1);
		}
		else if(ret == EINVAL){
			printf("thread %d : pthread_join is not joinanble ! \n", thread_data[i].index);
			norm_exit(1);
		}
		else if(ret == ESRCH){
			printf("thread %d : No thread with given ID is found ! \n", thread_data[i].index);
			norm_exit(1);
		}
		else{
			printf("thread %d : Error ocurred when joining the thread ! \n", thread_data[i].index);
			norm_exit(1);
		}
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

// 싱글스레드, 멀티스레드 공동 루틴 정의
// status == 1 -> single thread로 작동
// status == 0 -> multi thread로 작동
// index는 멀티스레로 작동시 필요한 부분이다.
int joint_routine(int status, int index){
	int gold = 0;
	
	if(status == SINGLE_THREAD){
		for(int i = 0; i < MAX_THREADS; i++){
			gold += read_file(i);
		}
	}
	else if(status == MULTI_THREAD){
		gold += read_file(index);
	}

	return gold;
}

void edit_title(char* title, int index){
	char txt[MAX_BUFFER] = ".txt";
	int len = strlen(title);

	if(index < 10){
		char num = index + '0';
		title[len] = num;
		title[len+1] = '\0';
	}
	else{
		char num[2];
		num[0] = index / 10 + '0';
		num[1] = index % 10 + '0';
		title[len] = num[0];
		title[len+1] = num[1];
		title[len+2] = '\0';
	}
	strcat(title, txt);
}

// 하나의 파일을 읽고 찾고자 하는 플레이어 아이디와
// 동일한 아이디가 있으면 해당 플레이어의 골드량
// 더한다. 중복을 허용하기에 끝까지 읽어 들여서
// 총합을 반환한다.
int read_file(int index){
	int gold = 0;
	char title[MAX_BUFFER] = "serverDB";
	char read_buf[MAX_BUFFER];
	char find_name[MAX_BUFFER] = " tmp ";
	edit_title(title, index);
	int fd = open(title, O_RDONLY);
	if(fd == -1){
		fprintf(stderr, "fd open error\n");
		norm_exit(1);
	}
	for(int w = 0; w < MAX_FILE_LINES; w++){
		// 원본파일 읽기
		if(read(fd, read_buf, 12) == -1){
			fprintf(stderr, "read error\n");
			close(fd);
			norm_exit(1);
		}
		read_buf[12] = '\0';
		char* name = strtok(read_buf, "\\");
		char* str_gold = strtok(NULL, "\\");
		if(strncmp(name, "vnovn", 5) == 0){
			int find_gold = atoi(str_gold);
			gold += find_gold;
			printf("%2d server : %s 플레이어 존재, 골드 소지량 : %d 확인\n", index, name, find_gold);
		}
	}
	close(fd);
	return gold;
}

// 초기 작업
// Data 생성 및 반환
// 스레드 생성 및 mutex, cond 할당
// signal handler 등록  (C^, segment fault)
void init(Data* thread_data){
	signal(SIGINT, norm_exit);
	signal(SIGSEGV, norm_exit);
	for(int i = 0; i < MAX_THREADS; i++){
		pthread_mutex_init(&mtd[i], NULL);
		thread_data[i].index = i;
		if(pthread_create(&td[i], NULL, thread_function, (void*)&thread_data[i]) != 0){
			fprintf(stderr, "thread create failed : index %d\n", i);
			exit(1);
		}
		else{
			//printf("%2d thread create\n", i);
		}
	}
	printf("basic setting completed.\n");
}
