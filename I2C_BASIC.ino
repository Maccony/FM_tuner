// ARDUINO PB1 - Si4702 RST
//(PC5) A5 - SCLK
//(PC4) A4 - SDIO
#define address_SI4703 0x10
#define START_TWSR 0x08

void setup() {
  Serial.begin(9600); //устанавливаем последовательное соединение

  TWBR=0x20; //скорость передачи (при 8 мГц получается 100 кГц)
  I2C_StartCondition();
}

void loop() {
    Serial.println(TWCR);
    delay(1000);
}

void I2C_StartCondition(void) {
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN); // отправка "СТАРТ"
    while(!(TWCR&(1<<TWINT)));  // ожидаем пока "СТАРТ" отправится
}
