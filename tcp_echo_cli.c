/*****************************************************************
*Module Date:2020.6.1
*Description: TCP单进程循环服务器 客户端 client
*****************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define MAX_CMD_STR 100

int echo_rqt(int sockfd)
{
    char buf[MAX_CMD_STR + 1];

    while (fgets(buf, MAX_CMD_STR, stdin))
    {
        // 收到 exit，退出循环返回
        if (strncmp(buf, "exit", 4) == 0)
        {
            return 0;
        }
        // 查询所读取 1 行字符的长度，并将行末'\n'更换为'\0'
        int len = strnlen(buf, MAX_CMD_STR);
        // 将\n换为\0
        char *temp;
        if ((temp = strchr(buf, '\n')) != NULL)
            *temp = '\0';
        // 发送缓存数据总长度
        int count = write(sockfd, &len, sizeof(int));
        // 按指定长度发送缓存数据
        count = write(sockfd, buf, len);
        // 读取服务器 echo 回显数据，并打印输出到 stdout
        int res = read(sockfd, &len, sizeof(int));
        if (res < 0)
        {
            printf("[cli] read len return %d and errno is %d\n", res, errno);
            if (errno == EINTR)
                continue;
            return 0;
        }
        memset(buf, 0, len);
        res = read(sockfd, buf, len);
        printf("[echo_rep] %s\n", buf);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    // setbuf(stdout, NULL);
    int res;
    // 定义服务器 Socket 地址 srv_addr；
    char *srv_addr;
    int port;
    // 定义 Socket 连接描述符 connfd；
    int connfd;

    if (argc != 3)
    {
        printf("Usage:%s <IP> <PORT>\n", argv[0]);
        return 0;
    }
    // 初始化服务器 Socket 地址 srv_addr，其中会用到 argv[1]、argv[2]
    srv_addr = argv[1];
    port = atoi(argv[2]);
    // 获取 Socket 连接描述符: connfd = socket(x,x,x);
    connfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_sock;
    server_sock.sin_family = AF_INET;
    server_sock.sin_port = htons(port);
    server_sock.sin_addr.s_addr = inet_addr(srv_addr);
    do
    {
        res = connect(connfd, (const struct sockaddr *)&server_sock, sizeof(server_sock));
        if (res == 0)
        {
            printf("[cli] server[%s:%d] is connected!\n", srv_addr, port);
            int r = echo_rqt(connfd);
            if (r == 0)
                break;
        }
        else if (res == -1 && errno == EINTR)
        {
            // 若 connect 因系统信号中断而失败，则再次执行 connect；
            continue;
        }
    } while (1);
    // 关闭 connfd
    close(connfd);
    printf("[cli] connfd is closed!\n");
    printf("[cli] client is exiting!\n");
    return 0;
}
