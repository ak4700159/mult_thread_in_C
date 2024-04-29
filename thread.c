#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_THREAD 20
#define MAX_FILE_LINES 10000

#define SINGLE_SEARCH 1
#define MULTI_SEARCH 2
#define EXIT 3

pthread_mutex_t mtd[MAX_THREAD];
pthread_cond_t ctd[MAX_THREAD];
pthread_t td[MAX_THREAD];
int gold_set = 0;

// 스레드 함수, 마지막 반환값은 해당 서버의 평균골드량이다.
void* searchDB(void* info);
void cleanup();
void menu();
void multi_search();
void single_search();
void file_search(int index);

int main(int argc, char* argv[]){
	if(argc != 1){
		fprintf(stderr, "%s : failed usage...\n", argv[1]);
		exit(1);
	}
	printf("=== Single thread VS Thread pool ===\n");

	// 스레드 상태 초기화
	for(int i = 0; i < MAX_THREAD; i++){
		printf("%d thread init ...\n", i);
		pthread_mutex_init(&mtd[i], NULL);
		pthread_cond_init(&ctd[i], NULL);
	}

	int info[12];
	for(int i = 0; i < MAX_THREAD; i++){
		info[i] = i;
		if(pthread_create(&td[i], NULL, searchDB, (void*)&info[i]) != 0){
			fprintf(stderr, "thread create failed : index %d\n", i);
			exit(1);
		}
	}
	
	// 본격적으로 프로그램 시작
	char buf[10];
	while(1){
		menu();
		fgets(buf, 10, stdin);
		int num = atoi(buf);
		switch(num) {
			case EXIT:   printf("EXIT !!\n");            break;
			case SINGLE_SEARCH: printf("SINGLE SEARCH start !!\n"); single_search();   continue;
			case MULTI_SEARCH : printf("MULTI SEARCH start !!\n");  multi_search();  continue;
			default:     printf("잘못된 입력입니다.\n"); continue;
		}
		break;
	}

	return 0;
}

// 스레드 함수
void* searchDB(void * info){
	int* index = (int*)info;
	pthread_cond_wait(&ctd[*index], &mtd[*index]);
	printf("%d thread start! \n", *index);
	pthread_mutex_lock(&mtd[0]);
	file_search(*index);
	pthread_mutex_unlock(&mtd[0]);
}


void menu(){
	printf("\n ----- MENU ---- \n");
	printf("1. single serach\n");
	printf("2. multi search\n");
	printf("3. exit\n");
}

// 직렬적으로 파일 탐색
void single_search(){
	struct timeval before, after;
	char buf[10];
	printf("set gold : ");
	fgets(buf, 10, stdin);
	gold_set = atoi(buf);
	// 시간 측정 시작
	gettimeofday(&before, NULL);
	for(int i = 0; i < MAX_THREAD; i++){
		file_search(i);
	}
	gettimeofday(&after, NULL);
	float dif = after.tv_sec - before.tv_sec + 1e-6 * (after.tv_usec - before.tv_usec);
	printf("single thread time : %f sec\n", dif);
}

// 병렬적으로 파일 탐색
void multi_search(){
	struct timeval before, after;
	char buf[10];
	printf("set gold : ");
	fgets(buf, 10, stdin);
	gold_set = atoi(buf);
	// 시간 측정 시작
	gettimeofday(&before, NULL);
	for(int i = 0; i < MAX_THREAD; i++){
		pthread_cond_signal(&ctd[i]);
	}

	for(int i = 0; i < MAX_THREAD; i++){
		pthread_join(td[i], NULL);
	}
	gettimeofday(&after, NULL);
	float dif = after.tv_sec - before.tv_sec + 1e-6 * (after.tv_usec - before.tv_usec);
	printf("multi thread time : %f sec\n", dif);
}

void file_search(int index){
	char title[20] = "serverDB";
	char txt[20] = ".txt";
	char read_buf[20];
	int len = strlen(title);
	int count = 0;

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

	unsigned int sum = 0;
	int fd = open(title, O_RDONLY);
	if(fd == -1){
		fprintf(stderr, "fd2 open error\n");
		exit(1);
	}
	for(int w = 0; w < MAX_FILE_LINES; w++){
		// 원본파일 읽기
		if(read(fd, read_buf, 12) == -1){
			fprintf(stderr, "read error\n");
			close(fd);
			exit(1);
		}
		read_buf[12] = '\0';
		char* name = strtok(read_buf, "\\");
		char* str_gold = strtok(NULL, "\\");
		int gold = atoi(str_gold);
		sum += gold;
		if(gold_set < gold) count++;
	}
	close(fd);
	printf("serverDB%d은 평균 %d, %d 골드보다 많이 소지한 플레이어가 %d명 있습니다. \n", index, sum/MAX_FILE_LINES, gold_set, count);
}
