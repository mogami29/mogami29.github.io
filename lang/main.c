#include <stdio.h>

main(){
	printf("Hello world!\n");
	printf("%d", sizeof(long long));
	int c;
	for(;;){
		c=getc(stdin);
		printf("%c", '.');
	}
}
