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
int t_id2;
struct termios oldtioRx;
struct termios newtioRx;
int Rfd;

int alarmEnabled = FALSE;
int alarmCount;

volatile int STOP = FALSE;
volatile int rSTOP = FALSE;

volatile int write_number = 0;
volatile int receiver_number = 0;

unsigned char write_cnt = 0x00;
unsigned char read_cnt = 0x00;


void alarmHandler(int signal){
    alarmEnabled = FALSE;
    alarmCount++;
    printf("Timeout #%d\n", alarmCount);
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters) {
    alarmCount = 0;
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
        t_id2 = fd;
        wbuf[0] = 0x7E;
        wbuf[1] = 0x03;
        wbuf[2] = 0x03;
        wbuf[3] = wbuf[1] ^ wbuf[2];
        wbuf[4] = 0x7E;
        wbuf[5] = '\n';
        int length = strlen(wbuf);
        (void)signal(SIGALRM, alarmHandler);

        int state = 0;
        unsigned char a;
        unsigned char c;

        while (alarmCount < connectionParameters.nRetransmissions + 1 && STOP == FALSE){
            if (alarmEnabled == FALSE){
                alarm(connectionParameters.timeout); 
                int bytes = write(fd, wbuf, 5);
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
                    if (rbuf[0] == 0x7E) {
                        STOP = TRUE;
                        state++;
                    }
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
        if (state != 5) {
            return -1;
        }
        printf("Ready to send!\n");
        
         
             
    }
    else {
        Rfd = fd;
        int state = 0;
        unsigned char a;
        unsigned char c;
        while (rSTOP == FALSE){
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
                    if (rbuf[0] == 0x7E) rSTOP = TRUE;
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
    STOP = FALSE;
    alarmEnabled = FALSE;
    unsigned char bcc2 = 0x00;
    unsigned char buffer[1000];
    buffer[0] = 0x7E;
    buffer[1] = 0x03;
    if(write_number == 0) buffer[2] = 0x00;
    else buffer[2] = 0x40;
    buffer[3] = buffer[1] ^ buffer[2];
    
    int i = 0;
    while(i < bufSize) {
        buffer[i+4] = buf[i];
        bcc2 ^= buffer[i+4];
        i++;
    }
    buffer[i+4] = bcc2;
    buffer[i+5] = 0x7E;
    int bytes;
    unsigned char rbuffer[12];
    (void)signal(SIGALRM, alarmHandler);
    alarmCount = 0;
    int Receiver_Ready = FALSE;
    int state = 1;
    while(alarmCount < 4 && STOP == FALSE) {
        if(alarmEnabled == FALSE) {
            alarm(3);
            alarmEnabled = TRUE;
            if(write(t_id2, buffer, bufSize+6 ) != bufSize+6) {
                printf("Missed some bytes\n");
            }
            //else alarmEnabled = FALSE; // remove to give time
        }
        if(read(t_id2,rbuffer,1)>0) {
            unsigned char a2;
            unsigned char c2;
            switch (state) {
                case 1:
                    if (rbuffer[0] == 0x7E) {
                        state++;
                    }
                    else state = 1;
                break;
                case 2:
                    if (rbuffer[0] == 0x03) {
                        state++;
                        a2 = rbuffer[0];
                    }
                    else state = 1;
                break;
                case 3:
                    if (write_number == 0 && rbuffer[0] == 0x05) {
                        state++;
                        c2 = rbuffer[0];
                    }
                    else if(write_number == 0 && rbuffer[0] == 0x01) {
                        state=1;
                        c2 = rbuffer[0];
                    }
                    else if (write_number == 1 && rbuffer[0] == 0x85) {
                        state++;
                        c2 = rbuffer[0];
                    }
                    else if (write_number == 1 && rbuffer[0] == 0x81) {
                        state = 1;
                        c2 = rbuffer[0];
                    }
                    else state = 1;
                break;
                case 4:
                    if (rbuffer[0] == a2 ^ c2) {
                        state++;
                    }
                    else state = 1;
                break;
                case 5:
                    if (rbuffer[0] == 0x7E) {
                        STOP = TRUE;
                        Receiver_Ready = TRUE;
                        if (write_number == 0) write_number = 1;
                        else write_number=0;
                    }
                break;
                default:
                    state = 1;
                    break;
            } 
        }
        
        
        
    }
    if (Receiver_Ready){
        printf("Bytes written: %d\n", 6+bufSize);
        return 1+i;
    }

    printf("Maximum timeouts reached!!!\n");
    return -1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet){
    unsigned char read_packet[1000];
    unsigned char rbuffer[5];

    int info = FALSE;
    int disc = FALSE;

    rSTOP = FALSE;
    int length;
    unsigned char a;
    unsigned char c;
    unsigned char bcc2 = 0x00;
    
    int read_bytes;
    int content_size;
    int state = 1;
    unsigned char l1, l2;
    unsigned char content[256];
    while (rSTOP == FALSE){
        read_bytes = read(Rfd, read_packet, 1);
        if (read_bytes > 0) {
            switch (state) {
            case 1 :
                if (read_packet[0] == 0x7E){
                    state++;    
                }
                else state = 1;        
                break;
            case 2 :
                if (read_packet[0] == 0x03){

                    a = read_packet[0];
                    state++;
                }
                else state = 1;
                break;
            case 3 :
                c = read_packet[0];
                if (read_packet[0] == 0x0B) {
                    state++;
                    disc = TRUE;
                }
                else if (receiver_number == 0 && c == 0x00) {
                    state++;
                    info=TRUE;
                }
                else if (receiver_number == 1 && c == 0x40) {
                    state++;
                    info=TRUE;
                }
                else state = 1;
                break;
            case 4 :
                if (read_packet[0] == a ^ c) {
                    state++;
                }
                else state = 1;
                break;
            case 5 :
                if (info == FALSE && read_packet[0] == 0x7E) {
                    state = 0;
                    rSTOP = TRUE;
                }
                else if (info == TRUE) {
                    if (read_packet[0] == 0x01){
                        bcc2 ^= read_packet[0];
                        state++;
                    }
                }
                else state = 1;
                break;
            case 6 :
                if (read_packet[0] == read_cnt) {
                    bcc2 ^= read_packet[0];
                    state++;
                }
                else state = 1;
                break;
            case 7 :
                l2 = read_packet[0];
                bcc2 ^= l2;
                while (read(Rfd, read_packet, 1) == 0);
                l1 = read_packet[0];
                bcc2 ^= l1;
                content_size = 256 * l2 + l1;
                int i = 0;
                while (i < content_size) {
                    i += read(Rfd, content + i,content_size-i);
                }
                int j = 0;
                while (j<content_size) {
                    bcc2 ^= content[j];
                    j++;
                }
                state++;
                break;
            case 8 :
                if (read_packet[0] == bcc2) {
                    state++;
                }
                else
                    state = 1;
                break;
            case 9 :
                if (read_packet[0] == 0x7E) {
                    read_cnt++;
                    rSTOP = TRUE;
                }
                break;
            default:
                rSTOP = TRUE;
                break;
            }
        }
        
        
        
    }

    
    if (info == TRUE) {
        memcpy(packet, content, content_size);
        unsigned char receiver_ready[10];
        receiver_ready[0] = 0x7E;
        receiver_ready[1] = 0x03;
        if (receiver_number == 0) {
            receiver_ready[2] = 0x05;
            receiver_number = 1;
        }
        else {
            receiver_ready[2] = 0x85;
            receiver_number = 0;
        }
        receiver_ready[3] = receiver_ready[1] ^ receiver_ready[2];
        receiver_ready[4] = 0x7E;
        write(Rfd, receiver_ready, 5);
        printf("Wrote RR\n");
    }
    if(disc == TRUE) return 0;

    

    return content_size;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics) {
    struct termios *oldtio;

    if (showStatistics == t_id2) {
        oldtio = &oldtioTx;
        unsigned char disc[10];
        disc[0] = 0x7E;
        disc[1] = 0x03;
        disc[2] = 0x0B;
        disc[3] = disc[1] ^ disc[2];
        disc[4] = 0x7E;
        disc[5] = '\0';
        write(t_id2,disc, 6);
        printf("Written disc message\n");
    }
    else if (showStatistics == Rfd) {
        oldtio = &oldtioRx;
    }
    else {
        perror("Wrong port identifier\n");
        return -1;
    }
    if (tcsetattr(showStatistics, TCSANOW, oldtio) == -1){
        perror("tcsetattr");
        exit(-1);
    }

    close(showStatistics);
    return 1;
}

