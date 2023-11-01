#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p1[2], p2[2];

  pipe(p1);
  pipe(p2);

  if(fork() == 0){
    char byte;

    close(p1[1]);   // 子进程不需要p1的写端
    close(p2[0]);   // 子进程不需要p2的读端
    
    read(p1[0], &byte, sizeof(byte));
    printf("%d: received ping\n", getpid());

    write(p2[1], &byte, sizeof(byte));

    close(p1[0]);
    close(p2[1]);

    exit(0);
  }else{
    char byte = 's';

    close(p1[0]);   // 父进程不需要p1的读端
    close(p2[1]);   // 父进程不需要p2的写端
    
    write(p1[1], &byte, sizeof(byte));

    read(p2[0], &byte, sizeof(byte));
    printf("%d: received pong\n", getpid());

    close(p1[1]);
    close(p2[0]);

    exit(0);
  }
}
