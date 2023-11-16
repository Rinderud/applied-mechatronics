# Computer TUI

The Terminal User Interface is made to communicate with the AVR, sending the reference RPM for the integrated controller and visualizing the measured RPM sent from the AVR.

Given `serialport.c` and `serialport.h` is using unix dependencies and the TUI is build with this in mind, the TUI needs to be run on a unix system. During the course, this was done with `CYGWIN`.
