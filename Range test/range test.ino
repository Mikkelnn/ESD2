// the number of the LED pin
const int PWM_output_pin = 22;  // 16 corresponds to GPIO16

// setting PWM properties
const int freq = 39000; // 80Khz
const int PWM_Channel = 0;
const int resolution = 8;
const int Dutycycle_50_Percent = 255/2;
 
void setup(){
  // configure LED PWM functionalitites
  ledcSetup(PWM_Channel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(PWM_output_pin, PWM_Channel);

  // set the duty cycle of the PWM channel
  ledcWrite(PWM_Channel, Dutycycle_50_Percent);
}

void loop() {
  // put your main code here, to run repeatedly:

}
