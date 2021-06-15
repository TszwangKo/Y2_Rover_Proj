#include <Wire.h>
#include <INA219_WE.h>
#include <SPI.h>
#include <SD.h>

INA219_WE ina219; // this is the instantiation of the library for the current sensor

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

const int chipSelect = 10;
unsigned int rest_timer;
unsigned int loop_trigger;
float Vpd;
unsigned int int_count = 0; // a variables to count the interrupts. Used for program debugging.
float u0i, u1i, delta_ui, e0i, e1i, e2i; // Internal values for the current controller
float Vin_present,Iin_present,p_present,Vin_old,p_old;
float ui_max = 1, ui_min = 0; //anti-windup limitation
float kpi = 0.02512, kii = 39.4, kdi = 0; // current pid.
float Ts = 0.001; //1 kHz control frequency.
float current_measure, current_ref = 0, error_amps; // Current Control
float pwm_out;
float V_Bat;
boolean input_switch;
boolean b1rly,b2rly,b1dis,b2dis;
int state_num=0,next_state;
String dataString;
float Vb1 = 0,Vb2 = 0;

void setup() {
  //Some General Setup Stuff

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

  //SMPS Pins
  pinMode(13, OUTPUT); // Using the LED on Pin D13 to indicate status
  pinMode(2, INPUT_PULLUP); // Pin 2 is the input from the CL/OL switch
  pinMode(6, OUTPUT); // This is the PWM Pin

  //LEDs on pin 7 and 8
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);

  //Analogue input, the battery voltage (also port B voltage)
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A3, INPUT);
  pinMode(A6, INPUT);


  //relays and discharge port pins // battery 1 red, relay D3, dis D9, meas A1 // battery 2 white, relay D4, dis D5, meas A6
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(9, OUTPUT);

  // TimerA0 initialization for 1kHz control-loop interrupt.
  TCA0.SINGLE.PER = 999; //
  TCA0.SINGLE.CMP1 = 999; //
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm; //16 prescaler, 1M.
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP1_bm;

  // TimerB0 initialization for PWM output
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; //62.5kHz

  interrupts();  //enable interrupts.
  analogWrite(6, 120); //just a default state to start with

}

