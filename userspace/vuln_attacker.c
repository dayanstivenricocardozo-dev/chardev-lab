#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define DEVICE_PATH "/dev/vuln_device"
#define MSG_TEST 'A'
#define MSG_SIZE 80

int main(void){

    char buffer[MSG_SIZE];
    memset(buffer, MSG_TEST, MSG_SIZE);

    int fd;
    fd = open(DEVICE_PATH, O_WRONLY);
    if (fd == -1){
        perror("open");
        return 1;
    }

    ssize_t bytes_written;
    bytes_written = write(fd, buffer, MSG_SIZE);
    if (bytes_written == -1){
        perror("write");
        return 1;
    }

    printf("%zd\n", bytes_written);

    int result;
    result = close(fd);
    if (result == -1){
        perror("close");
        return 1;
    }

    return 0;
}
