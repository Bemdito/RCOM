#include "lib/data.h"
#include <termios.h>

int main() {
    // open serial ports
    int fdTx = llopen("/dev/ttyS10", TRANSMITTER);
    if (fdTx < 0){
        perror("Opening Transmitter emulator serial port");
        return -1;
    }


    // close serial ports
    llclose(fdTx);

    return 0;
}