# NEN project
Essentially, we got a kit from our teacher and a "*do something with it*".
I always wanted to play around with 3D graphics and ray marching, so I decided this was it.

### Components
 - Some PIC8
	 - 2k RAM
	 - 32k Flash
	 - A sprinkle of EEPROM, could 
	 - 48MHz, but you know how it is with these PICs, each instruction takes 2 clock cycles, so it's really just 24MHz (still respectable tho, for an 8 bit thing)
 - Some 128x64 LCD
	 - It has a terrible fade time, I'm planning to use that for multiple gray levels
	 - The backlight is kinda meh, but we have PWM control over it, so that's nice
	 - It's also upside down for some reason
 - 5 Buttons
	 - Each one also has a red LED above it, which we plan to use as a health bar

I'd post the schematic, but I'll have to ask first.

### The state so far
Left two buttons walk, right two buttons turn. Center resets the location.
The renderer itself now runs at 35 FPS.

![ezgif com-optimize](https://github.com/user-attachments/assets/7955fe87-7f4a-43e5-aa2a-92a638bc3312)
