#include "output.h"

/*
 * Revised I2C LCD writing.
 *
 * A collection of functions for initialising, configuring, reading and writing the LCD screen.
 *
 * Method of Use:
 * 		- openLCD to open the LCD 'file' I/O
 * 		- writeLCD to write to the LCD display
 * 		||||||||||Primitive Functions||||||||||||||||
 * 		- I2cWrite to write configuration data accordingly
 *		- Initialise_LCD initialises the LCD screen with a list of I2C writes
 * 		- SetCursor sets the cursor data.
 */

/*
 * Adem Karakaya || s36031133 || 24.09.2019
 * Personal Notes:
 *
 * 		- 20 Characters Max
 * 		- Can update characters individually
 *
 */

void *LCD_Control(void *data) {
	/* RUN THIS AS THREAD */

	int error;
	int file;
	int *filep = &file;

	char L1[21] = {'B', 'O', 'O', 'M', ' ', 'G', 'A', 'T', 'E', 'S', ' ', '=', ' ', ' ', 'N', 'U', 'L', 'L', ' ', ' '};
	char L2[21] = {'P', 'E', 'D', 'E', 'S', 'T', 'R', 'I', 'A', 'N', ' ', '=', ' ', ' ', 'N', 'U', 'L', 'L', ' ', ' '};
	//   13   14	 15   16   17   18	 19
	ouput_thread_data_t *thread_data = (ouput_thread_data_t *)data;
	rail_state_data_t *rail_state_data = thread_data->rail_state_data;
	rail_state_t railOutputState;

	error = openLCD(filep);

	if (error != 0) {
		printf("Fatal Error: LCD could not initialize.");
		exit(EXIT_FAILURE);
	}

	writeLCD(0, 0, L1, filep);
	writeLCD(1, 0, L2, filep);

	while (1) {

		// get rail state
		pthread_mutex_lock(&rail_state_data->mutex_state);

		while (!rail_state_data->data_ready) {
			pthread_cond_wait(&rail_state_data->cond_state_changed, &rail_state_data->mutex_state);
		}

		railOutputState = rail_state_data->current_state;

		rail_state_data->data_ready = false;
		pthread_mutex_unlock(&rail_state_data->mutex_state);

		switch (railOutputState) {
			case NO_TRAIN:
				L1[12] = 'R';
				L1[13] = 'A';
				L1[14] = 'I';
				L1[15] = 'S';
				L1[16] = 'E';
				L1[17] = 'D';
				L2[18] = ' ';
				L2[12] = 'O';
				L2[13] = 'P';
				L2[14] = 'E';
				L2[15] = 'N';
				L2[16] = 'E';
				L2[17] = 'D';
				L2[18] = ' ';
				writeLCD(0, 0, L1, filep);
				writeLCD(1, 0, L2, filep);
				break;

			case TRAIN:
				L1[12] = 'L';
				L1[13] = 'O';
				L1[14] = 'W';
				L1[15] = 'E';
				L1[16] = 'R';
				L1[17] = 'E';
				L2[18] = 'D';
				L2[12] = 'C';
				L2[13] = 'L';
				L2[14] = 'O';
				L2[15] = 'S';
				L2[16] = 'E';
				L2[17] = 'D';
				L2[18] = ' ';
				writeLCD(0, 0, L1, filep);
				writeLCD(1, 0, L2, filep);
				break;

			default:
				L1[12] = 'L';
				L1[13] = 'O';
				L1[14] = 'W';
				L1[15] = 'E';
				L1[16] = 'R';
				L1[17] = 'E';
				L2[18] = 'D';
				L2[12] = 'C';
				L2[13] = 'L';
				L2[14] = 'O';
				L2[15] = 'S';
				L2[16] = 'E';
				L2[17] = 'D';
				L2[18] = ' ';
				writeLCD(0, 0, L1, filep);
				writeLCD(1, 0, L2, filep);
				break;
		}
	}
}

// Writes to I2C
int I2cWrite_(int *fd, uint8_t Address, uint8_t mode, uint8_t *pBuffer, uint32_t NbData) {
	/* Writes anything (including control data) to the I2C via packets.
	 *
	 *
	 *
	 *
	 */
	i2c_send_t hdr;
	iov_t sv[2];
	int status, i;

	uint8_t LCDpacket[21] = {}; // limited to 21 characters  (1 control bit + 20 bytes)

	// set the mode for the write (control or data)
	LCDpacket[0] = mode; // set the mode (data or control)

	// copy data to send to send buffer (after the mode bit)
	for (i = 0; i < NbData + 1; i++)
		LCDpacket[i + 1] = *pBuffer++;

	hdr.slave.addr = Address;
	hdr.slave.fmt = I2C_ADDRFMT_7BIT;
	hdr.len = NbData + 1; // 1 extra for control (mode) bit
	hdr.stop = 1;

	SETIOV(&sv[0], &hdr, sizeof(hdr));
	SETIOV(&sv[1], &LCDpacket[0], NbData + 1); // 1 extra for control (mode) bit
											   // int devctlv(int filedes, int dcmd,int sparts, int rparts, const iov_t *sv, const iov_t *rv, int *dev_info_ptr);
	status = devctlv(*fd, DCMD_I2C_SEND, 2, 0, sv, NULL, NULL);

	//if (status != EOK)
	//printf("status = %s\n", strerror ( status ));

	return status;
}

