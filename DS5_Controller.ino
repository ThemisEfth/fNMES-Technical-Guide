/***************************/
/*         Arduino         */
/*      DS5 Controller     */
/*                         */
/*  Andreas Gartus (2020)  */
/*                         */
/*  Use at your own risk!  */
/***************************/

#define VERSION "v0.93"

// Serial commands (19200-8N1):

// 255 (0xFF): Start device (TriggerMode; only needed once at beginning) - Can now also stop device!
//  99 (0x63): Identify device by blinking display
//  98 (0x62): Set pulse mode to bipolar (OpAmp enabled)
//  97 (0x61): Set pulse mode to monopolar (OpAmp enabled)
//  96 (0x60): Set pulse mode to monopolar (OpAmp disabled)
//  90 (0x5A) + Byte: Set trigger mode (T0, T1, T2, T3)
//  89 (0x59) + HighByte + LowByte: Set pulse repetition count [1...9999]
//  88 (0x58) + HighByte + LowByte: Set pulse repetition period [1...99999 * 0.1ms]
//  87 (0x57) + HighByte + LowByte: Set pulse width [1...99999 * 0.1ms]
//  83 (0x53) + HighByte + LowByte: Set output voltage [0...4095 * 0.5mV]
//  80 (0x50): Start pulse sequence (triggers and analog output)
//  78 (0x4E): Zero pulse count
//  69 (0x45) + 1: Turn on display
//  68 (0x44) + 0: Turn off display
//  65 (0x41): Query data
//  58 (0x3A) + HighByte + LowByte: Set pulse repetition period [10...9999 * 1us]
//  57 (0x39) + HighByte + LowByte: Set pulse width [10...9999 * 1us]
//  33 (0x21): Emergency stop pulse sequence

#include <LiquidCrystal_I2C.h>  // LiquidCrystal_I2C library by Frank de Brabander, Version 1.1.2

// No operation (single cycle, takes 62.5ns at 16MHz)
#define NOP __asm__ __volatile__ ("nop\n\t")

// Blink display backlight
#define BACKBLINK lcd.noBacklight();delay(200);lcd.backlight();delay(200);

// DAC (LTC1257) pins (for direct port access)
#define DAC_PORT PORTB
#define DAC_DDR DDRB
#define DAC_CLK_AVR PB2   // #define DAC_CLK_PIN 10
#define DAC_DIN_AVR PB3   // #define DAC_DIN_PIN 11
#define DAC_LOAD_AVR PB4  // #define DAC_LOAD_PIN 12

const int DAC_EN_ZERO = 2048;   // DAC value corresponding to 0V with enabled OpAmp

// Trigger pins (for direct port access)
#define TRIG_PORT PORTD
#define TRIG_DDR DDRD
#define TRIG1_AVR PD2     // #define TRIG1_PIN 2
#define TRIG2_AVR PD3     // #define TRIG2_PIN 3

// 20x4 alphanumeric i2c LCD
LiquidCrystal_I2C lcd(0x27,20,4);

// Serial buffer for receiving setting values (needs to be declared globally for whatever reason...)
byte value_buffer[2];

// Device settings (= defaults)
int SET_VOLTAGE = 4000;           // Monopolar or positive bipolar output voltage (* 0.5mV)
int SET_DAC_POS = 4048;           // DAC positive value (updated in setup)
int SET_DAC_ZERO = DAC_EN_ZERO;   // DAC zero value (updated in setup)
int SET_DAC_NEG = 48;             // DAC negative value (updated in setup, only relevant when OpAmp is enabled)
long SET_WIDTH = 50000;           // Output pulse width (* 1us)
uint8_t SET_TRIGGER = 3;          // Trigger settings (0,1,2,3)
int SET_REPEAT = 1;               // Number of pulse repetitions
long SET_PERIOD = 1000000;        // Pulse repetition period (* 1us)
int SET_COUNT = 0;                // Pulse count
bool SET_STARTED = false;         // Device started?
bool SET_DISPLAY_ON = true;       // Display on?
bool SET_BIPOLAR = true;          // Bipolar vs. monopolar pulse mode
bool SET_OPAMP_EN = true;         // OpAmp enabled (determined by hardware)


