#include "serialport.h"
#include <stdio.h>
#include <unistd.h> //uni: UNIX =)
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

int loopbackTest(int sp)
{
	unsigned char Tx[1] = 'a';
	unsigned char Rx[1] = 'x';
	int errs = 0;

	int i;
	for (i = 0; i < 42; i++)
	{
		printf("Sending: %s    ", Tx);
		write(sp, variable, 1);

		read(sp, Rx, 1);
		if (Tx == Rx)
		{
			printf("Ok  \n");
		}
		else
		{
			printf("Err \n");
			errs++;
		}
		Tx[0]++;
	}

	if (errs == 0)
	{
		printf("Succesfully looped back! \n");
		return 1;
	}
	printf("Not complete, #errors: %d \n", errs);
	return 0;
}

int main()
{
	int rpm = 0;
	int ref = 0;

	/*Initialise serial port */
	int sp, sl;
	sp = serial_init("/dev/ttyS0", 0);
	if (sp == 0)
	{
		printf("Error! Serial port could not be opened.\n");
	}
	else
	{
		printf("Serial port open with identifier %d \n", sp);
	}

	while (loopbackTest(sp) != 1)
	{
		printf("\n Running loopback test... \n");
	}

	printf("Tests complete.");

	/* Close serial port. */
	serial_cleanup(sp);
	return 1;
}
