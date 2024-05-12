#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>


int main(void){
	int fd[40];
	for(int i = 0; i < 40; i++){
		srand((unsigned int)time(NULL) + i);
		char title[20] = "serverDB";
		char txt[20] = ".txt";
		int len = strlen(title);
		if(i < 10){
			char num = i + '0';
			title[len] = num;
			title[len+1] = '\0';
		}
		else{
			char num[2];
			num[0] = i / 10 + '0';
			num[1] = i % 10 + '0';
			title[len] = num[0];
			title[len+1] = num[1];
			title[len+2] = '\0';
		}
		strcat(title, txt);

		fd[i] = open(title, O_WRONLY | O_CREAT, 0644);
		for(int j = 0;  j < 500000; j++){
			char str_buf[30];
			char int_buf[30];
			for(int m = 0; m < 5; m++){
				str_buf[m] = 'a' + rand() % 26;
			}
			str_buf[5] = '\\';
			str_buf[6] = '\0';
			for(int m = 0; m < 4; m++){
				int_buf[m] = 48 + rand() % 10; 
			}
			int_buf[4] = '\\';
			int_buf[5] = '\0';

			write(fd[i], str_buf, strlen(str_buf));
			write(fd[i], int_buf, strlen(int_buf));
			char tmp[] = "\n";
			write(fd[i], tmp, strlen(tmp));
		}
		close(fd[i]);
		printf("%d completed !\n", i);
	}

	return 0;
}
