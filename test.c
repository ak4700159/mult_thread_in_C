#include <stdio.h>
#include <pthread.h>

pthread_t t1;
pthread_t t2;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex;

int t1_count = 0;
int t2_count = 0;
void* thread_func1(void* tmp){
	pthread_cond_wait(&cond, &mutex);
	while(t1_count < 100000000){
		if(t1_count % 500000 == 0){
			printf("t1_count : %d\n", t1_count);
		}
		t1_count++;
	}
	printf("thread_func1 end\n");
}
void* thread_func2(void* tmp){
	while(t2_count < 100000000){
		if(t2_count % 500000 == 0){
			printf("t2_count : %d\n", t2_count);
		}
		t2_count++;
	}
	printf("thread_func2 end\n");
}

int main(int argv, char* argc){
	pthread_mutex_init(&mutex, NULL);
	pthread_create(&t1, NULL, thread_func1, NULL);
	pthread_create(&t2, NULL, thread_func2, NULL);

	pthread_join(t2, NULL);
	pthread_cond_signal(&cond);
	pthread_join(t1, NULL);
	return 0;
}
