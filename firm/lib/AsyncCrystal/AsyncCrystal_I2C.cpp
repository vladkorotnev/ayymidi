// Based on the work by DFRobot

#include "AsyncCrystal_I2C.h"
#include <inttypes.h>
#if defined(ARDUINO) && ARDUINO >= 100

#include "Arduino.h"

inline size_t AsyncCrystal_I2C::write(uint8_t value) {
	send(value, Rs);
	return 1;
}

#else
#include "WProgram.h"

#define printIIC(args)	Wire.send(args)
inline void AsyncCrystal_I2C::write(uint8_t value) {
	send(value, Rs);
}

#endif



// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set: 
//    DL = 1; 8-bit interface data 
//    N = 0; 1-line display 
//    F = 0; 5x8 dot character font 
// 3. Display on/off control: 
//    D = 0; Display off 
//    C = 0; Cursor off 
//    B = 0; Blinking off 
// 4. Entry mode set: 
//    I/D = 1; Increment by 1
//    S = 0; No shift 
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

AsyncCrystal_I2C::AsyncCrystal_I2C(uint8_t lcd_Addr,uint8_t lcd_cols,uint8_t lcd_rows)
{
  _Addr = lcd_Addr;
  _cols = lcd_cols;
  _rows = lcd_rows;
  _backlightval = LCD_NOBACKLIGHT;
}

void AsyncCrystal_I2C::oled_init(){
  _oled = true;
	init_priv();
}

void AsyncCrystal_I2C::init(){
	init_priv();
}

void AsyncCrystal_I2C::init_priv()
{
	i2c.begin();
	i2c.setRetryCount(5);
	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
	begin(_cols, _rows);  
}

void AsyncCrystal_I2C::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
	if (lines > 1) {
		_displayfunction |= LCD_2LINE;
	}
	_numlines = lines;

	// for some 1 line displays you can select a 10 pixel high font
	if ((dotsize != 0) && (lines == 1)) {
		_displayfunction |= LCD_5x10DOTS;
	}

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	delay(50); 
  
	// Now we pull both RS and R/W low to begin commands
	_asyncwrite_write(0, 0);
	flush();
	delay(1000);

  	//put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet
	// figure 24, pg 46
	
	  // we start in 8bit mode, try to set 4 bit mode
	uint8_t set_4bit_mode = 0x03 << 4;
   _asyncwrite_write(set_4bit_mode, 0);
   flush();
   delayMicroseconds(65000);
   
   // second try
   _asyncwrite_write(set_4bit_mode, 0);
   flush();
   delayMicroseconds(65000);
   
   // third go!
   _asyncwrite_write(set_4bit_mode, 0);
   flush();
   delayMicroseconds(65000);
   
   // finally, set to 4-bit interface
   _asyncwrite_write(0x02 << 4, 0);
   flush();
   delayMicroseconds(65000);


	// set # lines, font size, etc.
	command(LCD_FUNCTIONSET | _displayfunction);  
	flush();
	
	// turn the display on with no cursor or blinking default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	display();
	flush();
	
	// clear it off
	clear();
	flush();
	
	// Initialize to default text direction (for roman languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	
	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);
	flush();
	
	home();
	flush();
}

/********** high level commands, for the user! */
void AsyncCrystal_I2C::clear(){
	command(LCD_CLEARDISPLAY);// clear display, set cursor position to zero
	_asyncwrite_delay(2000);  // this command takes a long time!
  if (_oled) setCursor(0,0);
}

void AsyncCrystal_I2C::home(){
	command(LCD_RETURNHOME);  // set cursor position to zero
	_asyncwrite_delay(2000);  // this command takes a long time!
}

