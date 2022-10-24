#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
// Link layer protocol implementation

#include "link_layer.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

struct termios oldtioTx;
struct termios newtioTx;
int Tfd;
struct termios oldtioRx;
struct termios newtioRx;
int Rfd;

int alarmEnabled = FALSE;
int alarmCount = 0;

volatile int STOP = FALSE;

volatile int S = 0;


void alarmHandler(int signal){
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Timeout #%d\n", alarmCount);
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters) {
    
    struct termios *oldtio;
    struct termios *newtio;
    
    if (connectionParameters.role == LlTx){

        oldtio = &oldtioTx;
        newtio = &newtioTx;
    }
    else {
        
        oldtio = &oldtioRx;
        newtio = &newtioRx;
    }
    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    // Save current port settings
    if (tcgetattr(fd, oldtio) == -1)
        return -1;
    memset(newtio, 0, sizeof(*newtio));
    newtio->c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
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

    unsigned char wbuf[MAX_PAYLOAD_SIZE] = {0};
    unsigned char rbuf[MAX_PAYLOAD_SIZE + 1];
    
    if (connectionParameters.role == LlTx){
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

        while (alarmCount < connectionParameters.nRetransmissions + 1 && STOP == FALSE){
            if (alarmEnabled == FALSE){
                alarm(connectionParameters.timeout); 
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
        if (alarmCount >= connectionParameters.nRetransmissions){
            printf("Maximum timeouts reached!!!\n");
            return -1;
        }
        else
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

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize) {
    unsigned char f = 0x7E;
    unsigned char a = 0x03;
    unsigned char c;
    if (S == 0) {
        c = 0x00;
    }
    else c = 0x40;
    unsigned char bcc1 = a ^ c;
    unsigned char bcc2 = 0xff; // TODO aprender bcc2
    unsigned char buffer[1024];
    buffer[0] = f;
    buffer[1] = a;
    buffer[2] = c;
    buffer[3] = bcc1;
    int i = 0;
    while(i < bufSize) {
        buffer[4 + i] = buf[i];
        i++;
    }
    buffer[4 + i] = bcc2;
    buffer[5 + i] = f;
    buffer[6 + i] = '\0';
    int bytes;
    while(alarmCount < 4 && STOP == FALSE) {
        if(alarmEnabled == FALSE) {
            alarm(5);
            bytes = write(Tfd, buffer, 7 + i);
        }
        
    }


    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics) {
    struct termios *oldtio;
    if (showStatistics == Tfd) {
        oldtio = &oldtioTx;
    }
    else if (showStatistics == Rfd) {
        oldtio = &oldtioRx;
    }
    else perror("Wrong port identifier");

    if (tcsetattr(showStatistics, TCSANOW, oldtio) == -1){
        perror("tcsetattr");
        exit(-1);
    }

    close(showStatistics);
    return 1;
}
