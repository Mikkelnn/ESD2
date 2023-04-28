
#define NUMBER_BIT 8
const int byteGPIO[NUMBER_BIT] = {36, 39, 34, 35, 32, 25, 26}; // GPIO pins for the 8 bit, LSB first

const 

void setup() {
  // put your setup code here, to run once:

  for (int i = 0; i < NUMBER_BIT; i++) {
    pinMode(byteGPIO[i], INPUT);
  }

}

void loop() {
  // put your main code here, to run repeatedly:

}
