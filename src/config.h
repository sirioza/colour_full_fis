// Waveshare ESP32-S3-Touch-LCD-2.8
#define TFT_MOSI_PIN  45
#define TFT_SCLK_PIN  40
#define TFT_CS_PIN    42
#define TFT_DC_PIN    41
#define TFT_RST_PIN   39
#define TFT_BL_PIN    5
#define TFT_ROTATION  2
#define TFT_SPI_SPEED 40000000

// BUTTONS
#define BTN_NEXT  43
#define BTN_PREV  44
#define BTN_SET   11

// CAN
#define CAN_TX_PIN          15
#define CAN_RX_PIN          18
#define CAN_FRAME_DATA_LEN  8

// COLORS
#ifndef ILI9341_BLACK
#define ILI9341_BLACK   ST77XX_BLACK
#define ILI9341_BLUE    ST77XX_BLUE
#define ILI9341_RED     ST77XX_RED
#define ILI9341_GREEN   ST77XX_GREEN
#define ILI9341_CYAN    ST77XX_CYAN
#define ILI9341_MAGENTA ST77XX_MAGENTA
#define ILI9341_YELLOW  ST77XX_YELLOW
#define ILI9341_WHITE   ST77XX_WHITE
#endif

// CLUSTERS ID
#define ENGINE1_ID        0x280
#define ENGINE2_ID        0x288
#define ENGINE5_ID        0x480
#define ENGINE6_ID        0x488 //unused
#define BREAK1_ID         0x1A0
#define BREAK2_ID         0x5A0
#define BREAK3_ID         0x4A0 //unused
#define KOMBI1_ID         0x320
#define KOMBI2_ID         0x420
#define KOMBI3_ID         0x520
#define AIRBAG1_ID        0x50
#define SYSTEMINFO1_ID    0x5D0 //unused
#define NO_FOUND_ID       0x5D8 //unused

// SCREEN
#define SCREEN_WIDTH        240
#define STRIP_HEIGHT        320
#define SCREEN_HEIGHT       320
#define REFRESH_SCREEN_TIME 300
#define WIDTH_0000_VAL      48
#define WIDTH_000_0_VAL     58
#define WIDTH_000000_VAL    72

// ERRORS
#define BIT_FUEL  (1 << 0)
#define BIT_ABS   (1 << 1)
#define BIT_AIR   (1 << 2)
#define BIT_ICE   (1 << 3)