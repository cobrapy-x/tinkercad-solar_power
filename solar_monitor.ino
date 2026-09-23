// ===== Solar + Base Power Monitor =====
// A0: solar voltage (divider /11)     A1: bus voltage (divider /11)
// A2: RETURN rail = solar shunt       A3: point LB = bulb shunt
// Two I2C LCDs (0x20 top, 0x21 bottom) = 16x4 screen

#include <Adafruit_LiquidCrystal.h>

Adafruit_LiquidCrystal lcdTop(0);     // address 0x20
Adafruit_LiquidCrystal lcdBottom(1);  // address 0x21

// ----- Pins -----
const int SOLAR_V_PIN     = A0;
const int BUS_V_PIN       = A1;
const int SOLAR_SHUNT_PIN = A2;
const int LOAD_SHUNT_PIN  = A3;

// ----- Circuit values -----
const float DIVIDER    = 11.0;  // (100k + 10k) / 10k
const float SHUNT_OHMS = 0.2;   // both shunt resistors

// Read a pin in volts (0-5 V)
float readPin(int pin) {
  return analogRead(pin) * 5.0 / 1023.0;
}

// Write 16 characters to one row of the 16x4 screen (rows 0-3)
void putRow(int row, const char* text) {
  if (row < 2) {
    lcdTop.setCursor(0, row);
    lcdTop.print(text);
  } else {
    lcdBottom.setCursor(0, row - 2);
    lcdBottom.print(text);
  }
}

void setup() {
  lcdTop.begin(16, 2);
  lcdBottom.begin(16, 2);
  lcdTop.setBacklight(1);
  lcdBottom.setBacklight(1);

  putRow(0, "  SOLAR + BASE  ");
  putRow(1, " POWER  MONITOR ");
  putRow(2, "                ");
  putRow(3, "  Starting...   ");
  delay(1500);

  Serial.begin(9600);
}

void loop() {
  // ----- 1) Measure -----
  float solarV = readPin(SOLAR_V_PIN) * DIVIDER;  // solar panel voltage
  float busV   = readPin(BUS_V_PIN) * DIVIDER;    // bus voltage (vs GND)
  float vReturn = readPin(SOLAR_SHUNT_PIN);       // voltage across solar shunt
  float vLB     = readPin(LOAD_SHUNT_PIN);        // RETURN + bulb shunt voltage

  // ----- 2) Calculate -----
  float solarI = vReturn / SHUNT_OHMS;            // solar current
  float loadI  = (vLB - vReturn) / SHUNT_OHMS;    // bulb current
  if (loadI < 0) loadI = 0;

  float baseI = loadI - solarI;                   // base supply fills the rest
  if (baseI < 0) baseI = 0;

  float bulbV = busV - vLB;                       // voltage across the bulb
  if (bulbV < 0) bulbV = 0;
  float bulbW = bulbV * loadI;                    // bulb power

  int share = 0;                                  // % of bulb power from solar
  if (loadI > 0.05) share = solarI / loadI * 100;
  share = constrain(share, 0, 100);

  const char* mode;
  if (loadI < 0.05)      mode = "OFF  ";
  else if (share >= 95)  mode = "SOLAR";
  else if (share <= 5)   mode = "BASE ";
  else                   mode = "SHARE";

  // ----- 3) Display on the 16x4 screen -----
  char line[17];
  char a[8];
  char b[8];

  // Row 0: SOL  39.2V 4.10A
  dtostrf(solarV, 4, 1, a);
  dtostrf(solarI, 4, 2, b);
  strcpy(line, "SOL  ");
  strcat(line, a);
  strcat(line, "V ");
  strcat(line, b);
  strcat(line, "A");
  putRow(0, line);

  // Row 1: BASE       2.10A
  dtostrf(baseI, 4, 2, b);
  strcpy(line, "BASE       ");
  strcat(line, b);
  strcat(line, "A");
  putRow(1, line);

  // Row 2: BULB 38.5V 2.10A
  dtostrf(bulbV, 4, 1, a);
  dtostrf(loadI, 4, 2, b);
  strcpy(line, "BULB ");
  strcat(line, a);
  strcat(line, "V ");
  strcat(line, b);
  strcat(line, "A");
  putRow(2, line);

  // Row 3: Share:100% SOLAR
  dtostrf((float)share, 3, 0, a);
  strcpy(line, "Share:");
  strcat(line, a);
  strcat(line, "% ");
  strcat(line, mode);
  putRow(3, line);

  // ----- 4) Serial Monitor (more detail) -----
  Serial.print("Solar: ");
  Serial.print(solarV); Serial.print(" V, ");
  Serial.print(solarI); Serial.print(" A | Base: ");
  Serial.print(baseI);  Serial.print(" A | Bulb: ");
  Serial.print(bulbV);  Serial.print(" V, ");
  Serial.print(loadI);  Serial.print(" A, ");
  Serial.print(bulbW);  Serial.print(" W | ");
  Serial.println(mode);

  delay(250);   // raise to 1000 if the simulator runs slowly
}
