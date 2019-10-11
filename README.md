# Laser Tag Game

## by Anthony De Belen

### Project Background: 
A project that was started during my senior year in UC Riverside for the course CS122A - Intermediate Embedded and Real Time Systems.

### Introduction:
Previously called "Shooter Game", this laser tag game was inspired by arcade games which I frequently go to during my childhood days. The player will have control on the movement of the laser attached to a stepper motor. There will be a total of 8 targets labeled from 1 through 8. The motor is controlled by two motion sensors that, once triggered, will rotate the motor 45 degrees clockwise or counterclockwise. The LCD will display the score and status of the game.

### Check the PROJECT REPORT file for initial realease information.

### Version Control and Changes:
10/10/2019
`-Laser SM added which will be the basis for all actions related to the laser. Examples are reloading, shooting, cooldowns, etc. WIP`
`-USART SM added with a period of 100ms which cycles between sending text codes, total hit values, sound codes(WIP) and ammo values(WIP).
The USART SM solves the porblem of not being able to send more than 2 information values to the slave mcU. Before only text code and total hits value are sent to the slave mcU using USART1 and USART2 respectively. Now, the master mcU can send 1 information to the slave mcU every 100ms and the slave mcU will be able to handle this information and show the necessary outputs desired.`
