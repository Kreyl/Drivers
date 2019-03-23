The Drivers for 8-digit LED display
Supports MAX7219, MAX7221
SPI interface


Example:
#define MAX_GPIO        GPIOB
#define MAX_CS          2
#define MAX_SCK         3
#define MAX_SI          5

#define MAX_SPI         SPI1
#define MAX_SPI_AF      AF0
...
Disp_MAX7219_t LED_Display;
...
LED_Display.Init(4, Bright75);	// "Segments Count", "Intensity"
...
int32_t Out = -785
LED_Display.Print("", Out, 1);	// Indication "-78.5"
...
LED_Display.Print("Ch", 5);	// Indication "Ch 5"




