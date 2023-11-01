#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    char *cmd;                                                      // 记录xargs后面的命令
    char **new_argv;                                                // 记录xargs后面的命令需要的参数

    cmd = argv[1];                                                  // xargs后面的第一个参数就是命令
    new_argv = malloc(argc * sizeof(char *));                       // 申请argc个指针大小的内存
    for (int i = 0; i < argc - 1; ++i) new_argv[i] = argv[i + 1];   // 将除了xargs以外的内容都保存下来

    char buf[512];                                                  // 用来保存xargs前面的指令输出的内容
    char *pos = buf;                                                // 填充buf的下标指针
    while (read(0, pos, sizeof(char)) > 0) {                        // 每次读一个char，直到没有输出，也就是前面命令的输出内容全部读完了
        if (*pos == '\n' || *pos == '\0') {                         // 如果读出来是换行或者结束符，意味着一条参数读完了，将其统一修改为结束符
            *pos = '\0';
            if (fork() == 0) {                                      // 将最后一个参数补齐，由子进程来完成
                new_argv[argc - 1] = buf;
                exec(cmd, new_argv);
            } else {                                                // 父进程的buf清空，pos归位，准备再读下一条参数
                memset(buf, 0, 512);
                pos = buf;
            }
        } else pos++;                                               // 读到换行和结束符以外的内容就正常写入buf
    }
    wait(0);                                                        // 等待子进程全部结束
    free(new_argv);                                                 // 释放前面malloc的内存

    return 0;
}