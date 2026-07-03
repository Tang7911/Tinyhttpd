#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(4000);

    int result = connect(fd, (struct sockaddr *)&address, sizeof(address));
    if(result < 0)
    {
        perror("connect 111");
        exit(EXIT_FAILURE);
    }
    char request[256] = "GET / HTTP/1.0\r\n\r\n"; 
    if(write(fd, request, strlen(request)) < 0) 
    {
        perror("write 111");
        exit(EXIT_FAILURE);
    }
    int n = read(fd, &request, sizeof(request));
    if(n > 0)
    {
        printf("读取数据：%s\n", request);
    }
    close(fd);

    return 0;
}