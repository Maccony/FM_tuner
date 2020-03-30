// ARDUINO PB1 - Si4702 RST
//(PC5) A5 - SCLK
//(PC4) A4 - SDIO
#define address_SI4703 0x10
#define START_TWI 0x08
byte data;

void setup() {
  Serial.begin(9600); //устанавливаем последовательное соединение
  reset_Si4703(); // сброс Si4703;
  TWBR=0x20; // задаем скорость передачи (при 8 мГц получается 100 кГц)
}
void loop() {
    send_START(); // отправляем в шину "СТАРТ"
    send_SLA_X(1); // отправляем в шину SLA(адрес ведомого) и 1-Read или 0-Write
    for(byte count=0; count<32; count++) {
        busTWI_READ();
        registers_FM[count] = TWDR;
    }
    send_STOP();
    for(byte count=0; count<32; count++) {Serial.println(registers_FM[count], HEX);}
}

void reset_Si4703(void) {
    DDRB |= (1 << DDB1); // через регистр направления DDRB назначаем вывод PB1 выходным (OUTPUT), ставим бит №1 в HIGH
    PORTB |= (1 << PORTB1); // через регистр данных порта PORTB устанавливаем на выводе PB1 значение HIGH
    delay(1);} // время что бы выйти из сброса

void send_START(void) {
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTA); // сбрасываем бит прерывания TWINT, активируем шину TWI установкой TWEN, формируем "СТАРТ" установив TWSTA
    while(!(TWCR&(1<<TWINT)));}  // ожидаем пока "СТАРТ" отправится

void send_STOP(void) {TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);} // формируем "СТОП" установив TWSTO

void send_SLA_X(byte orRW) { // выдаем на шину пакет SLA
    TWDR = (0x10<<1)|orRW; // в TWDR загружаем 0х10 - адрес Si7703
    TWCR = (1<<TWINT)|(1<<TWEN);
    while(!(TWCR & (1<<TWINT)));}

void busTWI_READ(void) { /*считываем данные с подтверждением*/
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA); // выставив TWEA ждем подтверждение (ACK) от ведомого
    while(!(TWCR & (1<<TWINT)));}
