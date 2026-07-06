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
#include<stdint.h>
#include<pthread.h>
#include<sys/wait.h>

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
void bad_request();
void cannot_execute();
void unimplemented();

void accept_request(void *arg)
{
    int client = (intptr_t)arg;
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

    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        unimplemented(client);
        return;
    }
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
        not_found(client);
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
            execute_cgi(client, &method, query_string, &path);
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

void not_found(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
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
        not_found(client);
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

void execute_cgi(int client, const char *method, const char *query_string,
                 const char *path)
{
    int n = 1;
    int data_length;
    pid_t pid;
    char buf[255];
    char c;
    int status;
    int cgi_input[2];
    int cgi_output[2];

    if(!strcasecmp(method, "GET"))
    {
        while((n > 0) && strcmp("\n", buf))
        {
            n = get_line(client, buf, sizeof(buf));
        }
    }
    else
    {
        n = get_line(client, buf, sizeof(buf));
        buf[15] = '\0';

        while((n > 0) && strcmp("\n", buf))
        {
            if(strcmp("Content-Length:", buf))
            {
                data_length = atoi(&buf[16]);
            }
            n = get_line(client, buf, sizeof(buf));
        }
        if(data_length == -1)
        {
            bad_request(client);
            return;
        }
    }

    if(pipe(cgi_output))
    {
        cannot_execute(client);
        return;
    }
    if(pipe(cgi_input))
    {
        close(cgi_output[0]);
        close(cgi_output[1]);
        cannot_execute(client);
        return;
    }
    if((pid = fork()) < 0)
    {
        close(cgi_output[1]);
        close(cgi_input[1]);
        close(cgi_output[0]);
        close(cgi_input[0]);
        cannot_execute(client);
        return;
    }
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);

    if(pid == 0)
    {
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        dup2(cgi_input[0], 0);
        dup2(cgi_output[1], 1);
        close(cgi_input[1]);
        close(cgi_output[0]);

        sprintf(meth_env, "REQUEST_METHOD = %s", method);
        putenv(meth_env);
        if(!strcasecmp(method, "GET"))
        {
            sprintf(query_env, "QUERY_STRING = %s", query_string);
            putenv(query_env);
        }
        else
        {
            sprintf(length_env, "CONTENT_LENGTH = %d", data_length);
            putenv(length_env);
        }
        execl(path,path,NULL);
        exit(0);
    }
    else
    {
        close(cgi_input[0]);
        close(cgi_output[1]);
        if(!strcasecmp(method, "POST"))
        {
            for(int i = 0; i < data_length; i++)
            {
                recv(client, &c, 1, 0);
                write(cgi_input[1], &c, 1);
            }
        }
        while(read(cgi_output[0], &c, 1) > 0)
        {
            send(client, &c, 1, 0);
        }
        close(cgi_input[1]);
        close(cgi_output[0]);

        waitpid(pid, &status, 0);
    }
}

void bad_request(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

void cannot_execute(int client)
{
    char buf[255];
    sprintf(buf, "HTTP/1.0 500 Inter Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

void unimplemented(int client)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</TITLE></HEAD>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

int main()
{
    int server_sock = -1;
    int client_sock = -1;
    short port = 4000;
    struct sockaddr_in client_name;
    socklen_t client_name_len = sizeof(client_name);
    pthread_t newthread;

    server_sock = startup(&port);
    printf("httpd port: %d\n",port);

    while(1)
    {
        client_sock = accept(server_sock, (struct sockaddr *)&client_name,&client_name_len);
        if(client_sock == -1)
        {
            perror("accept 111");
            close(server_sock);
            exit(1);
        }
        if(pthread_create(&newthread, NULL, (void *)accept_request, (void *)(intptr_t)client_sock) != 0)
        {
            perror("pthread_create 111");
        }
    }
    close(server_sock);
    return 0;
}