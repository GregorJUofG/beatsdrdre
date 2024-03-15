#include "mbed.h"
// #include "TextLCD.h"
// #include "platform/mbed_thread.h"
// #include <string>

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

// OUR BOARDS PINS
// DigitalOut gpo(p12);
// AnalogOut Aout(p18);
// AnalogIn Ain(p17);

// JAMES PINS
DigitalOut gpo(p20);
AnalogOut Aout(p18);
AnalogIn Ain(p15);

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
int pauseCounter = 0;
float alpha = 0.2;
float i;
float processed;
float previous;
float processedArray[200] = {0};
float sum;
float rollingAvg;
float normalised;
float calculation = 0;
char pattern[8] = {};
int matrixVal;
// char  pattern_diagonal[8] = { 0x01, 0x2,0x4,0x08,0x10,0x20,0x40,0x80};
// char  pattern_square[8] = { 0xff, 0x81,0x81,0x81,0x81,0x81,0x81,0xff};
// char  pattern_star[8] = { 0x04, 0x15, 0x0e, 0x1f, 0x0e, 0x15, 0x04, 0x00};
/*
Write to the maxim via SPI
args register and the column data
*/

float sample() {
    // Read in the sample
    i = Ain;

    // 1ST ORDER FILTER
    processed = alpha * i + (1 - alpha) * processedArray[199];

    // Saving the filtered value to an array of 1000 elements
    processedArray[199] = processed;

    // Loop through all samples in the 1000 element array and get the rolling average value
    counter++;
    if (counter>199) {
        
        sum = 0; rollingAvg = 0;
        for (int j = 0; j < 199; j++) {
            sum = sum + processedArray[j];
        }

        rollingAvg = sum / 200;
    }

    // Normalise the filtered value using the rolling average
    // and a constant number to bring value positive
    normalised = processed - rollingAvg;

    // Return the single normalised value
    return normalised;
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

int main() {
    setup_dot_matrix(); /* setup matric */
    while (1) {
        i = sample();
        calculation = (int(i * 8)) / 8.0; // 8 DISCRETE LEVELS
        // calculation is floating point between 0 and 1
        Aout = calculation;
        // This value between 0 and 1 is passed into get display
        // get_display(calculation);

        pauseCounter++;
        if(pauseCounter > 20){

            matrixVal = 1;
            for (int j=0; j<7; j++) {
                pattern[j] = pattern[j+1];
            }

            for (int j=0; j < ((calculation*8)+1); j++) {
                matrixVal  *= 2;
            }
            pattern[7] = matrixVal;
            pauseCounter = 0;
        }
        pattern_to_display(pattern);
        // pc.printf("Hello World\n");
  }
}

// ================= UNUSED CODE ================= 

// The value passed in is between 0 and 1
void get_display(float discreteValue) {
  for(int increment = 0; increment<8; increment++){
    if(pattern[increment]%16 == 1){
      pattern[increment] = pattern[increment] - 1;
    }
    pattern[increment] = pattern[increment]/2;
  }

  // if the display isn't picking up some rows then change dividedValue
  float dividedValue = 1.5 / 10; // Calculate the value to be divided by
  
  if (discreteValue >= 0 && discreteValue < dividedValue) {
    pattern[7] = pattern[7] + 0x80;
  } else if (discreteValue >= dividedValue && discreteValue < 2 * dividedValue) {
    pattern[6] = pattern[6] + 0x80;
  } else if (discreteValue >= 2 * dividedValue && discreteValue < 3 * dividedValue) {
    pattern[5] = pattern[5] + 0x80;
  } else if (discreteValue >= 3 * dividedValue && discreteValue < 4 * dividedValue) {
    pattern[4] = pattern[4] + 0x80;
  } else if (discreteValue >= 4 * dividedValue && discreteValue < 5 * dividedValue) {
    pattern[3] = pattern[3] + 0x80;
  } else if (discreteValue >= 5 * dividedValue && discreteValue < 6 * dividedValue) {
    pattern[2] = pattern[2] + 0x80;
  } else if (discreteValue >= 6 * dividedValue && discreteValue < 7 * dividedValue) {
    pattern[1] = pattern[1] + 0x80;
  } else if (discreteValue >= 7*dividedValue && discreteValue < 3.3){ //this 3 can be changed depending on what the max input is
    pattern[0] = pattern[0] + 0x80;
  }
}