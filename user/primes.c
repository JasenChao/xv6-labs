#include "kernel/types.h"
#include "user/user.h"

int main() {
    int p1[2];
    pipe(p1);

    if (fork()) {
        // 父进程
        close(p1[0]);

        for (int i = 2; i < 36; ++i) {
            write(p1[1], &i, sizeof i);
        }

        close(p1[1]);
    } else {
        // 子进程
        close(p1[1]);

        int base;
        if (read(p1[0], &base, sizeof base) == 0) exit(1);
        printf("prime %d\n", base);

        int p2[2];
        pipe(p2);

        if (fork()) {
            // 还是子进程
            close(p2[0]);
            
            int n;
            while (read(p1[0], &n, sizeof n)) {
                if (n % base != 0) write(p2[1], &n, sizeof n);
            }

            close(p2[1]);
        } else {
            // 孙进程
            close(p2[1]);

            if (read(p2[0], &base, sizeof base) == 0) exit(1);
            printf("prime %d\n", base);

            int p3[2];
            pipe(p3);

            if (fork()) {
                // 还是孙进程
                close(p3[0]);

                int n;
                while (read(p2[0], &n, sizeof n)) {
                    if (n % base != 0) write(p3[1], &n, sizeof n);
                }

                close(p3[1]);
            } else {
                // 曾孙进程
                close(p3[1]);

                while (read(p3[0], &base, sizeof base)) {
                    int flag = 0;
                    for (int i = 2; i * i <= base; ++i) {
                        if (base % i == 0) {
                            flag = 1;
                            break;
                        }
                    }
                    if (flag == 0) printf("prime %d\n", base);
                }
            }
        }

        close(p1[0]);
    }

    wait(0);

    return 0;
}