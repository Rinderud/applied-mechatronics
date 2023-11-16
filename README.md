---
author: Jacob Rinderud
keywords: Mechatronics, embedded programming, automatic control
---
# Applied Mechatronics

This project is a major part of the Applied Mechatronics course (EIEN45). I took the course during the fall of 2022, my lecturer and course responsible was Fran Marquez.

## Goal

    To build an embedded servo system, starting from the component level.

Sub-goals include four different milestones; Serial communication, AVR board design, A dimmer using an encoder and PWM, and speed measurement and control.

Each sub-goal illustrates an important part of the project.

- Serial communication,

means that we are using the serial port on a computer to communicate with the AVR. The top of the [overview map](./Overview_map.pdf) is included in this were the design for the serial and opto was given.

- AVR board design

means that an AVR board that is similar to an arduino is designed. Capable of using all the other parts of the project. Including some debug LEDs and programming interface for the microcontroller.

- Dimmer

is defined as using a rotary encoder on the ENC board, and filtering the signals from it, to adjust the PWM signal amplified using the Amp board to power a lightbuld.

- Speed measurement

means using the rotary encoder situated on the motor, which is the same as the one on the ENC board, to calculate and measure the speed of the motor. This calculation involves counting interrupts and using the microcontrollers parameters, such as clock frequencies, to scale it into RPM.

- Automatic control

using the measured speed and the reference speed from the serial communication to do PI control. This involves using fixed point arithmatic and some tuning.
