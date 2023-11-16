#include "serialport.h"
#include <stdio.h>
#include <unistd.h> //uni: UNIX =)
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

const int min_rpm = 0;
const int max_rpm = 120;

int UI(int speed)
{
	int bar_speed = speed * 15 / (max_rpm - min_rpm);
	if (speed < 100)
	{
		if (speed < 10)
		{
			printf("Speed:   %d RPM ", speed);
		}
		else
		{
			printf("Speed:  %d RPM ", speed);
		}
	}
	else
	{
		printf("Speed: %d RPM ", speed);
	}

	printf("%.*s", bar_speed + 1,
		   "|---------------|"); // "Bar-graph" type thing.
	printf("\n");
	return 1;
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

	unsigned char cin[1];
	unsigned char cout[1];

	while (ref >= 0)
	{
		int input = 0;
		printf("Ref: ");
		scanf("%d", &input);

		if (input < 0)
		{
			int i;
			for (i = 0; i < 25; i++)
			{
				write(sp, cout, 1);
				read(sp, cin, 1);
				UI(cin[0]);
				cout[0] = ("%d", ref);
			}
		}
		else
		{
			ref = input;
		}
		if (input == -42)
		{
			ref = -1;
		}
	}

	/* Close serial port. */
	serial_cleanup(sp);
	return 1;
}
