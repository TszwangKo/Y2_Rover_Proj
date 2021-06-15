#include <Wire.h>
#include <INA219_WE.h>
#include <SPI.h>
#include <SD.h>


INA219_WE ina219; // this is the instantiation of the library for the current sensor

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

unsigned int loop_trigger;
const int chipSelect = 10;
unsigned int int_count = 0; // a variables to count the interrupts. Used for program debugging.


float duty = 0.01;
float Vb,Iout,Vin_present,Iin_present,p_present,Vin_old,p_old;
float Vpd;
String dataString;

void setup() {

  Wire.begin(); // We need this for the i2c comms for the current sensor
  Wire.setClock(700000); // set the comms speed for i2c
  ina219.init(); // this initiates the current sensor
  
  Serial.begin(9600); // USB Communications



  //Check for the SD Card
  Serial.println("\nInitializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("* is a card inserted?");
    while (true) {} //It will stick here FOREVER if no SD is in on boot
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }

  if (SD.exists("BatCycle.csv")) { // Wipe the datalog when starting
    SD.remove("BatCycle.csv");
  }

   noInterrupts(); //disable all interrupts
  analogReference(EXTERNAL); // We are using an external analogue reference for the ADC


  pinMode(6, OUTPUT); // This is the PWM Pin
  //Analogue input, the battery voltage (also port B voltage)
  pinMode(A0, INPUT);


  // TimerA0 initialization for 1kHz control-loop interrupt.
  TCA0.SINGLE.PER = 999; //
  TCA0.SINGLE.CMP1 = 999; //
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm; //16 prescaler, 1M.
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP1_bm;

  // TimerB0 initialization for PWM output
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; //62.5kHz


  interrupts();  //enable interrupts.
  analogWrite(6, (int)(255 - duty * 255));
   //analogWrite(6, 120)；

  

}

void loop() {

 if (loop_trigger == 1){

      Vb = (analogRead(A0)*4.096/1.03)*2;
      Vpd = analogRead(A3)*4.096/1.03;
      Iout = (ina219.getCurrent_mA()); // sample the inductor current (via the sensor chip)

      Vin_present = Vpd*3;
      p_present = Vb*Iout;

      duty = duty + 0.001;
      
    duty = saturation(duty, 0.99, 0.01); //duty_cycle saturation
    analogWrite(6, (int)(255 - duty * 255)); // write it out
    

     dataString = String(duty) + "," + String(Vb) + "," +  String(Iin_present) + ","  + String(Vin_present)+"," + String(p_present) + "," +  String(Iout);
     Serial.println(dataString);

    File dataFile = SD.open("BatCycle.csv", FILE_WRITE); // open our CSV file
    if (dataFile){ //If we succeeded (usually this fails if the SD card is out)
      dataFile.println(dataString); // print the data
    } else {
      Serial.println("File not open"); //otherwise print an error
    }
    dataFile.close(); // close the file
    loop_trigger = 0;
 }
   
}


ISR(TCA0_CMP1_vect) {
  loop_trigger = 1; //trigger the loop when we are back in normal flow
  TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP1_bm; //clear interrupt flag
}


float saturation( float sat_input, float uplim, float lowlim) { // Saturation function
  if (sat_input > uplim) sat_input = uplim;
  else if (sat_input < lowlim ) sat_input = lowlim;
  else;
  return sat_input;
}