void loop() {
  if (loop_trigger == 1){ // FAST LOOP (1kHZ)
      
      state_num = next_state; //state transition
      
      if(state_num == 1){
         V_Bat = 2456;// when both relay high, set Vbat to value so that not causing error
      }else{
         V_Bat = analogRead(A0)*4.096/1.03; //check the battery voltage (1.03 is a correction for measurement error, you need to check this works for you)
      }

      Vpd = analogRead(A3)*4.096/1.03;

      
      if(state_num == 0){
         digitalWrite(3, false);
         digitalWrite(4, false);
         digitalWrite(5, false);
         digitalWrite(9, false);
      }
      
      //Vb1 = analogRead(A1) * 4.096 / 1.03;
      //Vb2 = analogRead(A6) * 4.096 / 1.03;

      b1rly = digitalRead(3);
      b2rly = digitalRead(4);
      b1dis = digitalRead(9);
      b2dis = digitalRead(5);
      
      //if ((V_Bat > 3700 || V_Bat < 2400)) { //Checking for Error states (just battery voltage for now)
          //state_num = 5; //go directly to jail
          //next_state = 5; // stay in jail
          //digitalWrite(7,true); //turn on the red LED
          //current_ref = 0; // no current
      //}
      current_measure = (ina219.getCurrent_mA()); // sample the inductor current (via the sensor chip)
      if((state_num == 1) || (state_num == 0)){
            error_amps = (current_ref - current_measure) / 1000; //PID error calculation
            pwm_out = pidi(error_amps); //Perform the PID controller calculation
            pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation
            analogWrite(6, (int)(255 - pwm_out * 255)); // write it out (inverting for the Buck here)
            int_count++; //count how many interrupts since this was last reset to zero
            loop_trigger = 0; //reset the trigger and move on with life
      }else{
           if(current_measure < (current_ref - 30)){
           //if(1){
              Vin_present = Vpd*3;
              //Vin_present =V_Bat/pwm_out;
              Iin_present = current_measure*pwm_out; 
              p_present = V_Bat*current_measure;
              if(p_present > p_old){
                   if(Vin_present > Vin_old){
                        pwm_out  = pwm_out - 0.012;
                    }else{
                        pwm_out  = pwm_out + 0.012;}
  
              }else{
                    if(Vin_present > Vin_old){
                        pwm_out  = pwm_out + 0.012;
                    }else{
                        pwm_out  = pwm_out - 0.012;
                    }
              }       
              pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation
              analogWrite(6, (int)(255 - pwm_out * 255)); // write it out
  
              //Iin_old = Iin_present;
              Vin_old = Vin_present;
              p_old = p_present;
              int_count++;
              loop_trigger = 0;
           
           }else{
              error_amps = (current_ref - current_measure) / 1000; //PID error calculation
              pwm_out = pidi(error_amps); //Perform the PID controller calculation
              pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation
              analogWrite(6, (int)(255 - pwm_out * 255)); // write it out (inverting for the Buck here)
              int_count++; //count how many interrupts since this was last reset to zero
              loop_trigger = 0; //reset the trigger and move on with life
           }
        
      }
  }
  
  if (int_count == 1000) { // SLOW LOOP (1Hz)
    input_switch = digitalRead(2); //get the OL/CL switch status
    //input_switch = <UART> charging_signal;
    switch (state_num) { // STATE MACHINE (see diagram)
      case 0:{ // Start state (no current, no LEDs)
        current_ref = 0;
        if (input_switch == 1) { // if switch, move to charge
          next_state = 2;
          digitalWrite(8,false);
        } else { // otherwise stay put
          next_state = 0;
          digitalWrite(8,false);
          digitalWrite(3, false);
          digitalWrite(4, false);
        }
        //else if( <UART> idle_on == 1){
        //        next_state = 6; idle state for measure OCV
        //        }
        break;
      }

      case 1:{ //just measurement
          current_ref = 0;
          digitalWrite(3, true);// battery 1 red, relay D3, dis D9, meas A1
          digitalWrite(4, true);// battery 2 white, relay D4, dis D5, meas A6
            Vb1 = analogRead(A1) * 4.096 / 1.03;
            Vb2 = analogRead(A6) * 4.096 / 1.03;

        if(rest_timer < 120){
            next_state = 1;
            rest_timer++;
        }else{
            if((Vb1 - Vb2) > 5){
              next_state = 3;
              rest_timer = 0;
            }else if((Vb2 - Vb1) > 5){
              next_state = 4;
              rest_timer = 0;
            }else{
              next_state = 2;
              rest_timer = 0;
              digitalWrite(5, false);
              digitalWrite(9, false);
              digitalWrite(3, false);
              digitalWrite(4, false);
          }
        }
       

          if(input_switch == 0){ 
              next_state = 0;
              digitalWrite(8,false);
              digitalWrite(3, false);
              digitalWrite(4, false);
        }
            break;    
      }


      
      case 2:{ // Charge state (250mA and a green LED)
        current_ref = 500;
        if(V_Bat <= 3600){
            if (rest_timer < 120) { // if not charged, stay put
             next_state = 2;
             digitalWrite(8,true); 
             rest_timer++;         
            } else { // otherwise go to measure
              next_state = 1;
              digitalWrite(8,false);
              rest_timer = 0;
            }
        }else{
          next_state = 0;
          //send signal <UART> "cell fully charged" to control
          digitalWrite(8,false);
          digitalWrite(3, false);
          digitalWrite(4, false);
        }

            
        if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
          next_state = 0;
          digitalWrite(8,false);
          digitalWrite(3, false);
          digitalWrite(4, false);
        }
        break;
      }

      
   
      case 5: { // ERROR state RED led and no current
        current_ref = 0;
        next_state = 5; // Always stay here
        digitalWrite(7,true);
        digitalWrite(8,false);
        if(input_switch == 0){ //UNLESS the switch = 0, then go back to start
          next_state = 0;
          digitalWrite(7,false);
          digitalWrite(3, false);
          digitalWrite(4, false);
        }
        break;
      }



        case 3:{
          current_ref = 250;
          digitalWrite(3, true);// battery 1 red, relay D3, dis D9, meas A1
          digitalWrite(4, false);// battery 2 white, relay D4, dis D5, meas A6
            Vb1 = analogRead(A1) * 4.096 / 1.03;
            Vb2 = analogRead(A0) * 4.096 / 1.03;
          digitalWrite(9, true);
          if(rest_timer < 60){
             next_state = 3;
             rest_timer++; 
          }else{
            next_state = 1;
            digitalWrite(5, false);
            digitalWrite(9, false);
            //digitalWrite(3, false);
            //digitalWrite(4, false);
            rest_timer = 0;
          }


          
        if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
          next_state = 0;
          digitalWrite(8,false);
          digitalWrite(3, false);
          digitalWrite(4, false);
          }
          break;
            
     }




        case 4:{
          current_ref = 250;
          digitalWrite(3, false);// battery 1 red, relay D3, dis D9, meas A1
          digitalWrite(4, true);// battery 2 white, relay D4, dis D5, meas A6
            Vb1 = analogRead(A0) * 4.096 / 1.03;
            Vb2 = analogRead(A6) * 4.096 / 1.03;
          digitalWrite(5, true);
          if(rest_timer < 60){
             next_state = 4;
             rest_timer++; 
          }else{
            next_state = 1;
            digitalWrite(5, false);
            digitalWrite(9, false);
            rest_timer = 0;
            //digitalWrite(3, false);
            //digitalWrite(4, false);
          }


          
        if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
          next_state = 0;
          digitalWrite(8,false);
          digitalWrite(3, false);
          digitalWrite(4, false);
          }
          break;
            
     }

     //case 6:{  UART communication state
          //current_ref = 0;
          //digitalWrite(3, true);// battery 1 red, relay D3, dis D9, meas A1
          //digitalWrite(4, true);// battery 2 white, relay D4, dis D5, meas A6
           // Vb1 = analogRead(A1) * 4.096 / 1.03;
           // Vb2 = analogRead(A6) * 4.096 / 1.03;
          // send UART Vb1 Vb2 OCV values to control for SOC determination
          //if(<UART> idle_on = 0){
          //       next_state = 0;
          //          }
     //}



      default :{ // Should not end up here ....
        Serial.println("Boop");
        current_ref = 0;
        next_state = 5; // So if we are here, we go to error
        digitalWrite(7,true);
      }
      
    }
    
    dataString = "state_num = " + String(state_num) + ", " 
                 + "V_Bat = " + String(V_Bat) + ", " 
                 + "current_ref = " + String(current_ref) + ", " 
                 + "current_measure = " + String(current_measure)+ ", " 
                 + "vb1 = " + String(Vb1)+ ", " 
                 + "vb2 = " + String(Vb2) + ", " 
                 + "rly1 = " + String(b1rly) + ", "
                 + "rly2 = " + String(b2rly) + ", "
                 + "dis1 = " + String(b1dis) + ", "
                 + "dis2 = " + String(b2dis) + ", "
                 + "duty = " + String(pwm_out)+", "
                 + "power = " + String(p_present);

                 //build a datastring for the CSV file

       Serial.println(dataString);          
                 
     //Serial.println("state_num = " + String(state_num) + "," );
     //Serial.println("V_Bat = " + String(V_Bat) + ",");
     //Serial.println("current_ref = " + String(current_ref) + "," );
     //Serial.println("current_measure = " + String(current_measure)+ "," );
     //Serial.println("vb1 = " + String(Vb1)+ "," );
     //Serial.println("vb2 = " + String(Vb2) + "," );
     //Serial.println("rly1 = " + String(b1rly) + ",");
     //Serial.println("rly2 = " + String(b2rly) + ",");
     //Serial.println("dis1 = " + String(b1dis) + ",");
     //Serial.println("dis2 = " + String(b2dis) + ",");
     //Serial.println("duty = " + String(pwm_out));
     //Serial.println(" ");
 
    
    File dataFile = SD.open("BatCycle.csv", FILE_WRITE); // open our CSV file
    if (dataFile){ //If we succeeded (usually this fails if the SD card is out)
      dataFile.println(dataString); // print the data
    } else {
      Serial.println("File not open"); //otherwise print an error
    }
    dataFile.close(); // close the file
    int_count = 0; // reset the interrupt count so we dont come back here for 1000ms
  }
}

// Timer A CMP1 interrupt. Every 1000us the program enters this interrupt. This is the fast 1kHz loop
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

float pidi(float pid_input) { // discrete PID function
  float e_integration;
  e0i = pid_input;
  e_integration = e0i;

  //anti-windup
  if (u1i >= ui_max) {
    e_integration = 0;
  } else if (u1i <= ui_min) {
    e_integration = 0;
  }

  delta_ui = kpi * (e0i - e1i) + kii * Ts * e_integration + kdi / Ts * (e0i - 2 * e1i + e2i); //incremental PID programming avoids integrations.
  u0i = u1i + delta_ui;  //this time's control output

  //output limitation
  saturation(u0i, ui_max, ui_min);

  u1i = u0i; //update last time's control output
  e2i = e1i; //update last last time's error
  e1i = e0i; // update last time's error
  return u0i;
}
