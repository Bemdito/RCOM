#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
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

int alarmEnabled = FALSE;
int alarmCount = 0;

volatile int STOP = FALSE;

void alarmHandler(int signal){
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Timeout #%d\n", alarmCount);
}

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
        int lenght = strlen(wbuf) + 1;
        (void)signal(SIGALRM, alarmHandler);

        int state = 0;
        unsigned char a;
        unsigned char c;

        while (alarmCount < 4 && STOP == FALSE){
            if (alarmEnabled == FALSE){
                alarm(5); // Set alarm to be triggered in 3s
                int bytes = write(fd, wbuf, lenght);
                alarmEnabled = TRUE;
            }
            if (read(fd, rbuf, 1) > 0) {
                switch (state) {
                case 0:
                    if (rbuf[0] == 0x7E) state++;
                    else state = 0;
                break;
                case 1:
                    if (rbuf[0] == 0x03) {
                        state++;
                        a = rbuf[0];
                    }
                    else state = 0;
                break;
                case 2:
                    if (rbuf[0] == 0x07) {
                        state++;
                        c = rbuf[0];
                    }
                    else state = 0;
                break;
                case 3:
                    if (rbuf[0] == a ^ c) state++;
                    else state = 0;
                break;
                case 4:
                    if (rbuf[0] == 0x7E) STOP = TRUE;
                    else state = 0;
                break;
                default:
                    state = 0;
                    break;
                }
            }
        }
        if (alarmCount >= 4){
            printf("Maximum timeouts reached!!!\n");
            return -1;
        }
        
        printf("Ready to send packets\n");
             
    }
    else {
        Rfd = fd;
        int state = 0;
        unsigned char a;
        unsigned char c;
        while (STOP == FALSE){
            if (read(fd, rbuf, 1) > 0) {
                switch (state) {
                case 0:
                    if (rbuf[0] == 0x7E) state++;
                    else state = 0;
                break;
                case 1:
                    if (rbuf[0] == 0x03) {
                        state++;
                        a = rbuf[0];
                    }
                    else state = 0;
                break;
                case 2:
                    if (rbuf[0] == 0x03) {
                        state++;
                        c = rbuf[0];
                    }
                    else state = 0;
                break;
                case 3:
                    if (rbuf[0] == a ^ c) state++;
                    else state = 0;
                break;
                case 4:
                    if (rbuf[0] == 0x7E) STOP = TRUE;
                    else state = 0;
                break;
                default:
                    state = 0;
                    break;
                }
            }
        }
        printf("Receive SET\n");
        wbuf[0] = 0x7E;
        wbuf[1] = 0x03;
        wbuf[2] = 0x07;
        wbuf[3] = wbuf[1] ^ wbuf[2];
        wbuf[4] = 0x7E;
        wbuf[5] = '\n';
        int lenght = strlen(wbuf) + 1;
        int bytes = write(fd, wbuf, lenght);



    }


    

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