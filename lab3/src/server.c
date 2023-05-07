// server.c
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BIND_IP_ADDR "127.0.0.1"
#define BIND_PORT 8000
#define MAX_RECV_LEN 1048576
#define MAX_SEND_LEN 1048576
// 路径长度最大为4096
#define MAX_PATH_LEN 4096
#define MAX_HOST_LEN 1024
#define MAX_CONN 20

#define HTTP_STATUS_200 "200 OK"
#define HTTP_STATUS_404 "404 Not Found"
#define HTTP_STATUS_500 "500 Internal Server Error"

// 错误处理函数
void sol_error(char *error_msg)
{
    perror(error_msg);
    exit(EXIT_FAILURE);
}

int parse_request(char *request, ssize_t req_len, char *path, ssize_t *path_len)
{
    // 返回值为-1时，说明遇到本实验未定义的错误
    // 返回值为0时，说明在该函数中没有遇到错误
    char *req = request;
    ssize_t s1 = 3;
    ssize_t s2 = 5;
    // 访问路径不会跳出当前路径
    req[s1] = '.';
    // 该标志用于判断路径是否会跳出当前路径
    // 遇到"../"则标志自减，遇到"/"标志自增
    int sig_for_path = 0;
    while ((s2 - s1) <= MAX_PATH_LEN && req[s2] != ' ')
    {
        if (req[s2] == '/')
        {
            if (req[s2 - 1] == '.' && req[s2 - 2] == '.')
                sig_for_path--;
            else
                sig_for_path++;
        }
        if (sig_for_path < 0)
        {
            // 说明已经跳出当前路径
            return -1;
        }
        s2++;
    }
    if ((s2 - s1) > MAX_PATH_LEN)
        // 说明路径超过最大
        return -1;

    memcpy(path, req + s1, (s2 - s1 + 1) * sizeof(char));
    path[s2 - s1] = '\0';
    *path_len = (s2 - s1);
    return 0;
}

