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
#define GPA0TO7 0x00
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
  char buffer[2] = {
    register_addr,
    data
  };
  if (write(fd, buffer, 2) != 2) {
    perror("Failed to write to register");
    return -1;
  }
  return 0;
}

int read_register(int fd, char register_addr) {
  // Send the register address to start reading from
  if (write(fd, & register_addr, 1) != 1) {
    perror("Failed to write register address");
    return -1;
  }

  char data;
  if (read(fd, & data, 1) != 1) {
    perror("Failed to read from register");
    return -1;
  }

  return data;
}

int randomNumber() { //Random number generator. Number represents which LED will light up.
  int lower = 1;
  int upper = 4;
  static bool seed = false;

  if (!seed) {
    srand(time(NULL));
    seed = true;
  }

  return (rand() % (upper - lower + 1)) + lower;
}

void add_to_sequence(int * sequence, int * sequence_length) { //adds a random number between 1 - 4 to the sequence array
  if ( * sequence_length >= MAX_SEQUENCE_LENGTH) {
    printf("Sequence is full!\n");
    return;
  }

  sequence[ * sequence_length] = randomNumber() - 1; // Store 0-3 to represent LEDs
  //sequence[*sequence_length] = 0;
  ( * sequence_length) ++;
}

int play_sequence(int fd, int * sequence, int sequence_length) { //Plays LEDs in the order of sequence
  for (int index = 0; index < sequence_length; index++) {
    char data = 0x00;
    if (sequence[index] == 0) {
      data = 0x01;
    } else if (sequence[index] == 1) {
      data = 0x02;
    } else if (sequence[index] == 2) {
      data = 0x04;
    } else {
      data = 0x08;
    }
    if (write_register(fd, GPB, data) < 0) {
      close(fd);
      return 1;
    }
    //printf("Writing data to GPB: 0x%02X\n", data);
    sleep(1);
    if (write_register(fd, GPB, 0x00) < 0) {
      close(fd);
      return 1;
    }
    sleep(1.5);
  }
  return 0;
}

bool wait_for_button_press(int fd, int expected_button) {
  while (true) {
    int gpa_value = read_register(fd, GPA);

    // Check the expected button
    int button_state = (gpa_value >> expected_button) & 0x01;
    if (button_state == 0) { // Expected button pressed
      // Only the expected button is pressed 
      return true;
    }
    // Ensure no other buttons are pressed
    for (int i = 0; i < 4; i++) {
      if (i != expected_button) {
        if (i != expected_button && ((gpa_value >> i) & 0x01) == 0) {
          // Another button is pressed along with the expected one
          return false;
        }
      }

    }

    usleep(10000); // 10 milliseconds delay
  }
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

  // Configure GPIOB pins 0 to 7 as output 
  if (write_register(fd, GPB0TO7, 0x00) < 0) {
    close(fd);
    return 1;
  }

  // Configure GPIOA pins 0 to 7 as input 
  if (write_register(fd, GPA0TO7, 0xFF) < 0) {
    close(fd);
    return 1;
  }

  int round = 1;
  bool game_over = false;

  while (!game_over) {
    printf("Round %d\n", round);
    round++;
    // 1. Add to sequence
    printf("adding to sequence\n");
    add_to_sequence( & led_sequence, & sequence_length);

    // 2. Play sequence
    printf("Playing sequence\n");
    if (play_sequence(fd, led_sequence, sequence_length) != 0) {
      printf("Error playing sequence\n");
      game_over = true; // Exit loop on error
      break;
    }

    printf("Your turn!\n");
    for (int i = 0; i < sequence_length; i++) {
      bool button_pressed = false;
      if (!wait_for_button_press(fd, led_sequence[i])) {
        printf("Wrong button! Game over.\n");
        game_over = true;
        break;
      } else {
        button_pressed = true;
      }
      while (button_pressed) {
        int gpa_value = read_register(fd, GPA);

        int button_state = (gpa_value >> led_sequence[i]) & 0x01;
        if (button_state == 1) { // button is released
          // Only the expected button is released
          button_pressed = false;
        }
      }

    }

    sleep(2);
    if (round == MAX_SEQUENCE_LENGTH + 1) {
      game_over = true;
      printf("You win all ten rounds! Congratz.");
    }
  }
  //    if (wait_for_button_press(fd, 0)) {
  //        printf("Correct\n");
  //    }else{
  //        printf("Wrong\n");
  //    }

  //while(true){
  //    int gpa_value = read_register(fd, GPA);
  //    int blue = (gpa_value >> 0) & 0x01; // Isolate the bit for GPA0
  //    int green = (gpa_value >> 1) & 0x01; // Isolate the bit for GPA0
  //    int red = (gpa_value >> 2) & 0x01; // Isolate the bit for GPA0
  //    int yellow = (gpa_value >> 3) & 0x01; // Isolate the bit for GPA0
  //    printf("Blue: %d Green: %d Red: %d Yellow:%d\n", blue, green, red, yellow);
  //    sleep(0.25);
  //}

  //for (int i = 0; i < MAX_SEQUENCE_LENGTH; i++){
  //    add_to_sequence(&led_sequence, &sequence_length);
  //    sleep(1);
  //    if (play_sequence(fd, led_sequence, sequence_length) != 0){
  //        printf("Error playing sequnce");
  //        break;
  //    }
  //}

  close(fd);
  return 0;
}