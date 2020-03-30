// ARDUINO PB1 - Si4702 RST
//(PC5) A5 - SCLK
//(PC4) A4 - SDIO
byte registers_FM[32]; // выделяем массив для 16 регистров (размер регистра 16 бит => 2 байт)

void setup() {
  Serial.begin(9600); //устанавливаем последовательное соединение
  reset_Si4703(); // сброс si4703 (теперь регистры доступны на запись и чтение)
  TWBR=0x20; // задаем скорость передачи (при 8 мГц получается 100 кГц)
}
void loop() {
    send_START(); // отправляем в шину "СТАРТ"
    send_SLA_X(1); // отправляем в шину SLA(адрес ведомого) и 1-Read или 0-Write
    for(byte count=0; count<32; count++) { // читаем данные из регистров Si4703
        read_DATA();
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
    TWCR |= (1<<TWSTA); // формируем "СТАРТ" установив TWSTA
    bus_READY();}

void send_STOP(void) {TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);} // формируем "СТОП" установив TWSTO

void send_SLA_X(byte orRW) { // выдаем на шину пакет SLA
    TWDR = (0x10<<1)|orRW; // в TWDR загружаем 0х10 - адрес Si7703
    bus_READY();}

void read_DATA(void) { /*считываем данные с подтверждением*/
    TWCR |= (1<<TWEA);
    TWCR |= (1<<TWINT)|(1<<TWEN); // сбрасываем бит прерывания TWINT, активируем шину TWI установкой TWEN
    while(!(TWCR&(1<<TWINT)));}

void bus_READY(void) {
    TWCR |= (1<<TWINT)|(1<<TWEN); // сбрасываем бит прерывания TWINT, активируем шину TWI установкой TWEN
    while(!(TWCR&(1<<TWINT)));} // ожидаем пока "СТАРТ" отправится

void write_DATA(void) { /*считываем данные с подтверждением*/
    TWCR = (1<<TWINT)|(1<<TWEN); // выставив TWEA ждем подтверждение (ACK) от ведомого
    while(!(TWCR & (1<<TWINT)));}
