void setup() {
  pinMode(2, OUTPUT); //Sala
  pinMode(3, OUTPUT); //Comedor
  pinMode(4, OUTPUT); //Cocina
  pinMode(5, OUTPUT); //Baño
  pinMode(6, OUTPUT); //Habitación
}

void loop() {
  
  digitalWrite(2, HIGH); //Encender LED
  delay(1000);
  digitalWrite(2, LOW); // Apagar LED

  digitalWrite(3, HIGH); // Encender LED
  delay(1000);
  digitalWrite(3, LOW); // Apagar LED

  digitalWrite(4, HIGH); // Encender LED
  delay(1000);
  digitalWrite(4, LOW); // Apagar LED

  digitalWrite(5, HIGH); // Encender LED
  delay(1000);
  digitalWrite(5, LOW); // Apagar LED

  digitalWrite(6, HIGH); // Encender LED
  delay(1000);
  digitalWrite(6, LOW); // Apagar LED
}
