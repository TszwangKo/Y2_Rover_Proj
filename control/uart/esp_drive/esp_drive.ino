#define RXD2 16
#define TXD2 17

void setup() {
  // Set up UART connection
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

void loop() {
  /* Loop instruction input */
  Serial.print("Enter Instruction: ")
  String instr = Serial.readString();
  Serial2.println(instr)
}
/* End of Program */
