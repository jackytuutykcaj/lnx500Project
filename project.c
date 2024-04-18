#include <linux/i2c-dev.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define I2C_BUS 5
#define ADDRESS 0x20

/* Register addresses within the MCP23017 */
#define GPB0TO7 0x01
#define GPB 0x15  
#define GPA 0x12

#define MAX_SEQUENCE_LENGTH 10

int open_i2c_bus(int bus_number) {
    char filename[20];
    snprintf(filename, sizeof(filename), "/dev/i2c-%d", bus_number);
    int fd = open(filename, O_RDWR);
    if (fd < 0) {
        perror("Failed to open I2C device");
    }
    return fd;
}


int set_slave_address(int fd, int address) {
    if (ioctl(fd, I2C_SLAVE, address) < 0) {
        perror("Failed to set slave address");
        return -1;
    }
    return 0;
}

int write_register(int fd, char register_addr, char data) {
    // Writes to the register to turn on LED
    char buffer[2] = { register_addr, data };
    if (write(fd, buffer, 2) != 2) {
        perror("Failed to write to register");
        return -1;
    }
    return 0;
}

int read_register(int fd, char register_addr) {
    // Send the register address to start reading from
    if (write(fd, &register_addr, 1) != 1) { 
        perror("Failed to write register address");
        return -1;
    }

    char data; 
    if (read(fd, &data, 1) != 1) {
        perror("Failed to read from register");
        return -1;
    }

    return data; 
}

int randomNumber(){//Random number generator. Number represents which LED will light up.
    int lower = 1;
    int upper = 4;
    static bool seed = false;

    if(!seed){
        srand(time(NULL));
        seed = true;
    }

    return (rand() % (upper - lower + 1)) + lower;
}

void add_to_sequence(int *sequence, int *sequence_length){//adds a random number between 1 - 4 to the sequence array
    if (*sequence_length >= MAX_SEQUENCE_LENGTH) {
        printf("Sequence is full!\n");
        return; 
    }

    sequence[*sequence_length] = randomNumber() - 1; // Store 0-3 to represent LEDs
    //sequence[*sequence_length] = 0;
    (*sequence_length)++;
}

int play_sequence(int fd, int *sequence, int sequence_length){//Plays LEDs in the order of sequence
    for (int index = 0; index < sequence_length; index++){
        char data = 0x00;
        if(sequence[index] == 0){
            data = 0x01;
        }
        else if(sequence[index] == 1){
            data = 0x02;
        }
        else if(sequence[index] == 2){
            data = 0x04;
        }
        else {
            data = 0x08;
        }
        if (write_register(fd, GPB, data) < 0) {
            close(fd);
            return 1; 
        }
        printf("Writing data to GPB: 0x%02X\n", data);
        sleep(1);
        if (write_register(fd, GPB, 0x00) < 0) {
            close(fd);
            return 1; 
        }
        sleep(1.5);
    }
    return 0;
}

int main() {
    int led_sequence[MAX_SEQUENCE_LENGTH];
    int sequence_length = 0;
    int fd = open_i2c_bus(I2C_BUS);
    if (fd < 0) {
        return 1; // Exit on failure 
    }

    if (set_slave_address(fd, ADDRESS) < 0) {
         close(fd);
         return 1; 
    }

    // Configure port as output 
    if (write_register(fd, GPB0TO7, 0x00) < 0) { 
        close(fd);
        return 1; 
    }

    

    //int gpa_value = read_register(fd, GPA);

    //printf("Value of GPA register :0x%02X\n", gpa_value);

    for (int i = 0; i < MAX_SEQUENCE_LENGTH; i++){
        add_to_sequence(&led_sequence, &sequence_length);
        sleep(1);
        if (play_sequence(fd, led_sequence, sequence_length) != 0){
            printf("Error playing sequnce");
            break;
        }
    }

    close(fd);
    return 0;
}
