/*****************************************************************
*Module Date: 2020.6.1
*Description: TCP单进程循环服务器 服务端 server
*****************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <arpa/inet.h>
#include <signal.h>

#define MAX_CMD_STR 100

#define _BACKLOG 10

// 全局变量 sig_to_exit，用于指示服务器程序是否退出；
int sig_to_exit = 0;
int sig_type = 0;

// SIGINT 信号处理函数；
void sig_int(int signo)
{
    // TODO 记录本次系统信号编号；
    sig_type = signo;
    // TODO 设置全局变量，以提示服务器程序释放资源；
    sig_to_exit = 1;
}

// SIGPIPE 信号处理函数；
void sig_pipe(int signo)
{
    // TODO 记录本次系统信号编号；
    sig_type = signo;
}

// 业务逻辑处理函数
void echo_rep(int sockfd)
{
    //  定义数据长度变量 len，以及读取结果变量 res
    int len;
    int res;
    //  定义缓存指针变量 buf;
    char *buf;
    do
    {
        // (1)  读取数据长度： res = read(x,x,x);
        res = read(sockfd, &len, sizeof(int));
        // 以下代码紧跟 read();
        if (res < 0)
        {
            printf("[srv] read len return %d and errno is %d\n", res, errno);
            if (errno == EINTR)
            {
                if (sig_type == SIGINT)
                {
                    return;
                }
                continue;
            }
            return;
        }
        if (res == 0)
            return;
        // (2) 按长读取数据
        //  采用 malloc，根据数据长度分配 buf
        buf = (char *)malloc(len);
        // memset(buf, 0, len);
        //  按长读取数据： res = read(x,x,x);

        res = read(sockfd, buf, len);

        // 以下代码紧跟 read();
        if (res < 0)
        {
            printf("[srv] read data return %d and errno is %d\n", res, errno);
            if (errno == EINTR)
            {
                if (sig_type == SIGINT)
                {
                    free(buf);
                    return;
                }
                continue;
            }
            free(buf);
            return;
        }
        if (res == 0)
            return;

        // 本轮数据长度以及数据本身的读取结束：
        // 按题设要求打印接收到的[echo_rqt]信息；
        printf("[echo_rqt] %s\n", buf);
        // 回写客户端[echo_rep]信息；根据读写边界定义，
        // 同样需先发长度，再发数据：res = write(x,x,x);res = write(x,x,x);
        res = write(sockfd, &len, sizeof(int));
        if (res < 0)
        {
            printf("[srv] read len return %d and errno is %d\n", res, errno);
            if (errno == EINTR)
                continue;
            return;
        }
        res = write(sockfd, buf, len);
        if (res < 0)
        {
            printf("[srv] read len return %d and errno is %d\n", res, errno);
            if (errno == EINTR)
                continue;
            return;
        }

        // 发送结束，释放 buf
        free(buf);
    } while (1);
}

int main(int argc, char *argv[])
{
    // setbuf(stdout, NULL);
    int res;
    // 安装 SIGPIPE 信号处理器
    struct sigaction sigact_pipe, old_sigact_pipe;
    sigact_pipe.sa_handler = sig_pipe;
    sigemptyset(&sigact_pipe.sa_mask);
    sigact_pipe.sa_flags = 0;
    //设置受影响调用立刻重启
    sigact_pipe.sa_flags |= SA_RESTART;
    sigaction(SIGPIPE, &sigact_pipe, &old_sigact_pipe);
    // 安装 SIGINT 信号处理器
    struct sigaction sigact_int;
    sigact_int.sa_handler = sig_int;
    sigemptyset(&sigact_int.sa_mask);
    sigact_int.sa_flags = 0;
    sigaction(SIGINT, &sigact_int, NULL);
    // 定义服务器 Socket 地址 srv_addr，以及客户端 Socket 地址 cli_addr
    char *srv_addr;
    struct sockaddr_in cli_addr;
    // 定义客户端 Socket 地址长度 cli_addr_len（类型为 socklen_t）；
    socklen_t cli_addr_len;
    // 定义 Socket 监听描述符 listenfd，以及 Socket 连接描述符 connfd；
    int listenfd, connfd;
    // 初始化服务器 Socket 地址 srv_addr，其中会用到 argv[1]、argv[2]
    /* IP 地址转换推荐使用 inet_pton()；端口地址转换推荐使用 atoi(); */
    srv_addr = argv[1];
    int srv_port = atoi(argv[2]);
    // 按题设要求打印服务器端地址 server[ip:port]，推荐使用 inet_ntop();
    printf("[srv] server[%s:%d] is initializing!\n", srv_addr, srv_port);
    // 获取 Socket 监听描述符: listenfd = socket(x,x,x);
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    // 绑定服务器 Socket 地址: res = bind(x,x,x);
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_port = htons(srv_port);
    local.sin_addr.s_addr = inet_addr(srv_addr);
    res = bind(listenfd, (struct sockaddr *)&local, sizeof(struct sockaddr_in));
    // 开启服务监听: res = listen(x,x);
    res = listen(listenfd, _BACKLOG);
    // 开启 accpet()主循环，直至 sig_to_exit 指示服务器提出；
    while (!sig_to_exit)
    {
        // 获取 cli_addr 长度，执行 accept()：connfd = accept(x,x,x);
        cli_addr_len = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_addr_len);
        if (connfd < 0)
        {
            continue;
        }
        // 若上述 if 判断不成立且 connfd<0，则重启 accept();
        // 按题设要求打印客户端端地址 client[ip:port]，推荐使用 inet_ntop();
        char bufip[32];
        bufip[0] = 0;
        inet_ntop(AF_INET, &cli_addr.sin_addr, bufip, sizeof(bufip));
        printf("[srv] client[%s:%d] is accepted!\n", bufip, ntohs(cli_addr.sin_port));
        // 执行业务处理函数 echo_rep()，进入业务处理循环;
        echo_rep(connfd);
        // 业务函数退出，关闭 connfd;
        // int lenaddr = 0;
        // int sockwork = accept(connfd, (struct sockaddr *)&cli_addr, &lenaddr);

        close(connfd);
        printf("[srv] connfd is closed!\n");
    }
    // accpet()主循环结束，关闭 lstenfd;
    if (sig_to_exit)
    {
        printf("[srv] SIGINT is coming!\n");
    }
    close(listenfd);
    printf("[srv] listenfd is closed!\n");
    printf("[srv] server is exiting!\n");
    return 0;
}
