#include "lib/data.h"
#include <termios.h>

int main() {
    // open serial ports
    int fdRx = llopen("/dev/ttyS11", RECEIVER);
    if (fdRx < 0){
        perror("Opening Receiver emulator serial port");
        return -1;
    }


    // close serial ports
    llclose(fdRx);

    return 0;
}