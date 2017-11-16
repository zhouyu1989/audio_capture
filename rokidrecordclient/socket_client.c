#include <errno.h>
#include <stdio.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PATH "/data/com.rokid.record.localsocket"
#define BUFFER_LEN 512

int main(int argc, char *argv[]) 
{
    int i = 0;
    int len = 0;
    int ret = 0;
    int sockfd;
    char buffer[BUFFER_LEN];
    struct sockaddr_un address;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return sockfd;
    }

    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, PATH);
    len = sizeof(address);

    ret = connect(sockfd, (struct sockaddr *) &address, len);

    if(ret == -1) {
        perror("oops: connet failed\n");
        close(sockfd);
        goto end;
    }

    //Send Client Command to Server.
    for(i = 1; i < argc; i++) {
        memset(buffer, 0, BUFFER_LEN);
        strcpy(buffer, argv[i]);
        ret = write(sockfd, buffer, strlen(buffer));
        if(ret < 0) {
            printf("send command failed\n");
            return ret;
        }

        //printf("send command: %s successful\n", buffer);
        len = recv(sockfd, buffer, BUFFER_LEN, MSG_NOSIGNAL);

        //printf("recv data: %s, len = %d, errno = %d\n", buffer, len, errno);
        if(strncmp(buffer, "ack", 3) != 0) {
            break;
        }
    }

    ret = close(sockfd);

end:
    return ret;
}
