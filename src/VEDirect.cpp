/******************************************************************
 VEDirect Arduino

 Copyright 2018, 2019, Brendan McLearie
 Distributed under MIT license - see LICENSE.txt

 See README.md

 File: VEDirect.cpp
 - Implementation
 Updates:
 - 2019-07-14 See VEDirect.h
******************************************************************/

#include "VEDirect.h"

VEDirect::VEDirect(HardwareSerial& port):
	VESerial(port)
	// Initialise the serial port that the
	// VE.Direct device is connected to and
	// store it for later use.
{
}

VEDirect::~VEDirect() {
	// virtual destructor
}

uint8_t VEDirect::begin() {
	// Check connection the serial port
	VESerial.begin(19200);

	// Remove fluff, do not really understand
	// why we would keep opening/closing
	/*
	if (VESerial) {
		delay(500);
		if(VESerial.available()) {
			VESerial.flush();
			VESerial.end();
			return 1;
		}
	}
	*/
	return 0;
}

int32_t VEDirect::read(uint8_t target) {
	// Read VE.Direct lines from the serial port
	// Search for the label specified by enum target
	// Extract and return the corresponding value
	// If value is "ON" return 1. If "OFF" return 0;

	uint16_t loops = VED_MAX_READ_LOOPS;
	uint8_t lines = VED_MAX_READ_LINES;
	int32_t ret = 0;					// The value to be returned
	char line[VED_LINE_SIZE] = "\0";	// Line buffer
	uint8_t idx = 0;					// Line buffer index
	char* label;
	char* value_str;
	int8_t b;							// byte read from the stream

	// Should already be open
	// VESerial.begin(VED_BAUD_RATE);

	while (lines > 0) {
		if (VESerial.available()) {
			while (loops > 0) {
				b = VESerial.read();
				if ((b == -1) || (b == '\r')) { 	// Ignore '\r' and empty reads
					loops--;
				} else {
					if (b == '\n') { 				// EOL
						break;
					} else {
						if (idx < VED_LINE_SIZE) {
							line[idx++] = b;		// Add it to the buffer
						} else {
							return 0;				// Buffer overrun
						}
					}
				}
			}
			line[idx] = '\0';						// Terminate the string

			// Line in buffer
			if (target == VE_DUMP) {
				// Diagnostic routine - just print to Serial0;
				Serial.println(line);
				// Continue on rather than break to reset for next line
			}

			label = strtok(line, "\t");

			if (label && (strcmp(label, ved_labels[target]) == 0)) {
				value_str = strtok(0, "\t");
				if (value_str[0] == 'O') { 		//ON OFF type
					if (value_str[1] == 'N') {
						ret = 1;	// ON
						break;
					} else {
						ret = 0;	// OFF
						break;
					}
				} else {
					sscanf(value_str, "%ld", &ret);
					break;
				}
			} else {			// Line not of interest
				lines--;
				loops = VED_MAX_READ_LOOPS;
				line[0] = '\0';
				idx = 0;
			}
		}
	}
	return ret;
}

/**
 * @name: burst_read
 * 
 * @brief: The following function reads data from ther serial port,
 * parses it for data from the MPPT, and sends relevant information back to the calling fuction.
 * Note that this function will only monitor the serial port for a maximum of VED_MAX_TIMEOUT 
 * interval before the function returns. This is so the function does not block when
 * no new data is being sent from the MPPT.
 * 
 * @param[in]: uint8_t targets - A list of MPPT variables the calling function
 * would like to capture from the MPPT.
 * 
 * @param[in]: uint8_t num_targets - Number of targets this function will search for
 * in the serial stream. This must match the number of items in the 'targets' array.
 * 
 * @param[out]: int32_t return_targets - A 2-D array that is used by this function to store
 * the requested data from 'targets' and to be sent back to the the calling function. The first colum
 * in the array is to store the data captured from the serial port. The second column in the array
 * is used a tag to mark which data got updated on the burst_read. If a 'target' was captured from the
 * serial stream, the second column in 'return_targets' is marked with the target's ved_label. 
 * 
 * @return: NONE
 * 
 */
void VEDirect::burst_read(uint8_t targets[], int32_t return_targets[][2], uint8_t num_targets) {
	uint16_t loops = VED_MAX_READ_LOOPS;
	uint8_t num_targets_found = 0;
	char line[VED_LINE_SIZE] = "\0";	// Line buffer
	uint8_t idx = 0;					// Line buffer index
	char* label;
	char* value_str;
	int8_t b;							// byte read from the stream

	unsigned long startTime = millis();
	unsigned long currentTime = startTime;

	while ((num_targets_found < num_targets) && ((currentTime - startTime) < VED_MAX_TIMEOUT_INTERVAL)) {
		if (VESerial.available()) {
			while (loops > 0) {
				b = VESerial.read();
				if ((b == -1) || (b == '\r')) { 	// Ignore '\r' and empty reads
					loops--;
				} else {
					if (b == '\n') { 				// EOL
						break;
					} else {
						if (idx < VED_LINE_SIZE) {
							line[idx++] = b;		// Add it to the buffer
						} else {
							// Buffer overrun
							break;
						}
					}
				}
			}
			line[idx] = '\0';						// Terminate the string

			label = strtok(line, "\t");

			if (label) {
				for (uint8_t i = 0; i < num_targets; i++) {
					if (strcmp(label, ved_labels[targets[i]]) == 0) {
						value_str = strtok(0, "\t");
						if (value_str[0] == 'O') { 		//ON OFF type
							if (value_str[1] == 'N') {
								return_targets[i][0] = 1;	// ON
							} else {
								return_targets[i][0] = 0;	// OFF
							}
						} else {
							sscanf(value_str, "%ld", &return_targets[i][0]);
						}
						if (!(return_targets[i][1])) {
							return_targets[i][1] = targets[i];
							num_targets_found++;
						}
						break;
					}
				}
			}
			// Cleanup
			loops = VED_MAX_READ_LOOPS;
			line[0] = '\0';
			idx = 0;
		}
		currentTime = millis();
	}
}

void VEDirect::copy_raw_to_serial0() {
	read(VE_DUMP);
}

