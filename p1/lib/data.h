#ifndef DATA_H_
#define DATA_H_


typedef enum{
    TRANSMITTER,
    RECEIVER
}port_type;

int llopen(const char *serialPort, port_type type);
int llclose(int fd);


#endif