// Update I2C display
void update_display() {
  char buffer[16];
  if (SET_DISPLAY_ON) {
    //lcd.clear();
    if (SET_STARTED) {
      lcd.setCursor(0,0);
      if (SET_OPAMP_EN) {   // OpAmp enabled => Voltage is only accurate by 1mV
        if (SET_WIDTH <= 9999) sprintf(buffer,"P: %4i.0mV %4i.0us",int(SET_VOLTAGE/2),int(SET_WIDTH));
        else sprintf(buffer,"P: %4i.0mV %4i.%1ims",int(SET_VOLTAGE/2),int(SET_WIDTH/1000),int(SET_WIDTH%1000/100));
        lcd.print(buffer);
      }
      else {
        if (SET_WIDTH <= 9999) sprintf(buffer,"P: %4i.%1imV %4i.0us",int(SET_VOLTAGE/2),int(SET_VOLTAGE%2)*5,int(SET_WIDTH));
        else sprintf(buffer,"P: %4i.%1imV %4i.%1ims",int(SET_VOLTAGE/2),int(SET_VOLTAGE%2)*5,int(SET_WIDTH/1000),int(SET_WIDTH%1000/100));
        lcd.print(buffer);
      }
      lcd.setCursor(0,1);
      if (SET_PERIOD <= 9999) sprintf(buffer,"Rep: %5ix %4i.0us",SET_REPEAT,int(SET_PERIOD));
      else sprintf(buffer,"Rep: %5ix %4i.%1ims",SET_REPEAT,int(SET_PERIOD/1000),int(SET_PERIOD%1000/100));
      lcd.print(buffer);
      lcd.setCursor(0,2);
      sprintf(buffer,"Trig: %1i  PCnt: %5i",SET_TRIGGER,SET_COUNT);
      lcd.print(buffer);
      lcd.setCursor(0,3);
      if (SET_BIPOLAR) sprintf(buffer,"PMode: Bipol (OA EN)");
      else if (SET_OPAMP_EN) sprintf(buffer,"PMode: Mono (OA EN) ");
      else sprintf(buffer,"PMode: Mono (OA DIS)");
      lcd.print(buffer);
    }
    else {  // Stopped
      lcd.clear();
      lcd.setCursor(6,1);
      lcd.print(F("Stopped!"));
    }
  }
  else {
    lcd.clear();
  }
}

// Write voltage value into DAC (but do not yet output it)
void writeDAC(int data) {             // 12 bits, 4095, 0xFFF
  DAC_PORT &= ~(1 << DAC_CLK_AVR);    // digitalWrite(DAC_CLK_PIN,LOW);
  for (int8_t i=11; i>=0; i--) {
    if (data >> i & 1) DAC_PORT |= (1 << DAC_DIN_AVR);    // digitalWrite(DAC_DIN_PIN,data >> i & 1);
    else DAC_PORT &= ~(1 << DAC_DIN_AVR);
    DAC_PORT |= (1 << DAC_CLK_AVR);   // digitalWrite(DAC_CLK_PIN,HIGH);
    NOP; NOP; NOP; NOP;               // Wait for 0.5us
    NOP; NOP; NOP; NOP;
    DAC_PORT &= ~(1 << DAC_CLK_AVR);  // digitalWrite(DAC_CLK_PIN,LOW);
  }
  //NOP; NOP; NOP; NOP;                 // Wait for 0.5us
  //NOP; NOP; NOP; NOP;
  // Data is loaded on rising clock edge => Do NOT set clock high after writing DAC!
  //DAC_PORT |= (1 << DAC_CLK_AVR);     // digitalWrite(DAC_CLK_PIN,HIGH);
}

