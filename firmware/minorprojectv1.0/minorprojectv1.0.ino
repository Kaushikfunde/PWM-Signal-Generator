#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// --- Hardware Pin Definitions ---
const int freqPin = 34;  // 2.97V Stepped-down frequency input from voltage divider
const int rangePin = 27; // Range sense pin (High = Range A, Low = Range B)

// --- OLED Configuration ---
// SH1106 128x64 using hardware I2C (SDA = GPIO21, SCL = GPIO22)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 22, /* data=*/ 21);

// --- Interrupt Variables ---
// 'volatile' ensures the RAM is updated properly during high-speed ISR execution
volatile unsigned long pulseCount = 0;
unsigned long currentFrequency = 0;
unsigned long lastTime = 0;

// --- Interrupt Service Routine (ISR) ---
// Triggers exactly once every time the pulse goes from LOW to HIGH
void IRAM_ATTR countPulse() {
  pulseCount++;
}

void setup() {
  Serial.begin(115200);
  
  // Configure pins
  pinMode(freqPin, INPUT);
  pinMode(rangePin, INPUT_PULLUP); // Pullup prevents floating reads if left disconnected
  
  // Attach the hardware interrupt
  attachInterrupt(digitalPinToInterrupt(freqPin), countPulse, RISING);

  // Initialize the OLED Display
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 30, "System Booting...");
  u8g2.sendBuffer();
  
  delay(1000); // Brief pause to allow power rails to stabilize
}

void loop() {
  unsigned long currentTime = millis();

  // Non-blocking 1-second timer to calculate Frequency (Hz)
  if (currentTime - lastTime >= 1000) {
    
    // Suspend interrupts for 1 microsecond to safely transfer the count variable
    noInterrupts();
    currentFrequency = pulseCount;
    pulseCount = 0;
    interrupts();

    lastTime = currentTime;

    // Read the hardware range state
    bool isRangeA = digitalRead(rangePin);

    // --- Update the OLED Screen ---
    u8g2.clearBuffer();
    
    // Print Frequency
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.setCursor(0, 25);
    u8g2.print("Freq: ");
    u8g2.print(currentFrequency);
    u8g2.print(" Hz");

    // Print Active Range
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.setCursor(0, 55);
    u8g2.print("Range: ");
    if (isRangeA) {
      u8g2.print("A (1 uF)");
    } else {
      u8g2.print("B (150 nF)");
    }
    
    u8g2.sendBuffer();

    // Print to Serial Monitor for debugging
    Serial.print("Frequency: ");
    Serial.print(currentFrequency);
    Serial.println(" Hz");
  }
}