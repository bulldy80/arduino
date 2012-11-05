#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>



int setDTRAndRTS(int fd, unsigned short level)
{
     int status;

     if (ioctl(fd, TIOCMGET, &status) == -1) {
	  perror("setDTRAndRTS(): TIOCMGET");
	  return -1;
     }
     if (level) status |= (TIOCM_DTR | TIOCM_RTS);
     else status &= ~(TIOCM_DTR | TIOCM_RTS);
     if (ioctl(fd, TIOCMSET, &status) == -1) {
	  perror("setDTRAndRTS(): TIOCMSET");
	  return -1;
     }
     return 0;
}

void prepareForData(int fd)
{
    struct termios tio;
    int rc;
    cfmakeraw(&tio);
    cfsetispeed(&tio,B9600);
    cfsetospeed(&tio,B9600);
    tcsetattr(fd,TCSANOW,&tio);
    rc = fcntl(fd, F_GETFL, 0);
    if (rc != -1)
        fcntl(fd, F_SETFL, rc | O_NONBLOCK);
    else
        perror("prepareForData(fcntl)");
}

void prepareForControl(int fd)
{
    struct termios tio;
    int rc;

    rc = tcgetattr(fd, &tio);
    if (rc < 0) {
        perror("prepareForControl(tcgetattr)");
        return;
    }

    tio.c_iflag = IGNBRK;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cflag = (CS8 | CREAD | CLOCAL);
    tio.c_cc[VMIN]  = 1;
    tio.c_cc[VTIME] = 0;

    cfsetispeed(&tio,B115200);
    cfsetospeed(&tio,B115200);
    rc = tcsetattr(fd,TCSANOW,&tio);
    if (rc < 0)
        perror("prepareForControl(tcsetattr)");

    rc = fcntl(fd, F_GETFL, 0);
    if (rc != -1)
        fcntl(fd, F_SETFL, rc & ~O_NONBLOCK);
    else
        perror("prepareForControl(fcntl)");
}

void drain(int fd)
{
    struct timeval timeout;
    fd_set rfds;
    int nfds;
    int rc;
    unsigned char buf;

    while (1) {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 250000;

        nfds = select(fd + 1, &rfds, NULL, NULL, &timeout);
        if (nfds == 0) {
            break;
        }
        else if (nfds == -1) {
            if (errno != EINTR) {
                perror("drain(select)");
                return;
            }
        }
        else {
            rc = read(fd, &buf, 1);
            if (rc < 0) {
                perror("drain(read)");
            }
        }
    }
}

int stksync(int fd)
{
    unsigned char buf[2];
    const unsigned char syncCmd[] = { 0x30, 0x20 };
    write(fd, syncCmd, 2);
    drain(fd);
    write(fd, syncCmd, 2);
    drain(fd);
    write(fd, syncCmd, 2);
    if (read(fd, buf, 2) < 0) {
        perror("sync(read)");
        return -1;
    }
    if (buf[0] != 0x14 || buf[1] != 0x10) {
        printf("sync failed: %d(0x%x) %d(0x%x)\n", (int)buf[0], (int)buf[0], (int)buf[1], (int)buf[1]);
        return -1;
    }
    return 0;
}

int readParam(int fd, const unsigned char* cmd, int* result, const char* msg)
{
    unsigned char buf[3];
    write(fd, cmd, 3);
    if (read(fd, buf, 3) < 0) {
        perror("msg");
        return -1;
    }
    if (buf[0] != 0x14 || buf[2] != 0x10) {
        printf("%s: out of sync: %d(0x%x) %d(0x%x)\n", msg, (int)buf[0], (int)buf[0], (int)buf[2], (int)buf[2]);
        return -1;
    }
    *result = buf[1];
    return 0;
}

int stkversion(int fd, int* hardware, int* major, int* minor)
{
    const unsigned char hardwareCmd[] = { 0x41, 0x80, 0x20 };
    const unsigned char majorCmd[] = { 0x41, 0x81, 0x20 };
    const unsigned char minorCmd[] = { 0x41, 0x82, 0x20 };
    if (0 != readParam(fd, hardwareCmd, hardware, "hardware version")) {
        return -1;
    }
    if (0 != readParam(fd, majorCmd, major, "major version")) {
        return -1;
    }
    if (0 != readParam(fd, minorCmd, minor, "minor version")) {
        return -1;
    }
    return 0;
}

int stkenterprog(int fd)
{
    unsigned char buf[2];
    const unsigned char cmd[] = { 0x50, 0x20 };
    write(fd, cmd, 2);
    if (read(fd, buf, 2) < 0) {
        perror("enter(read)");
        return -1;
    }
    if (buf[0] != 0x14 || buf[1] != 0x10) {
        printf("enter is out of sync: %d(0x%x) %d(0x%x)\n", (int)buf[0], (int)buf[0], (int)buf[1], (int)buf[1]);
        return -1;
    }
    return 0;
}