// Load new voltage value into DAC (= actually output voltage)
void loadDAC() {
  DAC_PORT &= ~(1 << DAC_LOAD_AVR);   // digitalWrite(DAC_LOAD_PIN,LOW);
  NOP; NOP; NOP; NOP;                 // Wait for 0.5us
  NOP; NOP; NOP; NOP;
  DAC_PORT |= (1 << DAC_LOAD_AVR);    // digitalWrite(DAC_LOAD_PIN,HIGH);
}

// Setup
void setup() {
  // Initialize timer 1 to 0.5us counting
  noInterrupts();
  TCCR1A = 0;   // Timer 1 in normal mode
  TCCR1B = 0;
  TCCR1B |= (1 << CS11);    // Start timer 1 with prescaler 8
  TCNT1 = 0;    // Set timer 1 to 0
  interrupts();
  // Initialize trigger pins
  TRIG_DDR |= (1 << TRIG1_AVR);       // pinMode(TRIG1_PIN,OUTPUT);
  TRIG_PORT &= ~(1 << TRIG1_AVR);     // digitalWrite(TRIG1_PIN,LOW);
  TRIG_DDR |= (1 << TRIG2_AVR);       // pinMode(TRIG2_PIN,OUTPUT);
  TRIG_PORT &= ~(1 << TRIG2_AVR);     // digitalWrite(TRIG2_PIN,LOW);
  // Initialize DAC pins
  DAC_DDR |= (1 << DAC_LOAD_AVR);     // pinMode(DAC_LOAD_PIN,OUTPUT);
  DAC_PORT |= (1 << DAC_LOAD_AVR);    // digitalWrite(DAC_LOAD_PIN,HIGH);
  DAC_DDR |= (1 << DAC_CLK_AVR);      // pinMode(DAC_CLK_PIN,OUTPUT);
  DAC_PORT &= ~(1 << DAC_CLK_AVR);    // digitalWrite(DAC_CLK_PIN,LOW);
  DAC_DDR |= (1 << DAC_DIN_AVR);      // pinMode(DAC_DIN_PIN,OUTPUT);
  DAC_PORT &= ~(1 << DAC_DIN_AVR);    // digitalWrite(DAC_DIN_PIN,LOW);
  // Update DAC values (depending on whether OpAmp is enabled or disabled)
  if (SET_OPAMP_EN) {
    SET_DAC_POS = int(SET_VOLTAGE/2) + DAC_EN_ZERO;
    SET_DAC_ZERO = DAC_EN_ZERO;
    SET_DAC_NEG = DAC_EN_ZERO - int(SET_VOLTAGE/2);
  }
  else {
    SET_DAC_POS = SET_VOLTAGE;
    SET_DAC_ZERO = 0;
    SET_DAC_NEG = 0;
  }
  // Set DAC output to zero
  writeDAC(SET_DAC_ZERO);
  loadDAC();
  // Turn off LED connected to pin 13
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
  // Initialize serial connection
  Serial.begin(19200);
  Serial.setTimeout(100);   // Set timeout to 100ms (for Serial.readBytes)
  // Initialize i2c LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(6,0);
  lcd.print(F("Arduino"));
  lcd.setCursor(3,1);
  lcd.print(F("DS5 Controller"));
  lcd.setCursor(7,2);
  lcd.print(VERSION);
  lcd.setCursor(2,3);
  lcd.print(F("A. Gartus (2020)"));
  delay(2000);
  writeDAC(SET_DAC_POS);  // Prepare DAC for positive impulse
  update_display();
}

