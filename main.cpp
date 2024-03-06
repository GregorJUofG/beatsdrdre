// #include "SerialStream.h"
#include "mbed.h"


#define max7219_reg_noop 0x00
#define max7219_reg_digit0 0x01
#define max7219_reg_digit1 0x02
#define max7219_reg_digit2 0x03
#define max7219_reg_digit3 0x04
#define max7219_reg_digit4 0x05
#define max7219_reg_digit5 0x06
#define max7219_reg_digit6 0x07
#define max7219_reg_digit7 0x08
#define max7219_reg_decodeMode 0x09
#define max7219_reg_intensity 0x0a
#define max7219_reg_scanLimit 0x0b
#define max7219_reg_shutdown 0x0c
#define max7219_reg_displayTest 0x0f

#define LOW 0
#define HIGH 1

DigitalOut gpo(p12);
AnalogOut Aout(p18);
AnalogIn Ain(p17);
// FDRM PTD2 = MOSI, PTD1 = SCLK, LOAD = PTDO (digital pin);
// LPC1768 mosi = p5, sclk = p7, load = p8;
SPI max72_spi(p5, NC, p7);
// SPI max72_spi(PTD2, NC, PTD1);
DigitalOut load(p8); // will provide the load signal
// DigitalOut load(PTD0); //will provide the load signal
DigitalOut LEDout(LED1); // will provide the load signal
BufferedSerial serial(USBTX, USBRX, 115200);
// SerialStream<BufferedSerial> pc(serial);

int counter = 0;
float alpha = 0.2;
float i;
float processed;
float previous;
float processedArray[1000] = {0};
float sum;
float rollingAvg;
float normalised;
float calculation = 0;
// char  pattern_diagonal[8] = { 0x01, 0x2,0x4,0x08,0x10,0x20,0x40,0x80};
// char  pattern_square[8] = { 0xff, 0x81,0x81,0x81,0x81,0x81,0x81,0xff};
// char  pattern_star[8] = { 0x04, 0x15, 0x0e, 0x1f, 0x0e, 0x15, 0x04, 0x00};
char pattern[8] = {0, 0, 0, 0, 0, 0, 0, 0};
char tempZero;
char tempOne;
char temp;
/*
Write to the maxim via SPI
args register and the column data
*/

float sample() {
  i = Ain;

  // 1ST ORDER FILTER
  processed = alpha * i + (1 - alpha) * processedArray[((counter - 1) % 1000)];

  processedArray[(counter) % 1000] = processed;

  // Loop through all samples in the array and get the rolling average value
  sum = 0;
  rollingAvg = 0;
  for (int j = 0; j < 200; j++) {
    sum = sum + processedArray[j];
  }

  rollingAvg = sum / 200;
  normalised = processed - rollingAvg + 0.3;

  counter++;
  if (counter >= 1000) {
    counter = 0;
  }

  return normalised;
}

void get_display(float discreteValue) {
  // in the if statement need to replace 0x00 with whatever row/column
  tempZero = pattern[0];
  if (discreteValue == 0) {
    pattern[0] = 0x01;
  } else if (discreteValue == 0.4125) {
    pattern[0] = 0x2;
  } else if (discreteValue == 0.8250) {
    pattern[0] = 0x4;
  } else if (discreteValue == 1.2375) {
    pattern[0] = 0x08;
  } else if (discreteValue == 1.6500) {
    pattern[0] = 0x10;
  } else if (discreteValue == 2.0625) {
    pattern[0] = 0x20;
  } else if (discreteValue == 2.4750) {
    pattern[0] = 0x40;
  } else {
    pattern[0] = 0x80;
  }
  for (int i = 1; i <= 7; i++) {
    if (i == 1) {
      tempOne = pattern[i];
      pattern[i] = tempZero;
    } else {
      temp = pattern[i];
      pattern[i] = tempOne;
      tempOne = temp;
    }
  }
}

void write_to_max(int reg, int col) {
  load = LOW;           // begin
  max72_spi.write(reg); // specify register
  max72_spi.write(col); // put data
  load = HIGH;          // make sure data is loaded (on rising edge of LOAD/CS)
                        // pc.printf("Writing\n");
}

// writes 8 bytes to the display
void pattern_to_display(char *testdata) {
  int cdata;
  for (int idx = 0; idx <= 7; idx++) {
    cdata = testdata[idx];
    write_to_max(idx + 1, cdata);
  }
}

void setup_dot_matrix() {
  // initiation of the max 7219
  // SPI setup: 8 bits, mode 0
  max72_spi.format(8, 0);

  max72_spi.frequency(100000); // down to 100khx easier to scope ;-)

  write_to_max(max7219_reg_scanLimit, 0x07);
  write_to_max(max7219_reg_decodeMode,
               0x00);                       // using an led matrix (not digits)
  write_to_max(max7219_reg_shutdown, 0x01); // not in shutdown mode
  write_to_max(max7219_reg_displayTest, 0x00); // no display test
  for (int e = 1; e <= 8; e++) { // empty registers, turn all LEDs off
    write_to_max(e, 0);
  }
  // maxAll(max7219_reg_intensity, 0x0f & 0x0f);    // the first 0x0f is the
  // value you can set
  write_to_max(max7219_reg_intensity, 0x08);
}

void clear() {
  for (int e = 1; e <= 8; e++) { // empty registers, turn all LEDs off
    write_to_max(e, 0);
  }
}
// wait_ms has been depreciated
// ThisThread::sleep_for(BLINKING_RATE) is now used

// specific levels
// Bottom row = 0
// 0.4125
// 0.8250
// 1.2375
// 1.6500
// 2.0625
// 2.4750
// Top Row = 2.8875

int main() {
setup_dot_matrix(); /* setup matric */
  while (1) {
    i = sample();
    calculation = (int(i * 8)) / 8.0;
    Aout = calculation; // 8 DISCRETE LEVELS
    // da_star();
    get_display(calculation);
    pattern_to_display(pattern);
    LEDout = HIGH;
    wait_us(10000);
    LEDout = LOW;
    // //pc.printf("Hello World\n");
    // pattern_to_display(pattern_square);
    // thread_sleep_for(1000);
    // LEDout = HIGH;
    // pattern_to_display(pattern_star);
    // thread_sleep_for(1000);
    // LEDout = LOW;
    // thread_sleep_for(1000);
    // clear();
  }
}