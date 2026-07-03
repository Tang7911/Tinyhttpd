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
#define SERVER_STRING "Server: jdbhttpd/0.1.0\r\n"

void accept_request();
int startup();
int get_line();
void not_found();
void serve_file();
void headers();
void cat();
void execute_cgi();

void accept_request(int client)
{
    int n = 0;
    char buf[1024];
    char url[254];
    char method[255];
    char path[512];
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

    if(!strcasecmp(method, "POST"))
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

    if(!strcasecmp(method, "GET"))
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
    if(path[strlen(path)-1] == '/')
    {
        strcat(path, "/index.html");
    }

    if(stat(path, &st) == -1)
    {
        while((n > 0) && strcmp("\n", buf))
        {
            n = get_line(client, buf, sizeof(buf));
        }
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
            serve_file(client, path);
        }
        else
        {
            execute_cgi();
        }
    }
    close(client);
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
    exit(1);
}

void serve_file(int client, char *path)
{
    FILE *Document = NULL;
    int n = 1;
    char buf[1024];
    buf[0] = 'A';
    buf[1] = '\0';
    while((n > 0) && strcmp("\n", buf))
    {
        n = get_line(client, buf, sizeof(buf));
    }
    Document = fopen(path, "r");
    if(Document == NULL)
    {
        not_found();
    }
    else
    {
        headers(client);
        cat(client,Document);
    }
}

void headers(int client)
{
    char buf[1024];
    strcpy(buf, "HTTP/1.1 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

void cat(int client, FILE *Document)
{
    char buf[1024];
    fgets(buf, sizeof(buf), Document);
    while(feof(Document) == 0)
    {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), Document);
    }
    send(client, buf, strlen(buf), 0);
}

void execute_cgi()
{
    exit(1);
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

        accept_request(connfd);

        close(connfd);
        printf("成功断开连接！\n");
    }

    close(fd);
    return 0;
}