int stkleaveprog(int fd)
{
    unsigned char buf[2];
    const unsigned char cmd[] = { 0x51, 0x20 };
    write(fd, cmd, 2);
    if (read(fd, buf, 2) < 0) {
        perror("leave(read)");
        return -1;
    }
    if (buf[0] != 0x14 || buf[1] != 0x10) {
        printf("leave is out of sync: %d(0x%x) %d(0x%x)\n", (int)buf[0], (int)buf[0], (int)buf[1], (int)buf[1]);
        return -1;
    }
    return 0;
}

int stkreadsign(int fd, unsigned char* sign)
{
    unsigned char buf[5];
    const unsigned char cmd[] = { 0x75, 0x20 };
    write(fd, cmd, 2);
    if (read(fd, buf, 5) < 0) {
        perror("readsign(read)");
        return -1;
    }
    if (buf[0] != 0x14 || buf[4] != 0x10) {
        printf("readsign is out of sync: %d(0x%x) %d(0x%x)\n", (int)buf[0], (int)buf[0], (int)buf[4], (int)buf[4]);
        return -1;
    }
    sign[0] = buf[1];
    sign[1] = buf[2];
    sign[2] = buf[3];
    return 0;
}

int stksignon(int fd, char* msg)
{
    unsigned char buf;
    const unsigned char cmd[] = { 0x31, 0x20 };
    write(fd, cmd, 2);
    if (read(fd, &buf, 1) < 0) {
        perror("stksignon(read)");
        *msg = 0;
        return -1;
    }
    if (buf != 0x14) {
        printf("signon is out of sync: %d(0x%x)\n", (int)buf, (int)buf);
        *msg = 0;
        return -1;
    }
    while (1) {
        if (read(fd, &buf, 1) < 0) {
            perror("stksignon(read2)");
            *msg = 0;
            return -1;
        }
        if (buf == 0x10) {
            break;
        }
        *msg = buf;
        ++msg;
    }
    *msg = 0;
    return 0;
}

int stkloadaddress(int fd, int addr)
{
    unsigned char buf[2];
    unsigned char cmd[4];
    cmd[0] = 0x55;
    cmd[1] = addr & 0xFF;
    cmd[2] = (addr >> 8) && 0xFF;
    cmd[3] = 0x20;
    write(fd, cmd, 4);
    if (read(fd, buf, 2) < 0) {
        perror("loadaddr(read)");
        return -1;
    }
    if (buf[0] != 0x14 || buf[1] != 0x10) {
        printf("loadaddr is out of sync: %d(0x%x) %d(0x%x)\n", (int)buf[0], (int)buf[0], (int)buf[1], (int)buf[1]);
        return -1;
    }
    return 0;
}

int stkreadpage(int fd, char type, int size, unsigned char* data)
{
    int rc;
    unsigned char buf;
    unsigned char cmd[5];
    cmd[0] = 0x74;
    cmd[1] = (size >> 8) && 0xFF;
    cmd[2] = size & 0xFF;
    cmd[3] = type;
    cmd[4] = 0x20;
    write(fd, cmd, 5);
    if (read(fd, &buf, 1) < 0) {
        perror("readpage(read)");
        return -1;
    }
    if (buf != 0x14) {
        printf("readpage is out of sync: %d(0x%x)\n", (int)buf, (int)buf);
        return -1;
    }
    while (size > 0) {
        rc = read(fd, data, size);
        if (rc < 0) {
            perror("readpage(read2)");
            return -1;
        }
        data += rc;
        size -= rc;
    }
    if (read(fd, &buf, 1) < 0) {
        perror("readpage(read3)");
        return -1;
    }
    if (buf != 0x10) {
        printf("readpage is out of sync 2: %d(0x%x)\n", (int)buf, (int)buf);
        return -1;
    }
    return 0;
}

int openPort(const char* port)
{
    int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK );
    printf("Got fd: %d\n", fd);
    return fd;
}

void closePort(int fd)
{
    setDTRAndRTS(fd, 0);
    close(fd);
}

void printChar(int fd, unsigned char data)
{
    data = isprint(data) ? data : '.';
    write(fd, &data, 1);
}

void printHexChar(int fd, unsigned char data)
{
    const char HEX_CHARS[] = {
        '0', '1', '2', '3',
        '4', '5', '6', '7',
        '8', '9', 'A', 'B',
        'C', 'D', 'E', 'F'
    };
    write(fd, &HEX_CHARS[(data >> 4) & 0xF], 1);
    write(fd, &HEX_CHARS[data & 0xF], 1);
}

