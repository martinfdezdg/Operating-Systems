#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <math.h> 

static int fileDev;

#define MS 1000
#define PATH "/dev/leds"

void reset(){
	write(fileDev,"",1);
}

int elevate(int base, int exp){
	int result = 1;
	while (exp != 0){
		result*=base;
		--exp;
	}
	return result;
}

void showBinaryTrad(int i){
	if (i == 0) write(fileDev,"",1);
	else if (i == 1) write(fileDev,"3",1);
	else if (i == 2) write(fileDev,"2",1);
	else if (i == 3) write(fileDev,"23",2);
	else if (i == 4) write(fileDev,"1",1);
	else if (i == 5) write(fileDev,"13",2);
	else if (i == 6) write(fileDev,"12",2);
	else write(fileDev,"123",3);
}

void loading() {
	char* led = "123";
	int ledSize = strlen(led);
	int i;
	for (i = 0; i < ledSize*10; ++i){
		write(fileDev,&led[i%ledSize],1);
		usleep(150*MS);
	}
}

void pingpong() {
	char* led = "123";
	int ledSize = strlen(led);
	int i,j;
	for (i = 0; i < 10; ++i){
		for (j = 0; j < ledSize; j++) {
			write(fileDev,&led[j],1);
			usleep(150*MS);
		}
		for (j = ledSize-2; j > 0; j--) {
			write(fileDev,&led[j],1);
			usleep(150*MS);
		}
	}
}

void ovni() {
	int i;
	for (i = 0; i < 10; ++i){
		write(fileDev,"2",1);
		usleep(150*MS);
		write(fileDev,"13",2);
		usleep(150*MS);
	}
}

void custom() {
	char* led = "123";
	printf("Press 1, 2 or 3 to turn on/off the leds.\n");
	printf("Exit with any other number.\n");

	bool bled[4] = {false};
	int counter;

	bool exit = false;
	int number = -1;
	while (!exit){
		scanf("%d", &number);
		if (number < 1 || number > 3) exit = true;
		else {
			counter = 0;
			for (int i = 1; i < 4; ++i){
				if (number == i) bled[i] = !bled[i];
			}
			for (int i = 1; i < 4; ++i){
				if (bled[i]) counter += elevate(2,3-i);
			}
			printf("%d\n",counter);
			showBinaryTrad(counter);
		}
	}
}

void binaryCounter() {
	int i;
	for (i = 0; i <= 7; i++) {
		showBinaryTrad(i);
		usleep(1000*MS);
	}
}

int menu() {
	int order = -1;
	bool error = true;

	printf("\n");
	printf("# - PLAYS\n");
	printf("1 | Loading\n");
	printf("2 | Ping Pong\n");
	printf("3 | Ovni\n");
	printf("4 | Custom\n");
	printf("# - UTILS\n");
	printf("5 | Binary counter\n");
	printf("0 - EXIT\n");


	while (error) {
		printf("> ");
		scanf("%d", &order);
		if (order >= 0 && order <= 5) error = false;
	}

	return order;
}

int main() {
	printf("\n  | LED CONTROLLER |\n");
	
	int order = -1;
	fileDev = open(PATH,O_RDWR);
	while (order != 0) {
		order = menu();
		if (order != 0) printf("\nLook at your leds.\n");
		if (order == 1) loading();
		if (order == 2) pingpong();
		if (order == 3) ovni();
		if (order == 4) custom();
		if (order == 5) binaryCounter();
		if (order != 0) printf("Finished.\n");			
		reset();
	}
	close(fileDev);

	return 0;
}