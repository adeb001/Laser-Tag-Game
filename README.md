# Laser Tag Game

## by Anthony De Belen

### Project Background: 
A project that was started during my senior year in UC Riverside for the course CS122A - Intermediate Embedded and Real Time Systems.

### Introduction:
Previously called "Shooter Game", this laser tag game was inspired by arcade games which I frequently go to during my childhood days. The player will have control on the movement of the laser attached to a stepper motor. There will be a total of 8 targets labeled from 1 through 8. The motor is controlled by two motion sensors that, once triggered, will rotate the motor 45 degrees clockwise or counterclockwise. The LCD will display the score and status of the game.

### Check the PROJECT REPORT file for initial realease information.

### Version Control and Changes:
* 10/10/2019

`-Laser SM added which will be the basis for all actions related to the laser. Examples are reloading, shooting, cooldowns, etc. WIP`

`-USART SM added with a period of 100ms which cycles between sending text codes, total hit values, sound codes(WIP) and ammo values(WIP).
The USART SM solves the porblem of not being able to send more than 2 information values to the slave mcU. Before only text code and total hits value are sent to the slave mcU using USART1 and USART2 respectively. Now, the master mcU can send 1 information to the slave mcU every 100ms and the slave mcU will be able to handle this information and show the necessary outputs desired.`

* 10/17/2019

`-Background music is added to the game. A sound buzzer plays one of three melodies depending on the stage of the game. This is done by introducing pulse width modulation that converts an array of frequency values into sounds. `

`-Music SM was added to the slave microcontroller which determines the melody to play. The melody will be played continuously until the state of the game changes. The state of the game is sent via USART by the master microcontroller.`

`-Sound buzzer is connected to PIN B6.`

### Bug Tracker/Potential Fixes:

* Plays once

`-The game can only be played once properly. Consecutive games have trouble resetting all values and goes into weird states.`

`-I have seen weird transitions in my state machines that might have caused this bug. As my priority is perfecting the base game first, I will not pay attention to this bit until I get the game to work bug free.`

* Motor doesn't not move in divisibles of 45 degrees

`-The original state of the motor in degrees is not divisible by 45. Therefore the alignment of the motor is a little bit off.`

`-The motor needs to be calibrated to be divisible by 45 to make sure when the game is build on a case, there will be no problem with the aiming angles.`
