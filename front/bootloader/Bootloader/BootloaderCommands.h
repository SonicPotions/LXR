/*
 * BootloaderCommands.h
 *
 * Created: 07.07.2012 21:42:54
 *  Author: Julian Schmidt
 */


#ifndef BOOTLOADERCOMMANDS_H_
#define BOOTLOADERCOMMANDS_H_


/** send to the bootloader after reset to tell it to get ready for the new firmware.
 * if no INIT_BOOTLOADER message is received the bootloader will start the normal program and not update the firmware.
 * The bootloader sends ACK back to tell the host it is ready
 */
#define INIT_BOOTLOADER 1

/** after the write data cmd is received
 *  we expect a 4 byte message with unsigned int 32 bit data. 1st byte is MSB 4th byte is LSB
 */
#define WRITE_DATA		2

/** set the address for the next received data block.
    we expect a 4 byte message with unsigned int 32 bit address
	the next received WRITE_DATA block will be written to this address
*/
#define WRITE_ADDRESS 3

/** send by the bootloadr to the host to confirm a received message*/
#define ACK				4
#define NACK			5

/** signal end of transmission and end bootloader -> start application */
#define END_BOOTLOADER	6


#endif /* BOOTLOADERCOMMANDS_H_ */
