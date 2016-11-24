// screen is 160x128

#include <stdio.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <Time.h>
#include <TimeAlarms.h>

#define TFT_CS          10
#define TFT_RST         9
#define TFT_DC          8

#define Black           0x0000      /*   0,   0,   0 */
#define Navy            0x000F      /*   0,   0, 128 */
#define DarkGreen       0x03E0      /*   0, 128,   0 */
#define DarkCyan        0x03EF      /*   0, 128, 128 */
#define Maroon          0x7800      /* 128,   0,   0 */
#define Purple          0x780F      /* 128,   0, 128 */
#define Olive           0x7BE0      /* 128, 128,   0 */
#define LightGrey       0xC618      /* 192, 192, 192 */
#define DarkGrey        0x7BEF      /* 128, 128, 128 */
#define Blue            0x001F      /*   0,   0, 255 */
#define Green           0x07E0      /*   0, 255,   0 */
#define Cyan            0x07FF      /*   0, 255, 255 */
#define Red             0xF800      /* 255,   0,   0 */
#define Magenta         0xF81F      /* 255,   0, 255 */
#define Yellow          0xFFE0      /* 255, 255,   0 */
#define White           0xFFFF      /* 255, 255, 255 */
#define Orange          0xFD20      /* 255, 165,   0 */
#define GreenYellow     0xAFE5      /* 173, 255,  47 */
#define Pink            0xF81F

#define bgColour        Black
#define textColour      White
#define textColourLight DarkGrey
#define STATIC_TEXT_COLOUR LightGrey
#define darkLineColour  DarkGrey
#define GRAPH_COLOUR    Green
#define AXIS_COLOUR     DarkGrey
#define featureColour2  Magenta
#define featureColour3  DarkGreen

#define SCREEN_WIDTH  160
#define SCREEN_HEIGHT 128
#define BORDER        10
#define GRAPH_SAMPLES (SCREEN_WIDTH - 2*BORDER)
#define GRAPH_HEIGHT  40
#define GRAPH_WIDTH   GRAPH_SAMPLES

#define FRIDGE_PIN    20
#define FRIDGE_OFF    0
#define FRIDGE_ON     1
#define FRIDGE_SETPOINT      19.0
#define FRIDGE_MIN_TEMP      18.0
#define FRIDGE_MAX_TEMP      20.0

#define TEMP_SENS_CAL_OFFSET -13.1
#define KELVIN               273.15

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST); 

int sensorPin = 18;

int arrayFull = 0;
int loopCount = 0;

int testArray[GRAPH_SAMPLES];
float max = -999;
float min = 999;
float currentTemp = 0;
float average = 0;
int fridge = FRIDGE_OFF;

void setup(void){
  Serial.begin(9600);
  
  //set the clock
  setTime(8,29,0,1,1,11);

  pinMode(FRIDGE_PIN, OUTPUT);
  
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  tft.setTextColor(textColour, bgColour);
  tft.fillScreen(bgColour);
  tft.setRotation(1);

  int i=0;
  while(i<GRAPH_SAMPLES){
    testArray[i]=0;
    i++;
  }
  
  recordTemp();
  tftGraph();
  // dont record samples yet sinceit screws up the min/max

  // set the timers
  Alarm.timerRepeat(1, recordTemp);
  Alarm.timerRepeat(1, tftDynamicText);
  Alarm.timerRepeat(1, manageFridge);

//  Alarm.timerRepeat(617, recordSample);
//  Alarm.timerRepeat(617, tftGraph);
  Alarm.timerRepeat(1, recordSample);
  Alarm.timerRepeat(1, tftGraph);
  
}

void loop(void){
  Alarm.delay(100);
}

void recordTemp(){
  int sum = 0;
  int i=0;
  while(i < 50){
    sum += analogRead(sensorPin);
    i++;
  }
  float rawTemp = sum / 50.0;
  currentTemp = (rawTemp * 0.4882812) - KELVIN + TEMP_SENS_CAL_OFFSET;
//  Serial.print("rawTemp "); Serial.println(rawTemp);
//  Serial.print("currentTemp "); Serial.println(currentTemp);
}

void manageFridge(){
  if(currentTemp >= FRIDGE_MAX_TEMP){
    fridge = FRIDGE_ON;
    digitalWrite(FRIDGE_PIN, HIGH);
  }

  if(currentTemp <= FRIDGE_MIN_TEMP){
    fridge = FRIDGE_OFF;
    digitalWrite(FRIDGE_PIN, LOW);
  }
}

void recordSample(){
  if(loopCount>GRAPH_SAMPLES){
    arrayFull = 1;
  }
  
  // shift array over by 1
  // also find largest value
  int i=GRAPH_SAMPLES-1;
  int sum = 0;
  
  while(i>0){
    if(i < loopCount && testArray[i] > max){
      max = testArray[i];
    }

    if(i < loopCount && testArray[i] < min){
      min = testArray[i];
    }
    
    testArray[i] = testArray[i-1];

    sum = sum + testArray[i];
    
    i--;
  }

  // record latest sample
  testArray[0] = currentTemp;

  // do averaging
  sum = sum + testArray[0];
  if(!arrayFull && loopCount <= 0){
    average = sum;
  } else if(!arrayFull && loopCount > 0){
    average = (float)sum/(float)(loopCount+1);
  } else {
    average = (float)sum/(float)GRAPH_SAMPLES;
  }

  // recheck min and max
  if(currentTemp > max){
    max = currentTemp;
  }
  if(currentTemp < min){
    min = currentTemp;
  }
  loopCount++;
//  Serial.print("min ");Serial.println(min);
//  Serial.print("max ");Serial.println(max);
}

