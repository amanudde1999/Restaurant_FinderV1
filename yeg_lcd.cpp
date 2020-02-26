/*
	A simple demonstration of drawing a section of the
	large map of Edmonton (yeg-big.lcd),
	and then scrolling it over one "screen".
*/

#include <Arduino.h>

// core graphics library (written by Adafruit)
#include <Adafruit_GFX.h>

// Hardware-specific graphics library for MCU Friend 3.5" TFT LCD shield
#include <MCUFRIEND_kbv.h>

// LCD and SD card will communicate using the Serial Peripheral Interface (SPI)
// e.g., SPI is used to display images stored on the SD card
#include <SPI.h>

// needed for reading/writing to SD card
#include <SD.h>

#define SD_CS 10

#include "lcd_image.h"


MCUFRIEND_kbv tft;

#define DISPLAY_WIDTH  480
#define DISPLAY_HEIGHT 320

#define YEG_SIZE 2048

lcd_image_t yegImage = { "yeg-big.lcd", YEG_SIZE, YEG_SIZE };

void setup() {
  init();

  Serial.begin(9600);

  
  //    tft.reset();             // hardware reset
  uint16_t ID = tft.readID();    // read ID from display
  Serial.print("ID = 0x");
  Serial.println(ID, HEX);
  if (ID == 0xD3D3) ID = 0x9481; // write-only shield
  
  // must come before SD.begin() ...
  tft.begin(ID);                 // LCD gets ready to work
    
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.print("failed! Is it inserted properly?");
    while (true) {}
  }
  Serial.println("OK!");

  tft.setRotation(1);

  // black is 0, i.e. 0 == tft.color565(0, 0, 0)

  // fill with black
  tft.fillScreen(tft.color565(0, 0, 0));
}

int main() {
  setup();

  int yegMiddleX = YEG_SIZE/2 - DISPLAY_WIDTH/2;
  int yegMiddleY = YEG_SIZE/2 - DISPLAY_HEIGHT/2;

  // &yegImage is a "pointer" to the location (in memory) of yegImage
  lcd_image_draw(&yegImage, &tft,
		 // upper left corner in the image to draw
		 yegMiddleX, yegMiddleY,
		 // upper left corner of the screen to draw it in
		 0, 0,
		 // width and height of the patch of the image to draw
		 DISPLAY_WIDTH, DISPLAY_HEIGHT);
  
  Serial.end();
  return 0;
}