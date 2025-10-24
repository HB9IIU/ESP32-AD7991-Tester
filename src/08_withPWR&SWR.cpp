#include <Wire.h>
#include <Arduino.h>
#include <TFT_eSPI.h> // Bodmer's TFT display library
#include <SPI.h>
#include <Preferences.h> // For saving calibration data

#define SDA_PIN 21
#define SCL_PIN 22
const uint8_t AD_ADDR = 0x29;  // from your scan
const float VREF_VOLTS = 2.6f; // external ref on VIN3

// CH1 + CH0 + external ref (VIN3). Keep your value:
const uint8_t CFG = 0x38;

// how many samples to average per channel
const uint8_t AVG_N = 16;

int leftMargin = 210;
int leftMargin2 = 310;
int yLine1 = 35;
int yLine2 = 90;
int yLine3 = 160;

Preferences preferences;
TFT_eSPI tft = TFT_eSPI(); // TFT instance

// Prototypes
void checkAndApplyTFTCalibrationData(bool recalibrate);
void calibrateTFTscreen();
bool readSample(uint8_t &chid, uint16_t &raw12);

void setup()
{
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  Wire.beginTransmission(AD_ADDR);
  Wire.write(CFG);
  if (Wire.endTransmission() != 0)
  {
    Serial.println("AD7991 config failed. Check wiring/address.");
    while (true)
      delay(1000);
  }
  Serial.println("AD7991: CH1+CH0, ext ref (2.6V).");
  delay(5);

  // --- Backlight control sequence ---
  pinMode(TFT_BLP, OUTPUT);
  digitalWrite(TFT_BLP, HIGH); // Leave ON

  // --- TFT setup ---
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  // --- Touch calibration (auto unless forced) ---
  checkAndApplyTFTCalibrationData(false); // set to true to force recalibration

  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);

  tft.setTextDatum(BL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.drawCentreString("AD7991 Calibration Tool", 160, 0, 2);

  tft.drawString("Ch.", 20, yLine1 + 22, 1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Ch.", 20, yLine1 + 22, 1);
  tft.drawString("00", 24, yLine1 + 48, 1);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Ch.", 20, yLine2 + 22, 1);
  tft.drawString("01", 24, yLine2 + 48, 1);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawRightString(String(8888), leftMargin, yLine1, 7); // right edge at x=300
  tft.drawRightString(String(8888), leftMargin, yLine2, 7); // right edge at x=300
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawRightString(String("8.88 V"), leftMargin2, yLine1 + 32, 1); // right edge at x=300
  tft.drawRightString(String("8.88 V"), leftMargin2, yLine2 + 32, 1); // right edge at x=300
  tft.setTextSize(1);

  Serial.println("📟 TFT + Touch test ready");
}

void loop()
{
  // accumulate AVG_N samples per channel
  uint32_t sum0 = 0, sum1 = 0;
  uint8_t n0 = 0, n1 = 0;

  // simple cap to avoid endless loop if reads fail
  for (int tries = 0; tries < 64 && (n0 < AVG_N || n1 < AVG_N); ++tries)
  {
    uint8_t ch;
    uint16_t v;
    if (!readSample(ch, v))
      continue;
    if (ch == 0 && n0 < AVG_N)
    {
      sum0 += v;
      ++n0;
    }
    if (ch == 1 && n1 < AVG_N)
    {
      sum1 += v;
      ++n1;
    }
  }
  // n0 = 1;
  if (n0 > 0)
  {
    uint16_t avg0 = (uint16_t)(sum0 / n0);
    float v0 = (VREF_VOLTS * (float)avg0) / 4096.0f;
    Serial.printf("CH0 avg of %u: %4u -> %.6f V   ", n0, avg0, v0);
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawRightString(String(8888), leftMargin, yLine1, 7);
    tft.setTextSize(2);
    tft.drawRightString(String("8.88"), leftMargin2 - 24, yLine1 + 32, 1);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawRightString(String(avg0), leftMargin, yLine1, 7);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawRightString(String(v0), leftMargin2 - 24, yLine1 + 32, 1);
    tft.setTextSize(1);
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.drawRightString(String(-8888), leftMargin - 60, yLine3, 7);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    //y = 2.6693x - 8210.9
    float mW0 = 2.6693f * (float)avg0 - 8210.9f; // RAW -> mW
    tft.drawRightString(String(mW0, 0), leftMargin - 60, yLine3, 7);
  }
  else
  {
    Serial.print("CH0: n/a   ");
  }
  // n1 = 1;
  if (n1 > 0)
  {
    uint16_t avg1 = (uint16_t)(sum1 / n1);
    float v1 = (VREF_VOLTS * (float)avg1) / 4096.0f;
    Serial.printf("CH1 avg of %u: %4u -> %.6f V\n", n1, avg1, v1);
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawRightString(String(8888), leftMargin, yLine2, 7);
    tft.setTextSize(2);
    tft.drawRightString(String("8.88"), leftMargin2 - 24, yLine2 + 32, 1);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawRightString(String(avg1), leftMargin, yLine2, 7);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawRightString(String(v1), leftMargin2 - 24, yLine2 + 32, 1);
    tft.setTextSize(1);
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.drawRightString(String(-8888), 320 - 10, yLine3, 7);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    // y = 2.948x - 9020.5
    float mW1 = 2.948f * (float)avg1 - 9020.5f; // RAW -> mW
    tft.drawRightString(String(mW1, 0), 320 - 10, yLine3, 7);
  }
  else
  {
    Serial.println("CH1: n/a");
  }

  delay(200);
}

bool readSample(uint8_t &chid, uint16_t &raw12)
{
  if (Wire.requestFrom(AD_ADDR, (uint8_t)2) != 2)
    return false;
  uint8_t msb = Wire.read(), lsb = Wire.read();
  uint16_t w = ((uint16_t)msb << 8) | lsb;
  chid = (w & 0x3000) >> 12; // 0..3
  raw12 = (w & 0x0FFF);      // 12-bit
  return true;
}

void checkAndApplyTFTCalibrationData(bool recalibrate)
{
  if (recalibrate)
  {
    calibrateTFTscreen();
    return;
  }

  uint16_t calibrationData[5];
  preferences.begin("TFT", true);
  bool dataValid = true;

  for (int i = 0; i < 5; i++)
  {
    calibrationData[i] = preferences.getUInt(("calib" + String(i)).c_str(), 0xFFFF);
    if (calibrationData[i] == 0xFFFF)
    {
      dataValid = false;
    }
  }
  preferences.end();

  if (dataValid)
  {
    tft.setTouch(calibrationData);
    Serial.println("Calibration data applied.");
  }
  else
  {
    Serial.println("Invalid calibration data. Recalibrating...");
    calibrateTFTscreen();
  }
}

void calibrateTFTscreen()
{
  uint16_t calibrationData[5];

  // Display recalibration message
  tft.fillScreen(TFT_BLACK);             // Clear screen
  tft.setTextColor(TFT_BLACK, TFT_GOLD); // Set text color (black on gold)
  tft.setFreeFont(&FreeSansBold9pt7b);   // Use custom free font
  tft.setTextSize(1);
  tft.setCursor(5, 22);
  tft.fillRect(0, 0, tft.width(), 30, TFT_GOLD);
  tft.print("TFT TOUCHSCREEN CALIBRATION");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.setFreeFont(&FreeSans9pt7b);

  String instructions[] = {
      "Initial touchscreen calibration",
      "Procedure will only be required once",
      "On the next screen you will see arrows",
      "appearing at each corner",
      "Just tap them until completion.",
      "",
      "Tap anywhere on the screen to begin"};

  int16_t yPos = 70;

  for (String line : instructions)
  {
    int16_t xPos = (tft.width() - tft.textWidth(line)) / 2;
    tft.setCursor(xPos, yPos);
    tft.print(line);
    yPos += tft.fontHeight();
  }

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor((tft.width() - tft.textWidth(instructions[6])) / 2, yPos - tft.fontHeight());
  tft.print(instructions[6]);

  while (true)
  {
    uint16_t x, y;
    if (tft.getTouch(&x, &y))
    {
      break;
    }
  }

  tft.fillScreen(TFT_BLACK);

  tft.calibrateTouch(calibrationData, TFT_GREEN, TFT_BLACK, 12);

  preferences.begin("TFT", false);
  for (int i = 0; i < 5; i++)
  {
    preferences.putUInt(("calib" + String(i)).c_str(), calibrationData[i]);
  }
  preferences.end();

  // Test calibration with dynamic feedback
  tft.fillScreen(TFT_BLACK);
  uint16_t x, y;
  int16_t lastX = -1, lastY = -1;
  String lastResult = "";

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(4);
  String text = "Tap anywhere to test";
  tft.setCursor((tft.width() - tft.textWidth(text)) / 2, 8);
  tft.print(text);

  text = "Click Here to Exit";
  int16_t btn_w = tft.textWidth(text) + 20;
  int16_t btn_h = tft.fontHeight() + 12;
  int16_t btn_x = (tft.width() - btn_w) / 2;
  int16_t btn_y = 200;
  int16_t btn_r = 10;

  tft.fillRoundRect(btn_x, btn_y, btn_w, btn_h, btn_r, TFT_BLUE);
  tft.drawRoundRect(btn_x, btn_y, btn_w, btn_h, btn_r, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setCursor((tft.width() - tft.textWidth(text)) / 2, btn_y + 10);
  tft.print(text);

  while (true)
  {
    if (tft.getTouch(&x, &y))
    {
      String result = "x=" + String(x) + "  y=" + String(y) + "  p=" + String(tft.getTouchRawZ());

      tft.setTextColor(TFT_BLACK, TFT_BLACK);
      tft.setCursor(lastX, lastY);
      tft.print(lastResult);

      int16_t textWidth = tft.textWidth(result);
      int16_t xPos = (tft.width() - textWidth) / 2;
      int16_t yPos = (tft.height() - tft.fontHeight()) / 2;

      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setCursor(xPos, yPos);
      tft.print(result);

      lastX = xPos;
      lastY = yPos;
      lastResult = result;

      tft.fillCircle(x, y, 2, TFT_RED);

      if (x > btn_x && x < btn_x + btn_w && y > btn_y && y < btn_y + btn_h)
      {
        Serial.println("Exit Touched");
        break;
      }
    }
  }
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
}
