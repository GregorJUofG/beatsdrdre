// WORKING CODE
#include "mbed.h"

DigitalOut gpo(p12);
AnalogOut Aout(p18);
AnalogIn Ain(p17);

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

float sample() {
    i = Ain;

    // 1ST ORDER FILTER
    processed = alpha * i + (1-alpha) * processedArray[((counter-1)%1000)];

    processedArray[(counter)%1000] = processed;

    // Loop through all samples in the array and get the rolling average value
    sum = 0;
    rollingAvg = 0;
    for (int j = 0; j < 200; j++){
            sum = sum + processedArray[j];
        }

    rollingAvg = sum/200;
    normalised = processed-rollingAvg+0.3;

    counter++;
    if (counter >= 1000) {counter = 0;}

    return normalised;
 }

int main() {
  while (1) {
    i = sample();
    Aout = (int(i * 8))/8.0; // 8 DISCRETE LEVELS
    wait_us(10000);
  }
}