#include <errno.h>
#include <stdio.h>
#include <fcntl.h> 
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PATH "com.rokid.record.localsocket"
#define BUFFER_LEN 512

static char buffer[BUFFER_LEN];
static int socket_server_exit = 0;

void sigint_handler(int sig)
{
    socket_server_exit = 1;
}


int main(void)
{
    int len = 0;
    int server_len, client_len;
    int server_sockfd, client_sockfd;

    struct sockaddr_un server_address;
    struct sockaddr_un client_address;

    //delete the server create before.
    unlink(PATH);
    server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, PATH);
    server_len = sizeof(server_address);
    bind(server_sockfd, (struct sockaddr *)&server_address, server_len);

    listen(server_sockfd, 5);

    while(1) {
        printf("rokid record server enter listen mode\n");
        client_len = sizeof(client_address);
        client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);
	fcntl(client_sockfd, F_SETFL, O_NONBLOCK);

        while(1) {
            len = recv(client_sockfd, buffer, BUFFER_LEN, MSG_NOSIGNAL);

            //printf("len = %d, buffer = %s, errno: %d\n", len, buffer, errno);
            if((len < 0 && errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN) || len == 0) {
                printf("return value: %d, err: %d\n", len, errno);
                break;
            } else {
                if(len < 0)
                    continue;

                printf("%s\n", buffer);
            }

            if(len > 0) {
                memset(buffer, 0, sizeof(buffer));
                strncpy(buffer, "ack", 3);
                len = send(client_sockfd, buffer, BUFFER_LEN, MSG_NOSIGNAL);
                memset(buffer, 0, sizeof(buffer));
            }

            len = 0;
        }

        close(client_sockfd);
        client_sockfd = -1;

        if(socket_server_exit) {
            printf("rokid record server exit");
            break;
	}
    }

    close(server_sockfd);
    server_sockfd = -1;

    return 0;
}

