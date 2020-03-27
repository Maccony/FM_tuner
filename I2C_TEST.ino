// ARDUINO PB1 - Si4702 RST
//(PC5) A5 - SCLK
//(PC4) A4 - SDIO
#define address_SI4703 0x10
#define START_TWSR 0x08

void setup() {
  Serial.begin(9600); //устанавливаем последовательное соединение

  TWBR=0x20; //скорость передачи (при 8 мГц получается 100 кГц)
  TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN); // отправка "СТАРТ"
  while (!(TWCR & (1<<TWINT))); // ожидаем пока "СТАРТ" отправится
  if ((TWSR & 0xF8) != START_TWSR) Serial.println("START ERROR");
  TWDR = address_SI4703;
  TWCR = (1<<TWINT)|(1<<TWEN); // передача адреса
  while (!(TWCR & (1<<TWINT))); // ожидание передачи адреса
  if ((TWSR & 0xF8) != MT_SLA_ACK) Serial.println("ADDRESS ERROR")();

}

void loop() {
  // считываем данные с входа 0
  analogValue = analogRead(0);

  // выводим в различных базисах
  Serial.println(analogValue);       // print as an ASCII-encoded decimal
  Serial.println(analogValue, DEC);  // print as an ASCII-encoded decimal
  Serial.println(analogValue, HEX);  // print as an ASCII-encoded hexadecimal
  Serial.println(analogValue, OCT);  // print as an ASCII-encoded octal
  Serial.println(analogValue, BIN);  // print as an ASCII-encoded binary
  Serial.println(analogValue, BYTE); // print as a raw byte value

  delay(10);
}
