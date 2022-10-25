// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

int t_id;
int r_id;

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


        t_id = llopen(connectionParameters);
        if(t_id<0) {
            printf("Error in llopen\n");
            return -1;
        }

        /* CONTROL PACKET TODO LATER
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
        unsigned char content[1000];
        FILE *file = fopen(filename, "r");

        if(file == NULL) {
            printf("Error opening file\n");
            return -1;
        }
        unsigned char i = 0x00;
        int x;
        x = fread(content+4,1,256,file);
        while(x > 0) {
            content[0] = 0x01;
            content[1] = i;
            if (x == 256) {
                content[2] = 0x01;
                content[3] = 0x00;
            }
            else {
                content[2] = 0x00;
                content[3] = x;
            }
            if(llwrite(content,x+4) < 0) {
                printf("Error in llwrite()\n");
                return -1;
            }
            i++;
            x = fread(content+4,1,256,file);
        }
        
        fclose(file);

        if (llclose(t_id)<0){
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


        r_id = llopen(connectionParameters);
        if(r_id<0) {
            printf("Error in llopen\n");
            return -1;
        }


        FILE* file;
        file = fopen(filename,"w");
        char string[1000];

        while (llread(string) != 0){
            int length = strlen(string);

            fwrite(string,256,1,file);
        }
        
        fclose(file);

        if (llclose(r_id)<0){
            printf("Error in llclose\n");
            return -1;
        }

    }
    else {
        printf("Not valid role");
        return -1;
    }
    
    

    
}
