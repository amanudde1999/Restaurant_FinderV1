//------------------------------------------------------
// Names: Amro Amanuddein and Pushkar Sabharwal
// ID's: 1572498 (Amro) 1588927 (Pushkar)
// CMPUT275, Winter 2020
//
// Assignment 1 Part 1: Restaurant Finder
//------------------------------------------------------

// Inititalzing all required libraries
#define SD_CS 10
#define JOY_VERT  A9 // should connect A9 to pin VRx
#define JOY_HORIZ A8 // should connect A8 to pin VRy
#define JOY_SEL   53
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <SPI.h>
#include <SD.h>
#include "lcd_image.h"
#include <math.h>
#include <TouchScreen.h>
MCUFRIEND_kbv tft;

// Defining al given constants
#define REST_START_BLOCK 4000000
#define MAP_WIDTH 2048
#define MAP_HEIGHT 2048
#define LAT_NORTH 5361858l
#define LAT_SOUTH 5340953l
#define LON_WEST -11368652l
#define LON_EAST -11333496l

#define DISPLAY_WIDTH  480
#define DISPLAY_HEIGHT 320
#define BLUE 0x000F
#define YEG_SIZE 2048
#define YP A3
#define XM A2
#define YM 9
#define XP 8
#define TS_MINX 100
#define TS_MINY 120
#define TS_MAXX 940
#define TS_MAXY 920
#define MINPRESSURE 10
#define MAXPRESSURE 1000
#define NUM_LINES 21

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

lcd_image_t yegImage = { "yeg-big.lcd", YEG_SIZE, YEG_SIZE };
#define JOY_CENTER   512
#define JOY_DEADZONE 64

#define CURSOR_SIZE 9
#define NUM_RESTAURANTS 1066

// the cursor position on the display
int cursorX, cursorY;

Sd2Card card;
// forward declaration for redrawing the cursor
void redrawCursor(uint16_t colour);

void setup() {
  init();

  Serial.begin(9600);

  pinMode(JOY_SEL, INPUT_PULLUP);

  //    tft.reset();             // hardware reset
  uint16_t ID = tft.readID();    // read ID from display
  Serial.print("ID = 0x");
  Serial.println(ID, HEX);
  if (ID == 0xD3D3) ID = 0x9481; // write-only shield

  // must come before SD.begin() ...
  tft.begin(ID);                 // LCD gets ready to work

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed! Is it inserted properly?");
    while (true) {}
  }
  Serial.println("OK!");
  Serial.print("Initializing SPI communication for raw reads...");
  if (!card.init(SPI_HALF_SPEED, SD_CS)){
    Serial.println("failed! Is the card inserted properly?");
    while(true){}
  }
  Serial.println("OK!");

  tft.setRotation(1);
  // will fill the screen black before loading the map
  tft.fillScreen(TFT_BLACK);

  // draws the centre of the Edmonton map, leaving the rightmost 60 columns black
  int yegMiddleX = YEG_SIZE/2 - (DISPLAY_WIDTH - 60)/2;
  int yegMiddleY = YEG_SIZE/2 - DISPLAY_HEIGHT/2;
  lcd_image_draw(&yegImage, &tft, yegMiddleX, yegMiddleY,
                 0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);

  // initial cursor position is the middle of the screen
  // subtract 60 as the right-most column of 60 pixels is black
  //x-coordinate
  cursorX = (DISPLAY_WIDTH - 60)/2;
  //y-coordinate
  cursorY = DISPLAY_HEIGHT/2;
  redrawCursor(TFT_RED);
}


int yegMiddleX = YEG_SIZE/2 - (DISPLAY_WIDTH - 60)/2;
int yegMiddleY = YEG_SIZE/2 - DISPLAY_HEIGHT/2;
int16_t MiddleX = yegMiddleX;
int16_t MiddleY = yegMiddleY;

// Defining new struct which occupies less memory to get the restaurants with least distance
struct RestDist {
  uint16_t index;
  uint16_t dist;
};

// Struct with all the restaurant name, ratings, longitute and latitude is initialised
struct restaurant {
  int32_t lat;
  int32_t lon;
  uint8_t rating; // from 0 to 10
  char name[55];
};

// Initializing new array of type RestDist to store all restaurant distances
RestDist rest_dist[NUM_RESTAURANTS];

