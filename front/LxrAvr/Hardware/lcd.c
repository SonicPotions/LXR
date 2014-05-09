// Ansteuerung eines HD44780 kompatiblen LCD im 4-Bit-Interfacemodus
// http://www.mikrocontroller.net/articles/HD44780
// http://www.mikrocontroller.net/articles/AVR-GCC-Tutorial/LCD-Ansteuerung
//
// Pins can be configured in lcd.h
 
#include <avr/io.h>
#include "lcd.h"
#include <util/delay.h>
#include <avr/pgmspace.h>
////////////////////////////////////////////////////////////////////////////////
// Erzeugt einen Enable-Puls
static void lcd_enable( void )
{
    LCD_RSEN_PORT |= (1<<LCD_EN);     // Enable auf 1 setzen
    _delay_us( LCD_ENABLE_US );  // kurze Pause
    LCD_RSEN_PORT &= (uint8_t)~(1<<LCD_EN);    // Enable auf 0 setzen
}
 
////////////////////////////////////////////////////////////////////////////////
// Sendet eine 4-bit Ausgabeoperation an das LCD
static void lcd_out( uint8_t data )
{
    data &= 0xF0;                       // obere 4 Bit maskieren
 
    LCD_PORT &= (uint8_t)~(0xF0>>(4-LCD_DB));    // Maske l�schen
    LCD_PORT |= (data>>(4-LCD_DB));     // Bits setzen
    lcd_enable();
}
 
////////////////////////////////////////////////////////////////////////////////
// Initialisierung: muss ganz am Anfang des Programms aufgerufen werden.
void lcd_init( void )
{
    // verwendete Pins auf Ausgang schalten
    uint8_t pins = (0x0F << LCD_DB);           // 4 Datenleitungen
	
	uint8_t rsen_pins = (1<<LCD_RS) |                // R/S Leitung
						(1<<LCD_EN);                 // Enable Leitung
    LCD_DDR |= pins;
	
	LCD_RSEN_DDR |= rsen_pins;
 
    // initial alle Ausg�nge auf Null
    LCD_PORT &= (uint8_t)~pins;
	
	LCD_RSEN_PORT &= (uint8_t)~rsen_pins;
 
    // warten auf die Bereitschaft des LCD
    _delay_ms( LCD_BOOTUP_MS );
    
    // Soft-Reset muss 3mal hintereinander gesendet werden zur Initialisierung
    lcd_out( LCD_SOFT_RESET );
    _delay_ms( LCD_SOFT_RESET_MS1 );
 
    lcd_enable();
    _delay_ms( LCD_SOFT_RESET_MS2 );
 
    lcd_enable();
    _delay_ms( LCD_SOFT_RESET_MS3 );
 
    // 4-bit Modus aktivieren 
    lcd_out( LCD_SET_FUNCTION |
             LCD_FUNCTION_4BIT );
    _delay_ms( LCD_SET_4BITMODE_MS );
 
    // 4-bit Modus / 2 Zeilen / 5x7
    lcd_command( LCD_SET_FUNCTION |
                 LCD_FUNCTION_4BIT |
                 LCD_FUNCTION_2LINE |
                 LCD_FUNCTION_5X7 |
				 LCD_FUNCTION_RUSSIAN);
 
    // Display ein / Cursor aus / Blinken aus
    lcd_command( LCD_SET_DISPLAY |
                 LCD_DISPLAY_ON |
                 LCD_CURSOR_OFF |
                 LCD_BLINKING_OFF); 
 
    // Cursor inkrement / kein Scrollen
    lcd_command( LCD_SET_ENTRY |
                 LCD_ENTRY_INCREASE |
                 LCD_ENTRY_NOSHIFT );
 
	
    lcd_clear();
}
  
////////////////////////////////////////////////////////////////////////////////
// Sendet ein Datenbyte an das LCD
void lcd_data( uint8_t data )
{
    LCD_RSEN_PORT |= (1<<LCD_RS);    // RS auf 1 setzen
 
    lcd_out( data );            // zuerst die oberen, 
    lcd_out( (uint8_t)(data<<4) );         // dann die unteren 4 Bit senden
 
    _delay_us( LCD_WRITEDATA_US );
}
 
////////////////////////////////////////////////////////////////////////////////
// Sendet einen Befehl an das LCD
void lcd_command( uint8_t data )
{
    LCD_RSEN_PORT &= (uint8_t)~(1<<LCD_RS);    // RS auf 0 setzen
 
    lcd_out( data );             // zuerst die oberen, 
    lcd_out( (uint8_t)(data<<4) );           // dann die unteren 4 Bit senden
 
    _delay_us( LCD_COMMAND_US );
}
//////////////////////////////////////////////////////////////////////////////// 
void lcd_turnOn(uint8_t isOn, uint8_t cursorOn)
{
	
	    // Display ein / Cursor aus / Blinken aus
    lcd_command( (uint8_t)(LCD_SET_DISPLAY |
                 (LCD_DISPLAY_ON * isOn) |
                 LCD_CURSOR_ON * cursorOn |
                 LCD_BLINKING_OFF));
}

////////////////////////////////////////////////////////////////////////////////
// Sendet den Befehl zur L�schung des Displays
void lcd_clear( void )
{
    lcd_command( LCD_CLEAR_DISPLAY );
    _delay_ms( LCD_CLEAR_DISPLAY_MS );
}
 
////////////////////////////////////////////////////////////////////////////////
// Sendet den Befehl: Cursor Home
void lcd_home( void )
{
    lcd_command( LCD_CURSOR_HOME );
    _delay_ms( LCD_CURSOR_HOME_MS );
}
 
////////////////////////////////////////////////////////////////////////////////
// Setzt den Cursor in Spalte x (0..15) Zeile y (1..4) 
 
void lcd_setcursor( uint8_t x, uint8_t y )
{
    uint8_t data;
 
    switch (y)
    {
        case 1:    // 1. Zeile
            data = (uint8_t)(LCD_SET_DDADR + LCD_DDADR_LINE1 + x);
            break;
 
        case 2:    // 2. Zeile
            data = (uint8_t)(LCD_SET_DDADR + LCD_DDADR_LINE2 + x);
            break;
 
        case 3:    // 3. Zeile
            data = (uint8_t)(LCD_SET_DDADR + LCD_DDADR_LINE3 + x);
            break;
 
        case 4:    // 4. Zeile
            data = (uint8_t)(LCD_SET_DDADR + LCD_DDADR_LINE4 + x);
            break;
 
        default:
            return;                                   // f�r den Fall einer falschen Zeile
    }
 
    lcd_command( data );
}
 
////////////////////////////////////////////////////////////////////////////////
// Schreibt einen String auf das LCD
 
void lcd_string( const char *data )
{
    while( *data != '\0' )
        lcd_data( *data++ );
}

////////////////////////////////////////////////////////////////////////////////
void lcd_string_F( const char *data )
{
  while (pgm_read_byte(&(*data))!= '\0')
   lcd_data(pgm_read_byte(&(*data++)));
};
 
////////////////////////////////////////////////////////////////////////////////
// Schreibt ein Zeichen in den Character Generator RAM
 
void lcd_generatechar( uint8_t code, const uint8_t *data )
{
    // Startposition des Zeichens einstellen
    lcd_command( LCD_SET_CGADR | (uint8_t)(code<<3) );
 
    // Bitmuster �bertragen
    for ( uint8_t i=0; i<8; i++ )
    {
        lcd_data( data[i] );
    }
}
