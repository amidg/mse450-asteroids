MSE450. Summer 2021. Simon Fraser University

Project Asteroids (1971) developed by Group 5:
Dmitrii Gusev – 301297008

Functionality:
- Dual ADC on channels PE3 and PE2
- Dual UART:
    1. UART0 via virtual COM port
    2. UART5 via additional MCP2221A circuit
- Nokia 5110 LCD screen
- On-board LEDs and buttons using interrupt

WARNING!
Project is designed and tested for real-world hardware. TExas simulator is not supported and will never be supported. Use real hardware, not knock-offs. 

How to use?
1. Set compiler variant on lines 50 and 51. Put 1 for the desired framework. Never put 1 on both options.
2. Build code (VS Code or Keil)
3. Flash code (VS Code or Keil)
4. Open UART port using minicom (Linux/MacOS, check /dev/ttyACMx in terminal) or PuTTY (Windows, check Device Manager).
5. Reset board whenever downloading is completed
6. Press SW1 to start game
7. Use joystick to point in the right direction
8. Use SW1 to shoot
9. Reset after message "YOU DIED"

Project is released under GPLv3 and is completely open-source. Team of Dmitrii, Khamat and Aldiyar does not support proprietary firmware for school projects. 