// A track variable to know the current highlighted string
int highlightedString;
// Function made to swap the two restauraunts depending on distance for the insertion sorting
void swap(RestDist *rest1, RestDist *rest2){
  RestDist a = *rest1;
  *rest1 = *rest2;
  *rest2 = a;
}
// Insertion sort made from the algorithm in the assignment description
void isort(int len, RestDist rest_dist[]){
  int i=1;
  while (i<len){
    int j=i;
    while (j>0 && (rest_dist[j-1].dist)>(rest_dist[j].dist)){
      swap(&rest_dist[j], &rest_dist[j-1]);
      j--;
    }
    i++;
  }
}
// Cache to store the restauraunts as we go, implemented as an improvement from Weekly 2
restaurant cacheData[8];
uint32_t cacheBlock;
void getRestaurantFast(int restIndex, restaurant *restPtr) {
  uint32_t blockNum = REST_START_BLOCK + restIndex/8;
  if (blockNum != cacheBlock) {
    while (!card.readBlock(blockNum, (uint8_t*) cacheData)) {
      Serial.println("Read block failed, trying again.");
    }

  }

  *restPtr = cacheData[restIndex % 8];
  cacheBlock = blockNum;
}

// Function to disselect the current restaurant in list if the joystick is moved
void notSelected(int index) {
  tft.setCursor(0, 16*index);
  restaurant current_rest;
  getRestaurantFast(rest_dist[index].index , &current_rest);
  tft.setTextColor (TFT_WHITE , TFT_BLACK);
  tft.setTextSize(2);
  tft.println(current_rest.name);
}

// Functio to select the current restaurant with a white color on the list
void selected(int index) {
  tft.setCursor(0, 16*index);
  restaurant current_rest;
  getRestaurantFast(rest_dist[index].index , &current_rest);
  tft.setTextColor (TFT_BLACK , TFT_WHITE);
  tft.setTextSize(2);
  tft.println(current_rest.name);
}

// Function to list the nearest 21 restaurants to the cursor
void listRestaurants() {
  tft.fillScreen(0);
  for (int16_t i = 0; i <= 21; i++) {
    tft.setCursor(0, 16*i);
    restaurant current_rest;
    getRestaurantFast(rest_dist[i].index , &current_rest);
    if (i !=  0) {
      tft.setTextColor (TFT_WHITE , TFT_BLACK);
    }
    else {
      tft.setTextColor (TFT_BLACK , TFT_WHITE);
    }
    tft.setTextSize(2);
    tft.println(current_rest.name);
  }
}

// The following 4 functions were implemented from eclass
int16_t  lon_to_x(int32_t  lon) {
  return  map(lon , LON_WEST , LON_EAST , 0, MAP_WIDTH);
}

int16_t  lat_to_y(int32_t  lat) {
  return  map(lat , LAT_NORTH , LAT_SOUTH , 0, MAP_HEIGHT);
}
int32_t x_to_lon(int16_t x){
  return map(x,0,MAP_WIDTH,LON_WEST,LON_EAST);
}
int32_t y_to_lat(int16_t y){
  return map(y,0, MAP_HEIGHT, LAT_NORTH, LAT_SOUTH);
}
// The following function is used from the tester files for the tft display
void processtouchscreen(){
  TSPoint touch = ts.getPoint();
  pinMode(YP,OUTPUT);
  pinMode(XM,OUTPUT);
  if (touch.z > MINPRESSURE && touch.z < MAXPRESSURE){
    // Co-ordinates are obtained for where the screen is touched and adjusted for the size of the map on the screen
    int16_t point_x = map(touch.x, TS_MINX, TS_MAXX,420-1,0);
    int16_t point_y = map(touch.y, TS_MINY, TS_MAXY,320-1,0);
    restaurant current_rest;
    int16_t xPos;
    int16_t yPos;
    // For loop to obtain all restauraunts on the current frame and display them as black dots on the screen
    for(int i=0; i< NUM_RESTAURANTS; i++){
      getRestaurantFast(i, &current_rest);
        xPos = lon_to_x(current_rest.lon);
        yPos = lat_to_y(current_rest.lat);
        // If statement to determine if the co-ordinates from the struct are on the frame of the current of the map
        // and if so, display the dots
        if (xPos > MiddleX && xPos < (MiddleX+420) && yPos > MiddleY && yPos < (MiddleY + 320)){
          tft.fillCircle(xPos-MiddleX,yPos-MiddleY,3.5, TFT_BLACK);
      }
    }
  }
}