void tftDynamicText(){
  min = rangeCheck(min, 0.0, 99.9);
  max = rangeCheck(max, 0.0, 99.9);
  currentTemp = rangeCheck(currentTemp, 0.0, 99.9);

  // current temp readout
  tft.setCursor(BORDER, 5);
  tft.setTextSize(0);
  tft.setTextColor(STATIC_TEXT_COLOUR, bgColour);
  tft.println("current temp:");

  tft.setCursor(BORDER, 18);
  tft.setTextSize(5);
  tft.setTextColor(textColour, bgColour);
  printWithPadding(currentTemp);

  // degrees c
  tft.setCursor(130, 16);
  tft.setTextSize(0);
  tft.setTextColor(STATIC_TEXT_COLOUR, bgColour);
  tft.print("o");
  tft.setCursor(138, 18);
  tft.setTextSize(2);
  tft.print("C");

  // min
  tft.setCursor(BORDER, 58);
  tft.setTextSize(0);
  tft.print("min: ");
  tft.setTextColor(textColour, bgColour);
  printWithPadding(min);

  // max
  tft.setCursor(80, 58);
  tft.setTextSize(0);
  tft.setTextColor(STATIC_TEXT_COLOUR, bgColour);
  tft.print("max: ");
  tft.setTextColor(textColour, bgColour);
  printWithPadding(max);

  // average
  tft.setCursor(BORDER, 68);
  tft.setTextSize(0);
  tft.setTextColor(STATIC_TEXT_COLOUR, bgColour);
  tft.print("avg: ");
  tft.setTextColor(textColour, bgColour);
  printWithPadding(average);

  // compressor state
  tft.setCursor(80, 68);
  tft.setTextSize(0);
  tft.setTextColor(STATIC_TEXT_COLOUR, bgColour);
  tft.print("fridge: ");
  
  if(fridge == FRIDGE_ON){
    tft.setTextColor(Red, bgColour);
    tft.print("on ");
  } else {
    tft.setTextColor(Green, bgColour);
    tft.print("off");
  }
}

void tftGraph(){
  int i=0;
  int normalised[GRAPH_SAMPLES];

  int graphMax = max;
  if(graphMax < FRIDGE_SETPOINT){
    graphMax = FRIDGE_SETPOINT;
  }

  while(i<GRAPH_SAMPLES){
    // normalise
    if(testArray[i] == graphMax){
      normalised[i] = GRAPH_HEIGHT;
    } else {
      normalised[i] = (testArray[i] * GRAPH_HEIGHT) / graphMax;
    }
    if(normalised[i] < 0){
      normalised[i] = 0;
    }
    
    i++;
  }
  
  i=0;
  while(i<GRAPH_SAMPLES-2){
    // clear this vertial segment of the graph
    tft.drawFastVLine(BORDER + i, SCREEN_HEIGHT-GRAPH_HEIGHT-BORDER, GRAPH_HEIGHT+1, bgColour);

    
    int dist = normalised[i] - normalised[i+1];
    
    // plot
    if(dist == 0 && i < loopCount-1){
      tft.drawFastVLine(BORDER + i, SCREEN_HEIGHT - normalised[i] - BORDER, 1, GRAPH_COLOUR);
    } else if(dist > 0 && i < loopCount-1){
      tft.drawFastVLine(BORDER + i, SCREEN_HEIGHT - normalised[i] - BORDER, dist, GRAPH_COLOUR);
    } else if(dist < 0 && i < loopCount-1){
      tft.drawFastVLine(BORDER + i, SCREEN_HEIGHT - normalised[i] - BORDER + dist, -dist, GRAPH_COLOUR);
    }
    i++;
  }

  float targetLineHeight = GRAPH_HEIGHT*(FRIDGE_SETPOINT/(float)graphMax);
  float averageLineHeight = GRAPH_HEIGHT*(average/(float)graphMax);
  
  // target temp line
  tft.drawFastHLine(BORDER, SCREEN_HEIGHT-BORDER - targetLineHeight, GRAPH_WIDTH-2, darkLineColour);

  // average line
  if(loopCount < GRAPH_SAMPLES-2){
    tft.drawFastHLine(BORDER, SCREEN_HEIGHT-BORDER - averageLineHeight, loopCount, Cyan);
  } else {
    tft.drawFastHLine(BORDER, SCREEN_HEIGHT-BORDER - averageLineHeight, GRAPH_WIDTH-2, Cyan);
  }
}

float rangeCheck(float val, float minimum, float maximum){
    if(val < minimum){
      val = minimum;
    }

    if(val > maximum){
      val = maximum;
    }

    return val;
}

void printWithPadding(float val){
  if(val < 9.95){
    tft.print(" ");
  }
  
  tft.print(val, 1);
}
