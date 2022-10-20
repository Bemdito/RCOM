#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include "data.h"

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source
#define FALSE 0
#define TRUE 1

#define BUF_SIZE 2048

struct termios oldtioTx;
struct termios newtioTx;
int Tfd;
struct termios oldtioRx;
struct termios newtioRx;
int Rfd;

volatile int STOP = FALSE;

int llopen(const char *serialPort, port_type type) {
    
    struct termios *oldtio;
    
    struct termios *newtio;
    
    if (type == TRANSMITTER){

        oldtio = &oldtioTx;
        newtio = &newtioTx;
    }
    else {
        
        oldtio = &oldtioRx;
        
        newtio = &newtioRx;
    }
   
    int fd = open(serialPort, O_RDWR | O_NOCTTY);
    // Save current port settings
    if (tcgetattr(fd, oldtio) == -1)
        return -1;
    memset(newtio, 0, sizeof(*newtio));
    newtio->c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio->c_iflag = IGNPAR;
    newtio->c_oflag = 0;
    newtio->c_lflag = 0;
    newtio->c_cc[VTIME] = 1; // Inter-character timer unused
    newtio->c_cc[VMIN] = 0;  // Read without blocking
    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd, TCSANOW, newtio) == -1)
        return -1;

        if (fd < 0)
        return -1;

    unsigned char wbuf[BUF_SIZE] = {0};
    unsigned char rbuf[BUF_SIZE + 1];
    
    if (type == TRANSMITTER){
        Tfd = fd;
        wbuf[0] = 0x7E;
        wbuf[1] = 0x03;
        wbuf[2] = 0x03;
        wbuf[3] = wbuf[1] ^ wbuf[2];
        wbuf[4] = 0x7E;
        wbuf[5] = '\n';

    }
    else {
        Rfd = fd;
        while (STOP == FALSE){
        
            if (read(fd, rbuf, 1) > 0);
            
            printf("BUF: %s\n", rbuf);
            
        }


    }


    int lenght = strlen(wbuf) + 1;
    int bytes = write(fd, wbuf, lenght);


    return fd;
}

int llclose(int fd) {
    struct termios *oldtio;
    if (fd == Tfd) {
        oldtio = &oldtioTx;
    }
    else if (fd == Rfd) {
        oldtio = &oldtioRx;
    }
    else perror("Wrong port identifier");

    if (tcsetattr(fd, TCSANOW, oldtio) == -1){
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);
}