// Function used which takes us to the desired restaurant when we select the restaurant
void goToRestaurant(int16_t xCoor, int16_t yCoor) {

  //Adjusting the value of MiddleX if the restaurant by checking if restaurant present on current patch
  // using yCoor and xCoor to determine CursorX
  if (xCoor < DISPLAY_WIDTH-60) {
    MiddleX = 0;
  }
  else if (xCoor > 2048 - 420) {
    MiddleX = 2048 - 420;
  }
  else {
    MiddleX = xCoor - (DISPLAY_WIDTH-60)/2;
  }

  // Case in which restaurant is outside the patch
  if (xCoor < 0) {
      cursorX = CURSOR_SIZE/2;
  }
  else if (xCoor > (DISPLAY_WIDTH-60)/2 && (xCoor < 2048 - (DISPLAY_WIDTH-60))) {
      cursorX = (DISPLAY_WIDTH - 60)/2;
  }
  else if (xCoor > 2048) {
    cursorX = 2048 - CURSOR_SIZE/2;
  }
  else {
    cursorX = xCoor;
  }

  //Adjusting the value of MiddleY if the restaurant by checking if restaurant present on current patch
  // using yCoor and xCoor to determine CursorY
  if (yCoor < DISPLAY_HEIGHT) {
    MiddleY = 0;
  }
  else if (yCoor > 2048 -320) {
    MiddleX = 2048 - 320;
  }
  else {
    MiddleY = yCoor - DISPLAY_HEIGHT/2;
  }

  // Case if the restaurant is outside the patch
  if (yCoor < 0) {
    cursorY = CURSOR_SIZE/2;
  }
  else if (yCoor > (DISPLAY_HEIGHT)/2 && (yCoor < 2048 - (DISPLAY_HEIGHT))) {
    cursorY = DISPLAY_HEIGHT/2;
  }
  else if (yCoor > 2048){
    cursorY = 2048 - CURSOR_SIZE/2;
  }
  else {
    cursorY = yCoor;
  }

  // Draw the image to the display once cursorX and cursorY have been determined
  lcd_image_draw(&yegImage, &tft, MiddleX, MiddleY, 0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);

  // Redraw the cursor at the selected restaurant
  redrawCursor(TFT_RED);

}
// The following function allows us to determine whether the cursor has gone off the patch or not for all directions
int Draw_image(int16_t x_coord, int16_t y_coord) {

  MiddleX += x_coord;
  MiddleY += y_coord;
  int flag = 1;
  // If the cursor goes off the patch upwards (north) or downwards (south)
  if (MiddleY == -320) {
    MiddleY = 0;
    flag = 0;
  }

  else if (MiddleY == 2048) {
    MiddleY = 2048-320;
    flag = 0;
  }
  else if (MiddleY > 2048-320) {
    MiddleY = 2048-320;
  }
  else if (MiddleY < 0) {
    MiddleY = 0;
  }
  // If the cursor goes off the patch left (west) or right (east)
  if (MiddleX == -420) {
    MiddleX = 0;
    flag = 0;
  }
  else if (MiddleX == 2048) {
    MiddleX = 2048-420;
    flag = 0;
  }
  else if (MiddleX > 2048-420) {
    MiddleX = 2048-420;
  }
  else if (MiddleX < 0) {
    MiddleX = 0;
  }
  // This if statement will run only if the flag is 1
  if (flag) {
    lcd_image_draw(&yegImage, &tft, MiddleX, MiddleY,
      0, 0, DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);
      return 1;
  }
    return 0;
}

// Separate mode1 function
void mode1(){
  restaurant rest4;
  for (int j=0; j< NUM_RESTAURANTS; ++j){
    getRestaurantFast(j, &rest4);
    rest_dist[j].index = j;
    int16_t mapCursorX = (MiddleX + cursorX) - lon_to_x(rest4.lon);
    int16_t mapCursorY = (MiddleY + cursorY) - lat_to_y(rest4.lat);
    rest_dist[j].dist = (abs(mapCursorX)+abs(mapCursorY));
  }
  isort(NUM_RESTAURANTS, rest_dist);

  // Function to show the list of restaurants on tft display
  listRestaurants();
  // This variable keeps track of the highlighted string
  int highlightedString =0;

  // This while loop keeps a check on when the button is pressed again
  while (digitalRead(JOY_SEL) == 1){
    // To scroll down
    if (analogRead(JOY_VERT) > JOY_CENTER + JOY_DEADZONE) {
      highlightedString+=1;
      selected(highlightedString);
      notSelected(highlightedString-1);
    }
    // To scroll up
    else if (analogRead(JOY_VERT) < JOY_CENTER - JOY_DEADZONE) {
      highlightedString-=1;
      selected(highlightedString);
      notSelected(highlightedString+1);
    }
  }

  //Once while loop is exited, xCoor, yCoor are determined using given functions. They are passed into
  // custom function goToRestaurant which determines cursorX and cursorY and takes shows it on display
  restaurant selectedRest;
  getRestaurantFast(rest_dist[highlightedString].index, &selectedRest);

  int xCoor = lon_to_x(selectedRest.lon);
  int yCoor = lat_to_y(selectedRest.lat);

  // To load that same black column on the right-most screen
  tft.fillRect(DISPLAY_WIDTH-60, 0, 60, DISPLAY_HEIGHT, TFT_BLACK);
  goToRestaurant(xCoor, yCoor);

}

