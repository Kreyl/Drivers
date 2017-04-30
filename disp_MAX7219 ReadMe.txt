The Drivers for 8-digit LED display
Supports MAX7219, MAX7221
SPI interface


Example:

Disp_MAX7219_t LED_Display;
...
LED_Display.Init(4, Bright75);	// "Segments Count", "Intensity"
...
int32_t Temp = -785
LED_Display.Print("", Temp, 1);	// Indication "-78.5"
...
LED_Display.Print("Ch", 5);	// Indication "Ch 5"




