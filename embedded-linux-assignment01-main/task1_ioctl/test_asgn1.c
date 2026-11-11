#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "asgn1_ioctl.h"

int main(void)
{
    int fd;
    char version[ASGN1_VERSION_SIZE] = {0};

    fd = open("/dev/asgn1", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (ioctl(fd, ASGN1_GET_VERSION, version) < 0) {
        perror("ioctl GET_VERSION");
        close(fd);
        return 1;
    }

    printf("Driver version: %s\n", version);
struct asgn1_stats stats;

if (ioctl(fd, ASGN1_GET_STATS, &stats) < 0) {
    perror("ioctl GET_STATS");
    close(fd);
    return 1;
}

printf("Open count: %d\n", stats.open_count);
printf("Write count: %d\n", stats.write_count);
printf("Read count: %d\n", stats.read_count);
printf("Buffer length: %d\n", stats.buffer_len);
printf("Last write size: %d\n", stats.last_write_size);
int mode = 0;

if (ioctl(fd, ASGN1_SET_MODE, &mode) < 0) {
    perror("ioctl SET_MODE");
    close(fd);
    return 1;
}

printf("SET_MODE = %d successful\n", mode);
const char *message = "Hello ASM1";
char buffer[128] = {0};
ssize_t n;

if (write(fd, message, 10) != 10) {
    perror("write");
    close(fd);
    return 1;
}

lseek(fd, 0, SEEK_SET);

n = read(fd, buffer, sizeof(buffer) - 1);
if (n < 0) {
    perror("read");
    close(fd);
    return 1;
}

buffer[n] = '\0';
printf("Read after SET_MODE 0: %s\n", buffer);
mode = 1;

if (ioctl(fd, ASGN1_SET_MODE, &mode) < 0) {
    perror("ioctl SET_MODE 1");
    close(fd);
    return 1;
}

printf("SET_MODE = %d successful\n", mode);

if (write(fd, message, 10) != 10) {
    perror("write mode 1");
    close(fd);
    return 1;
}

close(fd);

fd = open("/dev/asgn1", O_RDWR);
if (fd < 0) {
    perror("reopen mode 1");
    return 1;
}

n = read(fd, buffer, sizeof(buffer) - 1);
if (n < 0) {
    perror("read mode 1");
    close(fd);
    return 1;
}

buffer[n] = '\0';
printf("Read after SET_MODE 1: %s\n", buffer);
if (ioctl(fd, ASGN1_RESET_BUFFER) < 0) {
    perror("ioctl RESET_BUFFER");
    close(fd);
    return 1;
}

printf("RESET_BUFFER successful\n");

if (ioctl(fd, ASGN1_GET_STATS, &stats) < 0) {
    perror("ioctl GET_STATS after reset");
    close(fd);
    return 1;
}

printf("Stats after RESET_BUFFER:\n");
printf("Open count: %d\n", stats.open_count);
printf("Write count: %d\n", stats.write_count);
printf("Read count: %d\n", stats.read_count);
printf("Buffer length: %d\n", stats.buffer_len);
printf("Last write size: %d\n", stats.last_write_size);
errno = 0;

if (ioctl(fd, _IO('Z', 99)) == -1 && errno == ENOTTY) {
    printf("Invalid ioctl: ENOTTY returned correctly\n");
} else {
    printf("Invalid ioctl: FAILED (errno=%d)\n", errno);
    close(fd);
    return 1;
}

    close(fd);
    return 0;
}
