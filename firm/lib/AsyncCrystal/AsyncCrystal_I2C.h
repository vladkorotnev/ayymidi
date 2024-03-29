#ifndef AsyncCrystal_I2C_h
#define AsyncCrystal_I2C_h

#include <inttypes.h>
#include "Print.h" 
#include <AsyncI2CMaster.h>
#include <ArduinoQueue.h>

#ifdef ASYNCCRYSTAL_VFD
  typedef enum __attribute__((packed)) vfd_bright  {
    BRIGHT_100 = 0b00,
    BRIGHT_75 = 0b01,
    BRIGHT_50 = 0b10,
    BRIGHT_25 = 0b11
  } vfd_bright_t;
#endif

#ifndef ASYNCCRYSTAL_QUEUE_LENGTH
// absolute minimum to fully refresh a 1602 with little headroom, fits well in atmega328
// tune as necessary otherwise
#define ASYNCCRYSTAL_QUEUE_LENGTH 64
#endif

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define En B00000100  // Enable bit
#define Rw B00000010  // Read/Write bit
#define Rs B00000001  // Register select bit

class AsyncCrystal_I2C : public Print {
public:
  AsyncCrystal_I2C(uint8_t lcd_Addr,uint8_t lcd_cols,uint8_t lcd_rows);
  void begin(uint8_t cols, uint8_t rows, uint8_t charsize = LCD_5x8DOTS );
  void clear();
  void home();
  void noDisplay();
  void display();
  void noBlink();
  void blink();
  void noCursor();
  void cursor();
  void scrollDisplayLeft();
  void scrollDisplayRight();
  void printLeft();
  void printRight();
  void leftToRight();
  void rightToLeft();
  void shiftIncrement();
  void shiftDecrement();
  void noBacklight();
  void backlight();
  void autoscroll();
  void noAutoscroll(); 
  void createChar(uint8_t, uint8_t[]);
  void createChar(uint8_t location, const char *charmap);
  // Example: 	const char bell[8] PROGMEM = {B00100,B01110,B01110,B01110,B11111,B00000,B00100,B00000};
  
  void setCursor(uint8_t, uint8_t); 
#if defined(ARDUINO) && ARDUINO >= 100
  virtual size_t write(uint8_t);
#else
  virtual void write(uint8_t);
#endif
  void command(uint8_t);
  void init();
  void oled_init();

#ifdef ASYNCCRYSTAL_VFD
void AsyncCrystal_I2C::vfd_brightness(vfd_bright_t brightness);
#endif

////compatibility API function aliases
void blink_on();						// alias for blink()
void blink_off();       					// alias for noBlink()
void cursor_on();      	 					// alias for cursor()
void cursor_off();      					// alias for noCursor()
void setBacklight(uint8_t new_val);				// alias for backlight() and nobacklight()
void load_custom_character(uint8_t char_num, uint8_t *rows);	// alias for createChar()
void printstr(const char[]);

////Unsupported API functions (not implemented in this library)
uint8_t status();
void setContrast(uint8_t new_val);
uint8_t keypad();
void setDelay(int,int);
void on();
void off();
uint8_t init_bargraph(uint8_t graphtype);
void draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_col_end);
void draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_col_end);

void loop();
void flush();
bool busy();


private:
  typedef enum __attribute__((packed)) async_write_op  {
    SEND,
    WAIT_MICROS
  } async_write_op_t;

  typedef struct __attribute__((packed)) async_wait_info {
    uint16_t  wait_length;
  } async_wait_info_t;

  typedef struct __attribute__((packed))  async_send_info {
    uint8_t data;
    uint8_t tx_mode;
  } async_send_info_t;

  typedef union __attribute__((packed)) async_write_queue_info {
    async_send_info_t send_info;
    async_wait_info_t wait_info;
  } async_write_queue_info_t;

  typedef struct __attribute__((packed)) async_write_queue_item {
    async_write_op_t op;
    async_write_queue_info_t info;
  } async_write_queue_item_t;

  typedef enum __attribute__((packed)) async_write_stage  {
    WILL_SET_BUS,
    DID_SET_BUS,
    DID_EN_HIGH,
    WAIT_EN_LOW,
    DID_EN_LOW,
    WAIT_SETTLE
  } async_write_stage_t;

  typedef enum __attribute__((packed)) async_write_part  {
    HIGH_NIBBLE,
    LOW_NIBBLE
  } async_write_part_t;

  void init_priv();
  void send(uint8_t, uint8_t);

  void _asyncwrite_write(uint8_t, uint8_t);
  void _asyncwrite_delay(uint16_t);
  void _asyncwrite_start_if_needed();
  void _asyncwrite_tx_cycle();
  static void _asyncwrite_callback(uint8_t, void*);

  uint8_t _Addr;
  uint8_t _displayfunction;
  uint8_t _displaycontrol;
  uint8_t _displaymode;
  uint8_t _numlines;
  bool _oled = false;
  uint8_t _cols;
  uint8_t _rows;
  uint8_t _backlightval;
#ifdef ASYNCCRYSTAL_VFD
  vfd_bright_t _brightness;
#endif

  AsyncI2CMaster i2c;
  ArduinoQueue<async_write_queue_item_t> _queue = ArduinoQueue<async_write_queue_item_t>(ASYNCCRYSTAL_QUEUE_LENGTH);
  bool _waiting;
  unsigned long _wait_start;
  async_write_stage_t _tx_stage;
  async_write_part_t _tx_part;
  bool _init = true;
};

#endif