// Main loop
void loop() {
  char buffer[16];
  byte command;
  size_t num_bytes = 0;
  unsigned long start_time;
  unsigned long current_time;
  unsigned int start_timer1;
  unsigned int current_timer1;
  long half_pulse_width;
  if (Serial.available() > 0) {
    // Read incoming byte
    command = Serial.read();
    // Device started?
    if (SET_STARTED) {
      switch(command) {
        case 99:  // Identify device by blinking display
          // for (uint8_t i=0; i<10; i++) {
          BACKBLINK
          BACKBLINK
          BACKBLINK
          BACKBLINK
          BACKBLINK
          BACKBLINK
          BACKBLINK
          BACKBLINK
          BACKBLINK
          BACKBLINK
          //}
          break;
        case 98:  // Set pulse mode to bipolar (OpAmp enabled)
          SET_BIPOLAR = true;
          SET_OPAMP_EN = true;
          SET_DAC_POS = int(SET_VOLTAGE/2) + DAC_EN_ZERO;
          SET_DAC_ZERO = DAC_EN_ZERO;
          SET_DAC_NEG = DAC_EN_ZERO - int(SET_VOLTAGE/2);
          // Update display
          update_display();
          // Serial feedback
          Serial.write(198);
          break;
        case 97:  // Set pulse mode to monopolar (OpAmp enabled)
          SET_BIPOLAR = false;
          SET_OPAMP_EN = true;
          SET_DAC_POS = int(SET_VOLTAGE/2) + DAC_EN_ZERO;
          SET_DAC_ZERO = DAC_EN_ZERO;
          SET_DAC_NEG = DAC_EN_ZERO - int(SET_VOLTAGE/2);
          // Update display
          update_display();
          // Serial feedback
          Serial.write(197);
          break;
        case 96:  // Set pulse mode to monopolar (OpAmp disabled)
          SET_BIPOLAR = false;
          SET_OPAMP_EN = false;
          SET_DAC_POS = SET_VOLTAGE;
          SET_DAC_ZERO = 0;
          SET_DAC_NEG = 0;
          // Update display
          update_display();
          // Serial feedback
          Serial.write(196);
          break;
        case 90:  // Set triggers (new command)
          num_bytes = Serial.readBytes(value_buffer,1);
          if (num_bytes == 1) {
            SET_TRIGGER = uint8_t(value_buffer[0]);
            if (SET_TRIGGER > 3) SET_TRIGGER = 3;
            // Update display
            update_display();
            // Serial feedback
            Serial.write(130);
          }
          break;
        case 89:  // Set pulse repetition times (new command)
          num_bytes = Serial.readBytes(value_buffer,2);
          if (num_bytes == 2) {
            SET_REPEAT = int(value_buffer[0])*256 + int(value_buffer[1]);
            //if (SET_REPEAT > 9999) SET_REPEAT = 9999;
            if (SET_REPEAT < 1) SET_REPEAT = 1;
            // Update display
            update_display();
            // Serial feedback
            Serial.write(120);
          }
          break;
        case 88:  // Set pulse repetition period (new command)
          num_bytes = Serial.readBytes(value_buffer,2);
          if (num_bytes == 2) {
            SET_PERIOD = 100*(long(value_buffer[0])*256 + long(value_buffer[1]));
            if (SET_PERIOD > 9999900) SET_PERIOD = 9999900;
            if (SET_PERIOD < 100) SET_PERIOD = 100;
            // Update display
            update_display();
            // Serial feedback
            Serial.write(121);
          }
          break;
        case 80:  // Pulse
          // Pulse (train) output plus trigger impulses
          if ((SET_TRIGGER == 1)||(SET_TRIGGER == 3))
            TRIG_PORT |= (1 << TRIG1_AVR);    // digitalWrite(TRIG1_PIN,HIGH);
          if ((SET_TRIGGER == 2)||(SET_TRIGGER == 3))
            TRIG_PORT |= (1 << TRIG2_AVR);    // digitalWrite(TRIG2_PIN,HIGH);
          // --------------------------------------------------------------------------------
          // Pulse train output
          // --------------------------------------------------------------------------------
          if (SET_BIPOLAR) {  // Bipolar output
            // Divide pulse width into half for positive and negative wave width
            half_pulse_width = long(SET_WIDTH/2);
            // Use different timing methods
            if (SET_WIDTH > 9999) {            // Pulse width > 9.999ms => use micros function (only 4us accuracy)
              for (int i=0; i<SET_REPEAT; i++) {
                start_time = micros();
                loadDAC();
                writeDAC(SET_DAC_NEG);
                for (;;) {
                  current_time = micros();
                  if ((current_time-start_time) > half_pulse_width) break;
                }
                loadDAC();
                writeDAC(SET_DAC_ZERO);
                for (;;) {
                  current_time = micros();
                  if ((current_time-start_time) > SET_WIDTH) break;
                }
                loadDAC();
                writeDAC(SET_DAC_POS);
                if (Serial.peek() == 33) break;   // Stop pulse sequence if 33 is the last byte in the serial input buffer
                if (i == (SET_REPEAT-1)) break;
                for (;;) {
                  current_time = micros();
                  if ((current_time-start_time) > SET_PERIOD) break;
                }
              }
            }
            else if (SET_PERIOD > 9999) {     // Pulse width <= 9.999ms, but repetion period > 9.999ms => use timer 1 for pulse width
              for (int i=0; i<SET_REPEAT; i++) {
                start_time = micros();
                start_timer1 = TCNT1;
                loadDAC();
                writeDAC(SET_DAC_NEG);
                for (;;) {
                  current_timer1 = TCNT1;
                  if ((current_timer1-start_timer1) > int(2*half_pulse_width)) break;   // Timer 1 counts 0.5us => 2*half_pulse_width
                }
                loadDAC();
                writeDAC(SET_DAC_ZERO);
                for (;;) {
                  current_timer1 = TCNT1;
                  if ((current_timer1-start_timer1) > int(2*SET_WIDTH)) break;   // Timer 1 counts 0.5us => 2*SET_WIDTH
                }
                loadDAC();
                writeDAC(SET_DAC_POS);
                if (Serial.peek() == 33) break;   // Stop pulse sequence if 33 is the last byte in the serial input buffer
                if (i == (SET_REPEAT-1)) break;
                for (;;) {
                  current_time = micros();
                  if ((current_time-start_time) > SET_PERIOD) break;
                }
              }
            }
            else {    // Use timer 1 for both pulse width and repetion period
              for (int i=0; i<SET_REPEAT; i++) {
                start_timer1 = TCNT1;
                loadDAC();
                writeDAC(SET_DAC_NEG);
                for (;;) {
                  current_timer1 = TCNT1;
                  if ((current_timer1-start_timer1) > int(2*half_pulse_width)) break;   // Timer 1 counts 0.5us => 2*half_pulse_width
                }
                loadDAC();
                writeDAC(SET_DAC_ZERO);
                for (;;) {
                  current_timer1 = TCNT1;
                  if ((current_timer1-start_timer1) > int(2*SET_WIDTH)) break;   // Timer 1 counts 0.5us => 2*SET_WIDTH
                }
                loadDAC();
                writeDAC(SET_DAC_POS);
                if (Serial.peek() == 33) break;   // Stop pulse sequence if 33 is the last byte in the serial input buffer
                if (i == (SET_REPEAT-1)) break;
                for (;;) {
                  current_timer1 = TCNT1;
                  if ((current_timer1-start_timer1) > int(2*SET_PERIOD)) break;  // Timer 1 counts 0.5us => 2*SET_PERIOD
                }
              }
            }
          }
          // --------------------------------------------------------------------------------
          else {  // Monopolar output
            // Use different timing methods
            if (SET_WIDTH > 9999) {            // Pulse width > 9.999ms => use micros function (only 4us accuracy)
              for (int i=0; i<SET_REPEAT; i++) {
                start_time = micros();
                loadDAC();
                writeDAC(SET_DAC_ZERO);
                for (;;) {
                  current_time = micros();
                  if ((current_time-start_time) > SET_WIDTH) break;
                }
                loadDAC();
                writeDAC(SET_DAC_POS);
                if (Serial.peek() == 33) break;   // Stop pulse sequence if 33 is the last byte in the serial input buffer
                if (i == (SET_REPEAT-1)) break;
                for (;;) {
                  current_time = micros();
                  if ((current_time-start_time) > SET_PERIOD) break;
                }
              }
            }
            else if (SET_PERIOD > 9999) {     // Pulse width <= 9.999ms, but repetion period > 9.999ms => use timer 1 for pulse width
              for (int i=0; i<SET_REPEAT; i++) {
                start_time = micros();
                start_timer1 = TCNT1;
                loadDAC();
                writeDAC(SET_DAC_ZERO);
                for (;;) {
                  current_timer1 = TCNT1;
                  if ((current_timer1-start_timer1) > (2*SET_WIDTH)) break;   // Timer 1 counts 0.5us => 2*SET_WIDTH
                }
                loadDAC();
                writeDAC(SET_DAC_POS);
                if (Serial.peek() == 33) break;   // Stop pulse sequence if 33 is the last byte in the serial input buffer
                if (i == (SET_REPEAT-1)) break;
                for (;;) {
                  current_time = micros();
                  if ((current_time-start_time) > SET_PERIOD) break;
                }
              }
            }
            else {    // Use timer 1 for both pulse width and repetion period
              for (int i=0; i<SET_REPEAT; i++) {
                start_timer1 = TCNT1;
                loadDAC();
                writeDAC(SET_DAC_ZERO);
                for (;;) {
                  current_timer1 = TCNT1;
                  if ((current_timer1-start_timer1) > (2*SET_WIDTH)) break;   // Timer 1 counts 0.5us => 2*SET_WIDTH
                }
                loadDAC();
                writeDAC(SET_DAC_POS);
                if (Serial.peek() == 33) break;   // Stop pulse sequence if 33 is the last byte in the serial input buffer
                if (i == (SET_REPEAT-1)) break;
                for (;;) {
                  current_timer1 = TCNT1;
                  if ((current_timer1-start_timer1) > (2*SET_PERIOD)) break;  // Timer 1 counts 0.5us => 2*SET_PERIOD
                }
              }
            }
          }
          // --------------------------------------------------------------------------------
          TRIG_PORT &= ~(1 << TRIG1_AVR);   // digitalWrite(TRIG1_PIN,LOW);
          TRIG_PORT &= ~(1 << TRIG2_AVR);   // digitalWrite(TRIG2_PIN,LOW);
          SET_COUNT = SET_COUNT + 1;
          if (SET_COUNT > 99999) SET_COUNT = 0;
          // Update display
          update_display();
          // Serial feedback
          Serial.write(100);
          break;
        case 83:  // Set output voltage
          num_bytes = Serial.readBytes(value_buffer,2);
          if (num_bytes == 2) {
            if (SET_OPAMP_EN) {
              SET_VOLTAGE = int(value_buffer[0])*256 + int(value_buffer[1]);
            }
            else {
              SET_VOLTAGE = int(value_buffer[0])*256 + int(value_buffer[1]);
              if (SET_VOLTAGE > 4095) SET_VOLTAGE = 4095;
            }
            // Update DAC values (depending on whether OpAmp is enabled or disabled)
            if (SET_OPAMP_EN){
              SET_DAC_POS = int(float(SET_VOLTAGE)/2.0) - DAC_EN_ZERO;
              SET_DAC_ZERO = DAC_EN_ZERO;
              SET_DAC_NEG = DAC_EN_ZERO - int(float(SET_VOLTAGE)/2.0);
            }
            else {
              SET_DAC_POS = SET_VOLTAGE;
              SET_DAC_ZERO = 0;
              SET_DAC_NEG = 0;
            }
            // Write voltage to DAC (= prepare DAC)
            writeDAC(SET_DAC_POS);
            // Update display
            update_display();
            // Serial feedback
            Serial.write(110);
          }
          break;
        case 87:  // Set pulse width
          num_bytes = Serial.readBytes(value_buffer,2);
          if (num_bytes == 2) {
            SET_WIDTH = 100*(long(value_buffer[0])*256 + long(value_buffer[1]));
            if (SET_WIDTH > 9999900) SET_WIDTH = 9999900;
            if (SET_WIDTH < 100) SET_WIDTH = 100;
            // Update display
            update_display();
            // Serial feedback
            Serial.write(111);
          }
          break;
        case 78:  // Zeroing pulse count
          SET_COUNT = 0;
          // Update display
          update_display();
          // Feedback
          Serial.write(112);
          break;
        case 69:  // Select electrode (69+1 also turns display on)
          num_bytes = Serial.readBytes(value_buffer,1);
          if (num_bytes == 1) {
            // Mostly ignored as device cannot set electrodes, but 69+1 turns display on
            if (uint8_t(value_buffer[0]) == 1) {
              SET_DISPLAY_ON = true;
              lcd.backlight();  // Turn on LCD backlight
              update_display();
            }
            // Serial feedback
            Serial.write(114);
          }
          break;
        case 68:  // 68+0 turns display off
          num_bytes = Serial.readBytes(value_buffer,1);
          if (num_bytes == 1) {
            if (uint8_t(value_buffer[0]) == 0) {
              SET_DISPLAY_ON = false;
              lcd.noBacklight();  // Turn off LCD backlight
              update_display();
            }
            // Serial feedback
            Serial.write(113);
          }
          break;
        case 65:  // Query data
          // Send "old" data
          Serial.write(1);
          Serial.write(int(SET_VOLTAGE/256));
          Serial.write(SET_VOLTAGE%256);
          Serial.write(int(SET_WIDTH/256));
          Serial.write(SET_WIDTH%256);
          Serial.write(int(SET_COUNT/256));
          Serial.write(SET_COUNT%256);
          // Send also new data
          Serial.write(int(SET_REPEAT/256));
          Serial.write(SET_REPEAT%256);
          Serial.write(int(SET_PERIOD/256));
          Serial.write(SET_PERIOD%256);
          Serial.write(SET_TRIGGER);
          Serial.write(int(SET_BIPOLAR));
          Serial.write(int(SET_OPAMP_EN));
          break;
        case 58:  // Set pulse repetition period in us (new command)
          num_bytes = Serial.readBytes(value_buffer,2);
          if (num_bytes == 2) {
            SET_PERIOD = long(value_buffer[0])*256 + long(value_buffer[1]);
            if (SET_PERIOD > 9999) SET_PERIOD = 9999;
            if (SET_PERIOD < 10) SET_PERIOD = 10;
            // Update display
            update_display();
            // Serial feedback
            Serial.write(121);
          }
          break;
        case 57:  // Set pulse width in us (new command)
          num_bytes = Serial.readBytes(value_buffer,2);
          if (num_bytes == 2) {
            SET_WIDTH = long(value_buffer[0])*256 + long(value_buffer[1]);
            if (SET_WIDTH > 9999) SET_WIDTH = 9999;
            if (SET_WIDTH < 10) SET_WIDTH = 10;
            // Update display
            update_display();
            // Serial feedback
            Serial.write(111);
          }
          break;
        case 255:  // Stop device
          SET_STARTED = false;
          update_display();
          break;
      }
    }
    else {
      switch(command) {
      case 255:  // Start device (TriggerMode)
        SET_STARTED = true;
        writeDAC(SET_DAC_POS);
        update_display();
        break;
      case 99:   // Identify device by blinking display
        //for (uint8_t i=0; i<10; i++) {
          BACKBLINK
          BACKBLINK
          BACKBLINK
          BACKBLINK
          BACKBLINK
          BACKBLINK
          BACKBLINK
          BACKBLINK
          BACKBLINK
          BACKBLINK
        //}
        break;
      }
    }
  }
}
