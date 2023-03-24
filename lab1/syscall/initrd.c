#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

int main()
{
    char buffer1[40] = "\0";
    char buffer2[30] = "\0";

    long int sign1 = syscall(548, buffer1, 40);
    printf("buffer_size:%d, ret:%ld, buffer_content:%s\n",40, sign1, buffer1);

    long int sign2 = syscall(548, buffer2, 30);
    printf("buffer_size:%d, ret:%ld, buffer_content:%s\n",30, sign2, buffer2);

    return 0;
}