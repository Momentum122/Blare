#include <Adafruit_GFX.h> 
#include <Adafruit_ST7789.h> 
#include <SPI.h> 

#define TFT_SCLK D9  
#define TFT_MOSI D10 
#define TFT_RST  D8  
#define TFT_DC   D4  
#define TFT_CS   D5  
#define TFT_BL   D6  

#define BUZZER_PIN D7 

#define BTN_MODE D0   
#define BTN_UP   D1   
#define BTN_DOWN D2   
#define BTN_OK   D3   

class MyST7789 : public Adafruit_ST7789 {
public:
  MyST7789(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst)
    : Adafruit_ST7789(cs, dc, mosi, sclk, rst) {}
  void setOffsets(uint8_t col, uint8_t row) {
    _colstart = _colstart2 = col;
    _rowstart = _rowstart2 = row;
  }
};

MyST7789 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

int hours = 7, minutes = 29, seconds = 50; 
int alarmHours = 6, alarmMinutes = 0; 
bool alarmEnabled = true;
bool alarmTriggered = false;

unsigned long lastSecondTick = 0;
unsigned long lastBuzzerToggle = 0;
bool buzzerState = false;

enum ClockMode { NORMAL, SET_ALM_HR, SET_ALM_MIN };
ClockMode currentMode = NORMAL;

bool isButtonPressed(int pin) {
  if (digitalRead(pin) == LOW) {
    delay(50); 
    while(digitalRead(pin) == LOW) {
      delay(10); 
    }
    return true;
  }
  return false;
}

void updateStatusText() {
  tft.setTextSize(2); 
  tft.setCursor(22, 54);
  
  if (alarmTriggered) {
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.print("!! TIME TO WAKE UP !!");
  } else if (currentMode == SET_ALM_HR) {
    tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    tft.print("Set Alarm Hour...    ");
  } else if (currentMode == SET_ALM_MIN) {
    tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    tft.print("Set Alarm Minute...  ");
  } else if (alarmEnabled) {
    tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK);
    char buf[22];
    snprintf(buf, sizeof(buf), "ALARM %02d:%02d [ON]   ", alarmHours, alarmMinutes);
    tft.print(buf);
  } else {
    tft.setTextColor(0x7BEF, ST77XX_BLACK); 
    tft.print("ALARM [OFF]          ");
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(TFT_BL, OUTPUT); 
  digitalWrite(TFT_BL, HIGH); 
  
  pinMode(BUZZER_PIN, OUTPUT);
  
  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);

  tft.init(76, 284); 
  tft.setOffsets(82, 18); 
  tft.invertDisplay(false);
  tft.setRotation(1); 
  
  tft.fillScreen(ST77XX_BLACK); 
  updateStatusText();
  
  Serial.println("System Booted. Ready.");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastSecondTick >= 1000) {
    lastSecondTick += 1000; 
    seconds++;
    
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours = (hours + 1) % 24;
      }
      
      if (alarmEnabled && hours == alarmHours && minutes == alarmMinutes && seconds == 0) {
        alarmTriggered = true;
        updateStatusText();
      }
    }

    char timeStr[9];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", hours, minutes, seconds);
    
    tft.setTextSize(5); 
    tft.setCursor(22, 8);
    
    if (alarmTriggered) {
      tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
    } else {
      tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    }
    tft.print(timeStr);
  }

  if (alarmTriggered && (currentMillis - lastBuzzerToggle >= 250)) {
    lastBuzzerToggle = currentMillis;
    buzzerState = !buzzerState;
    digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
    
    tft.setTextSize(2);
    tft.setCursor(22, 54);
    if (buzzerState) {
      tft.setTextColor(ST77XX_BLACK, ST77XX_RED);
    } else {
      tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    }
    tft.print("!! TIME TO WAKE UP !!");
  }

  if (isButtonPressed(BTN_MODE)) {
    if (currentMode == NORMAL) currentMode = SET_ALM_HR;
    else if (currentMode == SET_ALM_HR) currentMode = SET_ALM_MIN;
    else currentMode = NORMAL;
    
    updateStatusText();
  }

  if (isButtonPressed(BTN_UP)) {
    if (currentMode == SET_ALM_HR) alarmHours = (alarmHours + 1) % 24;
    else if (currentMode == SET_ALM_MIN) alarmMinutes = (alarmMinutes + 1) % 60;
    
    if (currentMode != NORMAL) updateStatusText();
  }

  if (isButtonPressed(BTN_DOWN)) {
    if (currentMode == SET_ALM_HR) alarmHours = (alarmHours == 0) ? 23 : alarmHours - 1;
    else if (currentMode == SET_ALM_MIN) alarmMinutes = (alarmMinutes == 0) ? 59 : alarmMinutes - 1;
    
    if (currentMode != NORMAL) updateStatusText();
  }

  if (isButtonPressed(BTN_OK)) {
    if (alarmTriggered) {
      alarmTriggered = false; 
      digitalWrite(BUZZER_PIN, LOW);
    } else if (currentMode == NORMAL) {
      alarmEnabled = !alarmEnabled; 
    } else {
      currentMode = NORMAL; 
    }
    updateStatusText();
  }
}