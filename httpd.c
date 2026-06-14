#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>
#include<netinet/in.h>

int startup(short *port)
{
    int httpd = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    struct sockaddr_in name;

    if(httpd == -1)
    {
        perror("socket 111");
        exit(EXIT_FAILURE);
    }

    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    if(setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0)
    {
        perror("setsockopt 111");
        close(httpd);
        exit(EXIT_FAILURE);
    }
    if(bind(httpd, (const struct sockaddr *)&name, sizeof(name)) < 0)
    {
        perror("bind 111");
        close(httpd);
        exit(EXIT_FAILURE);
    }
    if(listen(httpd, 5) < 0)
    {
        perror("listen 111");
        close(httpd);
        exit(EXIT_FAILURE);
    }
    printf("listen...\n");
    return httpd;
}

int main()
{
    short n = 4000;
    int fd = startup(&n);

    while(1)
    {
        int connfd = accept(fd, NULL, NULL);
        if(connfd < 0)
        {
            perror("accept 111");
            continue;
        }
        printf("成功接受连接！\n");

        const char *http_response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 13\r\n\r\nHello, World!";
        send(connfd, http_response, strlen(http_response), 0);

        close(connfd);
        printf("成功断开连接！\n");
    }

    close(fd);
    return 0;
}