void handle_clnt(int clnt_sock)
{
    // 读取客户端发送来的数据，并解析
    // 将clnt_sock作为一个文件描述符，读取最多MAX_RECV_LEN个字符
    char *req_buf = (char *)malloc(MAX_RECV_LEN * sizeof(char));
    req_buf[0] = '\0';
    ssize_t req_len = 0;
    ssize_t len = 0;
    // buffer存储中间过程读的内容
    char *buffer = (char *)malloc(MAX_RECV_LEN * sizeof(char));
    // 构造要返回的数据
    char *response = (char *)malloc(MAX_SEND_LEN * sizeof(char));
    // 分析文件
    struct stat buf;

    while (1)
    {
        if ((len = read(clnt_sock, buffer, MAX_RECV_LEN)) < 0)
            sol_error("reading clnt_sock fails!\n");
        buffer[len] = '\0';
        strcat(req_buf, buffer);
        if (!strcmp(buffer + strlen(buffer) - 4, "\r\n\r\n"))
            break;
    }
    req_len = strlen(req_buf);

    if (req_len < 5 || strncmp(req_buf, "GET /", 5) != 0)
    {
        // 请求不完整/请求method不是GET
        sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n", HTTP_STATUS_500, (ssize_t)0);
        ssize_t response_len = strlen(response);
        ssize_t write_len = 0;
        while (response_len > 0)
        {
            // 通过clnt_sock向客户端发送信息
            // 将clnt_sock作为文件描述符写内容
            if ((write_len = write(clnt_sock, response, response_len)) < 0)
            {
                sol_error("writing clnt_sock fails!\n");
            }
            response = response + write_len;
            response_len = response_len - write_len;
        }
        // 关闭客户端套接字
        close(clnt_sock);
        // 释放内存
        // free(req_buf);
        // free(buffer);
        // free(response);
        return;
    }

    // 根据HTTP请求的内容，解析资源路径和Host头
    char *path = (char *)malloc(MAX_PATH_LEN * sizeof(char));
    ssize_t path_len;
    int sign = parse_request(req_buf, req_len, path, &path_len);

    if (sign == -1)
    {
        sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n", HTTP_STATUS_500, (ssize_t)0);
        ssize_t response_len = strlen(response);
        ssize_t write_len = 0;
        while (response_len > 0)
        {
            if ((write_len = write(clnt_sock, response, response_len)) < 0)
                sol_error("writing clnt_sock fails!\n");
            response = response + write_len;
            response_len = response_len - write_len;
        }
        // 关闭客户端套接字
        close(clnt_sock);
        // 释放内存
        // free(req_buf);
        // free(buffer);
        // free(response);
        // free(path);
        return;
    }
    else
    {
        int fd = open(path, O_RDONLY);
        if (fd < 0)
        {
            // 说明文件打开失败
            // 对应于404 Not Found
            sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n", HTTP_STATUS_404, (ssize_t)0);
            ssize_t response_len = strlen(response);
            ssize_t write_len = 0;
            while (response_len > 0)
            {
                if ((write_len = write(clnt_sock, response, response_len)) < 0)
                    sol_error("writing clnt_sock fails!\n");
                response = response + write_len;
                response_len = response_len - write_len;
            }
            // 关闭客户端套接字
            close(clnt_sock);
            // 释放内存
            // free(req_buf);
            // free(buffer);
            // free(response);
            // free(path);
            return;
        }
        else
        {
            if (fstat(fd, &buf) < 0)
                sol_error("Getting file attributes fails!\n");
            if (S_ISDIR(buf.st_mode) || !S_ISREG(buf.st_mode))
            {
                // 说明是目录
                // 或者不是标准文件
                sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n", HTTP_STATUS_500, (ssize_t)0);
                ssize_t response_len = strlen(response);
                ssize_t write_len = 0;
                while (response_len > 0)
                {
                    if ((write_len = write(clnt_sock, response, response_len)) < 0)
                        sol_error("writing clnt_sock fails!\n");
                    response = response + write_len;
                    response_len = response_len - write_len;
                }
                // 关闭客户端套接字
                close(clnt_sock);
                // 释放内存
                // free(req_buf);
                // free(buffer);
                // free(response);
                // free(path);
                return;
            }
            // 下面是正常读取的情况
            sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n", HTTP_STATUS_200, (ssize_t)buf.st_size);
            ssize_t response_len = strlen(response);
            ssize_t write_len = 0;
            while (response_len > 0)
            {
                if ((write_len = write(clnt_sock, response, response_len)) < 0)
                    sol_error("writing clnt_sock fails!\n");
                response = response + write_len;
                response_len = response_len - write_len;
            }
            // 读取文件内容
            if(read(fd, response, MAX_SEND_LEN) < 0)
                sol_error("Reading file fails!\n");
            response_len = strlen(response);
            write_len = 0;
            while (response_len > 0)
            {
                if ((write_len = write(clnt_sock, response, response_len)) < 0)
                    sol_error("writing clnt_sock fails!\n");
                response = response + write_len;
                response_len = response_len - write_len;
            }
        }
    }
    // 关闭客户端套接字
    close(clnt_sock);
    // 释放内存
    free(req_buf);
    free(path);
    free(response);
}

int main()
{
    // 创建套接字，参数说明：
    // AF_INET: 使用IPv4
    // SOCK_STREAM: 面向连接的数据传输方式
    // IPPROTO_TCP: 使用TCP协议
    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // 将套接字和指定的IP、端口绑定
    // 用0填充serv_addr（它是一个sockaddr_in结构体）
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    // 设置IPv4
    // 设置IP地址
    // 设置端口
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(BIND_IP_ADDR);
    serv_addr.sin_port = htons(BIND_PORT);
    // 绑定
    bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    // 使得serv_sock套接字进入监听状态，开始等待客户端发起请求
    listen(serv_sock, MAX_CONN);

    // 接收客户端请求，获得一个可以与客户端通信的新的生成的套接字clnt_sock
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);

    while (1) // 一直循环
    {
        // 当没有客户端连接时，accept()会阻塞程序执行，直到有客户端连接进来
        int clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        // 处理客户端的请求
        handle_clnt(clnt_sock);
    }

    // 实际上这里的代码不可到达，可以在while循环中收到SIGINT信号时主动break
    // 关闭套接字
    close(serv_sock);
    return 0;
}