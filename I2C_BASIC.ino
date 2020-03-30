// ARDUINO PB1 - Si4702 RST
//(PC5) A5 - SCLK
//(PC4) A4 - SDIO
#define address_SI4703 0x10
#define START_TWI 0x08
byte data;

void setup() {
  Serial.begin(9600); //устанавливаем последовательное соединение
  DDRB |= (1 << DDB1); // через регистр направления DDRB назначаем вывод PB1 выходным (OUTPUT), ставим бит №1 в HIGH
  PORTB |= (1 << PORTB1); // через регистр данных порта PORTB устанавливаем на выводе PB1 значение HIGH
  delay(1); //дадим Si4703 время что бы выйти из сброса
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
    Serial.print("START OK - ");
}

void I2C_StopCondition(void) {
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
}

void I2C_SRA_R(void) { // выдаем на шину пакет SLA-R
    TWDR = (address_SI4703<<1)|1;
    Serial.println(TWDR, HEX);
    TWCR = (1<<TWINT)|(1<<TWEN);
    while(!(TWCR & (1<<TWINT)));
    Serial.print("SLA_R OK - ");
}

void I2C_READ(void) { /*считываем данные с подтверждением*/
    TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
    while(!(TWCR & (1<<TWINT)));
    Serial.print("DATA - ");
    data = TWDR;
    Serial.println(data, HEX); // отображаем полученый байт
    delay(5000);
}
