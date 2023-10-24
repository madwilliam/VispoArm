 void setup() {
  Serial.begin(9600); 
}
 
void loop() {
  //Read analog pin
  int val = analogRead(A0);

  //Write analog value to serial port:
  Serial.print( val );      
  Serial.print("\n");
}