int min(int a, int b)
{
    return a < b ? a : b;
}

void printHex(int fd, const unsigned char* data, int len)
{
    const char spaces[] = "  ";
    const char ascii_pre[] = " |";
    const char ascii_post[] = "|\n";
    int i,j,end;
    for (i = 0; i < len; i += 16) {
        end = min(i+8, len);
        for (j = i; j < end; ++j) {
            printHexChar(fd, data[j]);
        }
        for (; j < i+8; ++j) {
            write(fd, spaces, 2);
        }
        write(fd, spaces, 1);

        end = min(i+16, len);
        for (j = i+8; j < end; ++j) {
            printHexChar(fd, data[j]);
        }
        for (; j < i+16; ++j) {
            write(fd, spaces, 2);
        }

        write(fd, ascii_pre, 2);
        end = min(i+8, len);
        for (j = i; j < end; ++j) {
            printChar(fd, data[j]);
        }
        for (; j < i+8; ++j) {
            write(fd, spaces, 1);
        }
        write(fd, spaces, 1);

        end = min(i+16, len);
        for (j = i+8; j < end; ++j) {
            printChar(fd, data[j]);
        }
        for (; j < i+16; ++j) {
            write(fd, spaces, 1);
        }
        write(fd, ascii_post, 2);
    }
}

int main(int argc, char **argv) {

    FILE* flashFile;
    int stdinfd, stdoutfd;
    int fd;
    int len;
    unsigned char buf[8192+1];
    fd_set readfds;
    fd_set master;
    int fdmax;
    int i, rc;
    int hardware, major, minor;
    unsigned char sign[3];

    stdinfd = fileno(stdin);
    stdoutfd = fileno(stdout);

    printf("stdinfd = %d\n", stdinfd);
    printf("stdoutfd = %d\n", stdoutfd);

    fd = openPort(argv[1]);
    prepareForData(fd);
    drain(fd);

    //FD_SET(stdinfd, &master);
    fdmax = fd;
    FD_SET(fd, &master);

    for(;;) {
        readfds = master;
        select(fdmax+1, &readfds, 0, 0, 0);
        if (FD_ISSET(stdinfd, &readfds)) {
            len = read(stdinfd, buf, sizeof(buf));
            if (len == 0) {
                break;
            }
            if (len > 0) {
                buf[len] = 0;
                if (0 == strcmp(buf, "r\n")) {
                    printf("Reseting!!\n");
                    prepareForControl(fd);
                    setDTRAndRTS(fd, 0);
                    usleep(250*1000);
                    setDTRAndRTS(fd, 1);
                    usleep(50*1000);
                    drain(fd);
                    rc = stksync(fd);
                    printf("sync = %d\n", rc);
                    rc = stkversion(fd, &hardware, &major, &minor);
                    printf("version = %d: %d %d.%d\n", rc, hardware, major, minor);
                    rc = stkenterprog(fd);
                    printf("enter = %d\n", rc);
                    rc = stksignon(fd, buf);
                    printf("signon = %d: '%s'\n", rc, buf);
                    rc = stkreadsign(fd, sign);
                    printf("sign = %d: %X %X %X\n", rc, (int)sign[0], (int)sign[1], (int)sign[2]);
                    rc = stkloadaddress(fd, 0x0000);
                    printf("loadaddr = %d\n", rc);
                    flashFile = fopen("flash.hex", "w");
                    for (i = 0; i < 4940; i += 0x0080) {
                        int pageSize = min(0x0080, 4940-i);
                        rc = stkreadpage(fd, 'F', pageSize, buf);
                        printf("readpage = %d\n", rc);
                        printHex(fileno(flashFile), buf, pageSize);
                    }
                    fclose(flashFile);
                    rc = stkleaveprog(fd);
                    printf("leave = %d\n", rc);
                    prepareForData(fd);
                }
                else {
                    int n = write(fd, buf, len);
                    if (n == -1) {
                        printf("Got error: %d: '%s'\n", errno, strerror(errno));
                    }
                }
            }
        }
        if (FD_ISSET(fd, &readfds)) {
            len = read(fd, buf, sizeof(buf));
            if (len == 0) {
                write(stdoutfd, "EOF!!\n", 6);
                break;
            }
            if (len > 0) {
                write(stdoutfd, buf, len);
                if (!FD_ISSET(stdinfd, &master) && memchr(buf, '\n', len)) {
                    if (fdmax < stdinfd) {
                        fdmax = stdinfd;
                    }
                    FD_SET(stdinfd, &master);
                }
            }
        }
    }

    closePort(fd);
}