void SetCursor(int *fd, uint8_t LCDi2cAdd, uint8_t row, uint8_t column) {
	/* Sets cursor
	 * Don't use
	 *
	 *
	 *
	 *
	 *
	 *
	 *
	 *
	 */

	uint8_t position = 0x80; // SET_DDRAM_CMD (control bit)
	uint8_t rowValue = 0;
	uint8_t LCDcontrol = 0;
	if (row == 1)
		rowValue = 0x40; // memory location offset for row 1
	position = (uint8_t)(position + rowValue + column);
	LCDcontrol = position;
	I2cWrite_(fd, LCDi2cAdd, Co_Ctrl, &LCDcontrol, 1); // write data to I2C
}

void Initialise_LCD(int *fd, _Uint32t LCDi2cAdd) {
	/* This will initialise the LCD.
	 * DON'T USE THIS
	 *
	 *
	 *
	 *
	 *
	 *
	 */

	uint8_t LCDcontrol = 0x00;

	//   Initialise the LCD display via the I2C bus
	LCDcontrol = 0x38;								   // data byte for FUNC_SET_TBL1
	I2cWrite_(fd, LCDi2cAdd, Co_Ctrl, &LCDcontrol, 1); // write data to I2C

	LCDcontrol = 0x39;								   // data byte for FUNC_SET_TBL2
	I2cWrite_(fd, LCDi2cAdd, Co_Ctrl, &LCDcontrol, 1); // write data to I2C

	LCDcontrol = 0x14;								   // data byte for Internal OSC frequency
	I2cWrite_(fd, LCDi2cAdd, Co_Ctrl, &LCDcontrol, 1); // write data to I2C

	LCDcontrol = 0x79;								   // data byte for contrast setting
	I2cWrite_(fd, LCDi2cAdd, Co_Ctrl, &LCDcontrol, 1); // write data to I2C

	LCDcontrol = 0x50;								   // data byte for Power/ICON control Contrast set
	I2cWrite_(fd, LCDi2cAdd, Co_Ctrl, &LCDcontrol, 1); // write data to I2C

	LCDcontrol = 0x6C;								   // data byte for Follower control
	I2cWrite_(fd, LCDi2cAdd, Co_Ctrl, &LCDcontrol, 1); // write data to I2C

	LCDcontrol = 0x0C;								   // data byte for Display ON
	I2cWrite_(fd, LCDi2cAdd, Co_Ctrl, &LCDcontrol, 1); // write data to I2C

	LCDcontrol = 0x01;								   // data byte for Clear display
	I2cWrite_(fd, LCDi2cAdd, Co_Ctrl, &LCDcontrol, 1); // write data to I2C
}

int openLCD(int *filep) {
	/* USE THIS
	 * This is to open the LCD 'file' I/O and init.
	 *
	 * int * I/O 'file' pointer.
	 *
	 *
	 *
	 *
	 */

	int error;
	volatile uint8_t LCDi2cAdd = 0x3C;
	_Uint32t speed = 200000; // nice and slow 10000 (will work with 200000)

	// Open I2C resource and set it up
	if ((*filep = open("/dev/i2c1", O_RDWR)) < 0) // OPEN I2C1
		error = -1;

	error = devctl(*filep, DCMD_I2C_SET_BUS_SPEED, &(speed), sizeof(speed), NULL); // Set Bus speed
	if (error) {
		//fprintf(stderr, "Error setting the bus speed: %d\n",strerror ( error ));
		exit(EXIT_FAILURE);
	} else
		//printf("Bus speed set = %d\n", speed);

		Initialise_LCD(filep, LCDi2cAdd);

	usleep(1);
	return error;
}

int writeLCD(int row, int col, char write[], int *filep) {
	int werror;
	/* USE THIS
	 * Int of Row Chosen (0,1)
	 * Int of Column Chosen (0-20)
	 * Char [] of what to write
	 * (Lim 20 Char)
	 *
	 * This will open the I2C file, then write to it what you will.
	 * Up: 0x5E Left: 0x7F Right: 0x7E
	 */

	volatile uint8_t LCDi2cAdd = 0x3C;
	// write some Text to the LCD screen

	size_t n = strlen(write);
	uint8_t LCDdata[n];
	strcpy(LCDdata, write);

	// set cursor on LCD to first position first line
	SetCursor(filep, LCDi2cAdd, row, col);
	werror = I2cWrite_(filep, LCDi2cAdd, DATA_SEND, &LCDdata[0], sizeof(LCDdata));
	// write new data to I2C

	//printf("\nComplete\n");

	return werror;
}
