// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {

    if (strcmp(role, "tx") == 0) {
        LinkLayer connectionParameters =
        {
            .role = LlTx,
            .baudRate = baudRate,
            .nRetransmissions = nTries,
            .timeout = timeout
        };
        strcpy(connectionParameters.serialPort,serialPort);


        int open = llopen(connectionParameters);
        if(open<0) {
            printf("Error in llopen\n");
            return -1;
        }
        /*
        unsigned char start_packet[1000];
        start_packet[0] = 0x02;
        start_packet[1] = 0x00;
        struct stat st;
        if (stat(filename, &st) != 0) {
            printf("Couldn't get file size\n");
            return -1;
        }
        if (st.st_size < 256 ) {
            start_packet[2] = 0x01;
        } else if (st.st_size < 65536) {
            start_packet[2] = 0x02;
        }
        else {
            start_packet[2] = 0x03;
        }
        start_packet[3] = 0x2A;
        start_packet[4] = 0xD8;
        */
        
        unsigned char content[256 + 1];
        FILE *file = fopen(filename, "r");
        if(file == NULL) {
            printf("Error opening file\n");
            return -1;
        }
        unsigned char count = 0x00;
        while(fgets(content,256,file) != NULL) {
            content[256] = '\0';
            printf("%s\n", content);
            unsigned char data_packet[MAX_PAYLOAD_SIZE];
            data_packet[0] = 0x01;
            data_packet[1] = count;
            count++;
            data_packet[2] = 0x00;
            data_packet[3] = strlen(content);
            int i = 0;
            while (i < strlen(content)) {
                data_packet[4 + i] = content[i];
                i++;
            }
            data_packet[i] = '\0';
            llwrite(data_packet,4+i+1);
        }
        

        
        if (llclose(open)<0){
            printf("Error in llclose\n");
            return -1;
        }
        
    }
    else if(strcmp(role, "rx") == 0) {
        LinkLayer connectionParameters =
        {
            .role = LlRx,
            .baudRate = baudRate,
            .nRetransmissions = nTries,
            .timeout = timeout
        };
        strcpy(connectionParameters.serialPort,serialPort);


        int open = llopen(connectionParameters);
        if(open<0) {
            printf("Error in llopen\n");
            return -1;
        }



        if (llclose(open)<0){
            printf("Error in llclose\n");
            return -1;
        }

    }
    else {
        printf("Not valid role");
        return -1;
    }
    
    

    
}
