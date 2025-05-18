#ifndef SERIAL_PORT_HEADER
#define SERIAL_PORT_HEADER


#include <stdint.h>

// Defining the serial port struct, the definition is hidden in the
// c file as no one really needs to know this.
struct _SerialPort;
typedef struct _SerialPort SerialPort;


// make any number of instances of the serial port (they are extern because
//   they are fixed, unique values)
extern SerialPort USART1_PORT;


// The user to select the baud rate
enum {
  BAUD_9600,
  BAUD_19200,
  BAUD_38400,
  BAUD_57600,
  BAUD_115200
};


// serial_initialise - initialise the serial port
// Input: baud rate as defined in the enum
void serial_initialise(uint32_t baudRate, SerialPort *serial_port, void (*completion_function)(void), void (*rx_callback)(char *, uint32_t));


// serial_output_char - output a char to the serial port
//  note: this version waits until the port is ready (not using interrupts)
// Input: char to be transferred
void serial_output_char(char data, SerialPort *serial_port);


// serial_output_string - output a NULL TERMINATED string to the serial port
// Input: pointer to a string
void serial_output_string(char *string, SerialPort *serial_port);


// serial_ read_line - read incoming characters into buffer until a terminating character is received
// Returns number of characters read
void serial_read_line(char *buffer, uint32_t max_len, SerialPort *serial_port, char terminator);

// enable_interrupts - enable interrupts for uart
void enable_interrupts(SerialPort *serial_port);

#endif
