// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <stdio.h>

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

        char content[100];
        FILE *file = fopen("penguin.gif", "r");
        if(file == NULL) {
            printf("Error opening file\n");
            return -1;
        }
        while(fgets(content,100,file) != NULL) {
            printf("%s\n", content);
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
