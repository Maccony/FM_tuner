// ARDUINO PB1 - Si4702 RST
//(PC5) A5 - SCLK
//(PC4) A4 - SDIO
byte registers_FM[32];

void setup() {
  Serial.begin(9600); //устанавливаем последовательное соединение
  reset_Si4703(); // сброс Si4703;
  TWBR=0x20; // задаем скорость передачи (при 8 мГц получается 100 кГц)
  readRegs();
  for(byte count=0; count<28; count++) Serial.println(registers_FM[count], HEX);
}
void loop() {}

void readRegs(void) {
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTA); // сбрасываем бит прерывания TWINT (ставим в 1), активируем шину TWI установкой TWEN
    while(!(TWCR & (1<<TWINT)));  // ожидаем пока "СТАРТ" отправится
    TWDR = 0B00100001; // в TWDR загружаем 0х10 - адрес Si7703
    TWCR = (1<<TWINT)|(1<<TWEN); // сбрасываем бит прерывания TWINT (ставим в 1), активируем шину TWI установкой TWEN
    while(!(TWCR & (1<<TWINT))); // ожидаем когда TWINT обнулится аппаратно (закончится выполнение операции отправки SLA)
    for(byte count = 0; count < 28; count++) {
        TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA); // сбрасываем бит прерывания TWINT (ставим в 1), активируем шину TWI установкой TWEN
        while(!(TWCR & (1<<TWINT))); // ожидаем когда TWINT обнулится аппаратно (закончится выполнение операции отправки SLA)
        registers_FM[count] = TWDR;}
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO); // формируем "СТОП" установив TWSTO
}

//запись в si4703 начинается с регистра 0x02. Но мы не должны писать в регистры 0x08 и 0x09
void writeRegs(void) {
    // формируем "СТАРТ" установив TWSTA
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTA); // сбрасываем бит прерывания TWINT (ставим в 1), активируем шину TWI установкой TWEN
    while(!(TWCR & (1<<TWINT)));  // ожидаем пока "СТАРТ" отправится
    registers_FM[15] = 0B00100000; // в TWDR загружаем 0х10 - адрес Si7703 и флаг записи (0) 10000|0
    for(byte count = 15; count < 28; count++) {
        TWDR = registers_FM[count];
        TWCR = (1<<TWINT)|(1<<TWEN); // сбрасываем бит прерывания TWINT (ставим в 1), активируем шину TWI установкой TWEN
        while(!(TWCR & (1<<TWINT)));} // ожидаем когда TWINT обнулится аппаратно (закончится выполнение операции отправки SLA)
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO); // формируем "СТОП" установив TWSTO
    Serial.println("WRITE OK");
}

void reset_Si4703(void) {
    DDRB |= (1 << DDB1); // через регистр направления DDRB назначаем вывод PB1 выходным (OUTPUT), ставим бит №1 в HIGH
    PORTB |= (1 << PORTB1); // через регистр данных порта PORTB устанавливаем на выводе PB1 значение HIGH
    delay(1);} // время что бы выйти из сброса