void AsyncCrystal_I2C::setCursor(uint8_t col, uint8_t row){
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if ( row > _numlines ) {
		row = _numlines-1;    // we count rows starting w/0
	}
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void AsyncCrystal_I2C::noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void AsyncCrystal_I2C::display() {
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void AsyncCrystal_I2C::noCursor() {
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void AsyncCrystal_I2C::cursor() {
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void AsyncCrystal_I2C::noBlink() {
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void AsyncCrystal_I2C::blink() {
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void AsyncCrystal_I2C::scrollDisplayLeft(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void AsyncCrystal_I2C::scrollDisplayRight(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void AsyncCrystal_I2C::leftToRight(void) {
	_displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void AsyncCrystal_I2C::rightToLeft(void) {
	_displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void AsyncCrystal_I2C::autoscroll(void) {
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void AsyncCrystal_I2C::noAutoscroll(void) {
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations
// with custom characters
void AsyncCrystal_I2C::createChar(uint8_t location, uint8_t charmap[]) {
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
		write(charmap[i]);
	}
}

//createChar with PROGMEM input
void AsyncCrystal_I2C::createChar(uint8_t location, const char *charmap) {
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
	    	write(pgm_read_byte_near(charmap++));
	}
}

// Turn the (optional) backlight off/on
void AsyncCrystal_I2C::noBacklight(void) {
	_backlightval=LCD_NOBACKLIGHT;
	_asyncwrite_write(0, 0);
}

void AsyncCrystal_I2C::backlight(void) {
	_backlightval=LCD_BACKLIGHT;
	_asyncwrite_write(0, 0);
}



/*********** mid level commands, for sending data/cmds */

inline void AsyncCrystal_I2C::command(uint8_t value) {
	send(value, 0);
}


/************ low level data pushing commands **********/

bool AsyncCrystal_I2C::busy() {
	return !_queue.isEmpty();
}

void AsyncCrystal_I2C::flush() {
	while(busy()) loop();
}

void AsyncCrystal_I2C::_asyncwrite_delay(uint16_t time) {
	_queue.enqueue(async_write_queue_item_t {
		op: WAIT_MICROS,
		info: async_write_queue_info_t { wait_info: async_wait_info_t { wait_length: time } }
	});
}

void AsyncCrystal_I2C::_asyncwrite_write(uint8_t value, uint8_t mode) {
	_queue.enqueue(async_write_queue_item_t {
		op: SEND,
		info: async_write_queue_info_t { send_info: async_send_info_t { data: value, tx_mode: mode } }
	});
}

void AsyncCrystal_I2C::_asyncwrite_callback(uint8_t status, void* arg) {
	AsyncCrystal_I2C * that = (AsyncCrystal_I2C*) arg;
	that->_asyncwrite_tx_cycle();
}

void AsyncCrystal_I2C::_asyncwrite_tx_cycle() {
	async_write_queue_item_t* current_task = _queue.getHeadPtr();
	if(current_task == nullptr || current_task->op != SEND) return;

all_over_again:
	uint8_t this_time_data = current_task->info.send_info.tx_mode | _backlightval;

	switch(_tx_part) {
		case HIGH_NIBBLE:
			this_time_data |= (current_task->info.send_info.data & 0xF0);
			break;

		case LOW_NIBBLE:
			this_time_data |= ((current_task->info.send_info.data << 4) & 0xF0);
			break;
	}

	switch(_tx_stage) {
		case WILL_SET_BUS: // set value on the bus (can be skipped but some LCD don't like it)
			i2c.send(_Addr, &this_time_data, sizeof(uint8_t), &this->_asyncwrite_callback, (void*) this);
			_tx_stage = DID_SET_BUS;
			break;

		case DID_SET_BUS: // now push en to high
			this_time_data |= En;
			i2c.send(_Addr, &this_time_data, sizeof(uint8_t), &this->_asyncwrite_callback, (void*) this);
			_tx_stage = DID_EN_HIGH;
			break;

		case DID_EN_HIGH: // now make sure we have it on for 2us or more
			_wait_start = micros();
			_tx_stage = WAIT_EN_LOW;
			break;

		case WAIT_EN_LOW: 
			{
				unsigned long now_time = micros();
				if(now_time - _wait_start > 2) {
					// time has passed, push EN low again
					i2c.send(_Addr, &this_time_data, sizeof(uint8_t), &this->_asyncwrite_callback, (void*) this);
					_tx_stage = DID_EN_LOW;
				}
			}
			break;

		case DID_EN_LOW: // give display 50us to settle
			_wait_start = micros();
			_tx_stage = WAIT_SETTLE;
			break;

		case WAIT_SETTLE: 
			{
				unsigned long now_time = micros();
				if(now_time - _wait_start > 50) {
					// time has passed, ready to do next nibble or next queue item
					if(_tx_part == HIGH_NIBBLE) {
						_tx_part = LOW_NIBBLE;
						_tx_stage = WILL_SET_BUS;
						goto all_over_again; // saving some stack mem :p
					} else {
						_queue.dequeue();
						_waiting = false;
					}
				}
			}
			break;
	}
}

void AsyncCrystal_I2C::loop() {
	async_write_queue_item_t* current_task = _queue.getHeadPtr();
	if(current_task != nullptr) {
		switch(current_task->op) {
			case WAIT_MICROS:
				{
					unsigned long now_time = micros();
					if(!_waiting) {
						_wait_start = now_time;
						_waiting = true;
					} else if(now_time - _wait_start > current_task->info.wait_info.wait_length) {
						_queue.dequeue();
						_waiting = false;
					} 
				}
				break;

			case SEND:
				if(!_waiting) {
					_tx_part = HIGH_NIBBLE;
#ifdef ASYNCCRYSTAL_SKIP_BUS_SET // for some displays that are fine with it
					_tx_stage = DID_SET_BUS;
#else
					_tx_stage = WILL_SET_BUS;
#endif
					_waiting = true;
					_asyncwrite_tx_cycle();
				}
				else if (_tx_stage == WAIT_EN_LOW || _tx_stage == WAIT_SETTLE) {
					_asyncwrite_tx_cycle();
				}
				break;
		}
	}

	i2c.loop();
}

// write either command or data
void AsyncCrystal_I2C::send(uint8_t value, uint8_t mode) {
	_asyncwrite_write(value, mode);
}

// Alias functions

void AsyncCrystal_I2C::cursor_on(){
	cursor();
}

void AsyncCrystal_I2C::cursor_off(){
	noCursor();
}

void AsyncCrystal_I2C::blink_on(){
	blink();
}

void AsyncCrystal_I2C::blink_off(){
	noBlink();
}

void AsyncCrystal_I2C::load_custom_character(uint8_t char_num, uint8_t *rows){
		createChar(char_num, rows);
}

void AsyncCrystal_I2C::setBacklight(uint8_t new_val){
	if(new_val){
		backlight();		// turn backlight on
	}else{
		noBacklight();		// turn backlight off
	}
}

void AsyncCrystal_I2C::printstr(const char c[]){
	//This function is not identical to the function used for "real" I2C displays
	//it's here so the user sketch doesn't have to be changed 
	print(c);
}


// unsupported API functions
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void AsyncCrystal_I2C::off(){}
void AsyncCrystal_I2C::on(){}
void AsyncCrystal_I2C::setDelay (int cmdDelay,int charDelay) {}
uint8_t AsyncCrystal_I2C::status(){return 0;}
uint8_t AsyncCrystal_I2C::keypad (){return 0;}
uint8_t AsyncCrystal_I2C::init_bargraph(uint8_t graphtype){return 0;}
void AsyncCrystal_I2C::draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_col_end){}
void AsyncCrystal_I2C::draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_row_end){}
void AsyncCrystal_I2C::setContrast(uint8_t new_val){}
#pragma GCC diagnostic pop
	