void redrawCursor(uint16_t colour) {
  tft.fillRect(cursorX - CURSOR_SIZE/2, cursorY - CURSOR_SIZE/2,
               CURSOR_SIZE, CURSOR_SIZE, colour);
}

void processJoystick() {
  // Values read from the joystick movement
  int xVal = analogRead(JOY_HORIZ);
  int yVal = analogRead(JOY_VERT);
  // Joystick pressed down?
  int buttonVal = digitalRead(JOY_SEL);
  int yegMiddleX = YEG_SIZE/2 - (DISPLAY_WIDTH - 60)/2;
  int yegMiddleY = YEG_SIZE/2 - DISPLAY_HEIGHT/2;
  // This will control the speed depending on the extent of movement of the joystick
  // implemented as an improvement from Weekly 1
  int xSpeed = abs(xVal - JOY_CENTER) / 50;
  int ySpeed = abs(yVal - JOY_CENTER) / 50;

  // If you move the joystick up
  if ((yVal < JOY_CENTER - JOY_DEADZONE)) {
    // Cover up that trail!
    lcd_image_draw(&yegImage, &tft, MiddleX + cursorX - CURSOR_SIZE/2, MiddleY + cursorY - CURSOR_SIZE/2,
    cursorX - CURSOR_SIZE/2, cursorY - CURSOR_SIZE/2, CURSOR_SIZE, CURSOR_SIZE);
    // Cursor will move up
    cursorY -= ySpeed;

    if (cursorY < CURSOR_SIZE/2) {
      // New portion of the map will be drawn if you go over
      if (Draw_image(0, -DISPLAY_HEIGHT)) {
        // Recentering the cursor after drawing
        cursorX = (DISPLAY_WIDTH - 60)/2;
        cursorY = DISPLAY_HEIGHT/2;
      }
      else {
        cursorY = CURSOR_SIZE/2;
      }

    }

  }
  // If you move the joystick down
  else if ((yVal > JOY_CENTER + JOY_DEADZONE)) {

    lcd_image_draw(&yegImage, &tft, MiddleY + cursorX - CURSOR_SIZE/2, MiddleY + cursorY - CURSOR_SIZE/2,
    cursorX - CURSOR_SIZE/2, cursorY - CURSOR_SIZE/2, CURSOR_SIZE, CURSOR_SIZE);
    // Cursor moves down
    cursorY += ySpeed;

    // Moving over the map from the bottom
    if (cursorY > DISPLAY_HEIGHT - CURSOR_SIZE/2) {

      if (Draw_image(0, DISPLAY_HEIGHT)) {
        cursorX = (DISPLAY_WIDTH - 60)/2;
        cursorY = DISPLAY_HEIGHT/2;
      }
      else {
        cursorY = DISPLAY_HEIGHT - CURSOR_SIZE/2;
      }

    }

  }

  // If you move to the left
  if ((xVal > JOY_CENTER + JOY_DEADZONE)) {
    // Cover up the trail
    lcd_image_draw(&yegImage, &tft, MiddleX + cursorX - CURSOR_SIZE/2, MiddleY + cursorY - CURSOR_SIZE/2,
    cursorX - CURSOR_SIZE/2, cursorY - CURSOR_SIZE/2, CURSOR_SIZE, CURSOR_SIZE);

    cursorX -= xSpeed;

    // If you go over the current patch
    if (cursorX < CURSOR_SIZE/2) {
      // New patch is redrawn
      if (Draw_image(-(420), 0)) {
        cursorX = (420)/2;
        cursorY = DISPLAY_HEIGHT/2;
      }
      else {
        cursorX = CURSOR_SIZE/2;
      }


    }
  }
  // If you move to the rught
  else if ((xVal < JOY_CENTER - JOY_DEADZONE)) {
    // Cover up that trail!
    lcd_image_draw(&yegImage, &tft, MiddleX + cursorX - CURSOR_SIZE/2, MiddleY + cursorY - CURSOR_SIZE/2,
    cursorX - CURSOR_SIZE/2, cursorY - CURSOR_SIZE/2, CURSOR_SIZE, CURSOR_SIZE);

    cursorX += 1*xSpeed;


    if (cursorX > (420) - CURSOR_SIZE/2) {


      if (Draw_image(420, 0)) {
        cursorX = (420)/2;
        cursorY = DISPLAY_HEIGHT/2;
      } else {
        cursorX = (420) - CURSOR_SIZE/2;
      }

    }
  }
  // If the Joystick is pressed down, we now enter mode 1
  if(buttonVal == 0){
    mode1();

  }


  redrawCursor(TFT_RED);
  // If you touch part of the screen including the map, load the restauarants with black dots that are on the currrent patch
  processtouchscreen();

  delay(20);
}







int main() {
  setup();
  while (true) {
  processJoystick();

}
  Serial.end();
  return 0;

}
