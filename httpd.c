#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>
#include<netinet/in.h>
#include<ctype.h>
#include<sys/stat.h>
#include<sys/types.h>

#define ISspace(x) isspace((int)(x))

void accept_request(int client)
{
    int n = 0;
    char buf[254];
    char url[254];
    char method[254];
    char path[254];
    int cgi = 0;
    int i = 0;
    int j = 0;
    struct stat st;
    char *query_string = NULL;

    n = get_line(client, buf, sizeof(buf));
    while(!ISspace(buf[i]) && i < sizeof(method)-1)
    {
        method[i] = buf[i];
        i++;
    }
    method[i] = '\0';

    if(!strcasecmp(method, 'POST'))
    {
        cgi = 1;
    }

    j = i;
    i = 0;

    while(ISspace(buf[j]) && (j < n))
    {
        j++;
    }
    while(!ISspace(buf[j]) && (j < n))
    {
        url[i] = buf[j];
        i++;
        j++;
    }
    url[i] = '\0';

    if(!strcasecmp(method, 'GET'))
    {
        query_string = url;
        while((*query_string != '?') && (*query_string != '\0'))
        {
            query_string++;
        }
        if(*query_string == '?')
        {
            cgi = 1;
            *query_string = '\0';
            query_string++;
        }
    }
    sprintf(path, "htdocs%s", url);
    if(path[strlen(path)-1] == "/")
    {
        strcat(path, '/index.html');
    }

    if(stat(path, &st) == -1)
    {
        not_found();
    }
    else
    {
        if((st.st_mode & S_IFMT) == S_IFDIR)
        {
            strcat(path, "/index.html");
        }
        if((st.st_mode & S_IXUSR)
         || (st.st_mode & S_IXGRP)
         || (st.st_mode & S_IXOTH))
        {
            cgi = 1;
        }
        if(!cgi)
        {
            serve_file();
        }
        else
        {
            execute_cgi();
        }
    }
}

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

int get_line(int client, char *buf, int size)
{
    char c = '\0';
    int i = 0;
    int n = 1;
    while((i < (size-1)) && (c != '\n'))
    {
        n = recv(client, &c, 1, 0);
        if(n > 0)
        {
            if(c == '\r')
            {
                n = recv(client, &c, 1, MSG_PEEK);
                if( (n > 0) && (c == '\n'))
                {
                    recv(client, &c, 1, 0);
                }
                else
                {
                    c = '\n';
                }
            }
            buf[i] = c;
            i++;
        }
        else
        {
            c = '\n';
        }
    }
    buf[i] = '\0';

    return i;
}

void not_found()
{

}

void serve_file()
{

}

void execute_cgi()
{

}

int main()
{
    short n = 4000;
    int fd = startup(&n);
    char buf[1024] = "Yes I read!";

    while(1)
    {
        int connfd = accept(fd, NULL, NULL);
        if(connfd < 0)
        {
            perror("accept 111");
            continue;
        }
        printf("成功接受连接！\n");

        if(write(connfd, buf, strlen(buf)) < 0)
        {
            perror("write 111");
            exit(EXIT_FAILURE);
        }

        close(connfd);
        printf("成功断开连接！\n");
    }

    close(fd);
    return 0;
}