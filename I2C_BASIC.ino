// ARDUINO PB1 - Si4702 RST
//(PC5) A5 - SCLK
//(PC4) A4 - SDIO
#define address_SI4703 0x10
#define START_TWI 0x08

void setup() {
  Serial.begin(9600); //устанавливаем последовательное соединение
  TWBR=0x20; //скорость передачи (при 8 мГц получается 100 кГц)
}

void loop() {
    I2C_StartCondition(); // отправляем в шину "СТАРТ"
    Serial.println(TWSR, HEX); // отображаем статусный регистр TWSR
    delay(1000);
    I2C_SRA_R();
    Serial.println(TWSR, HEX); // отображаем статусный регистр TWSR
    delay(1000);
    I2C_READ();
    I2C_StopCondition();
    delay(1000);
}

void I2C_StartCondition(void) {
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN); // отправка "СТАРТ"
    while(!(TWCR&(1<<TWINT)));  // ожидаем пока "СТАРТ" отправится
    if ((TWSR & 0xF8) != START_TWI) Serial.println("ERROR START");
}

void I2C_StopCondition(void) {
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
}

void I2C_SRA_R(void) { // выдаем на шину пакет SLA-R
    TWDR = (address_SI4703<<1)|1;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while(!(TWCR & (1<<TWINT)));
}

void I2C_READ(void) { /*считываем данные с подтверждением*/
    TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
    while(!(TWCR & (1<<TWINT)));
    data = TWDR;
    Serial.println(data , HEX); // отображаем полученый байт
    delay(5000);
}
