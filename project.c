#include <linux/i2c-dev.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define I2C_BUS 5
#define ADDRESS 0x20

/* Register addresses within the MCP23017 */
#define GPB0TO7 0x01
#define GPB 0x15  
#define GPA 0x12

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

int main() {
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

    // Set data for the GPIO register
    if (write_register(fd, GPB, 0x00) < 0) {
        close(fd);
        return 1; 
    }

    int gpa_value = read_register(fd, GPA);

    printf("Value of GPA register:0x%02X\n", gpa_value);

    close(fd);
    return 0;
}
