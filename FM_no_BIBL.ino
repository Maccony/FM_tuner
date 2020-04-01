// ARDUINO PB1 - Si4702 RST, (PC5) A5 - SCLK, (PC4) A4 - SDIO
byte registers_FM[28];

void setup() {
  reset_Si4703(); // сброс Si4703;
  TWBR=0x20; // задаем скорость передачи (при 8 мГц получается 100 кГц)
  readRegs();
  registers_FM[26] |= (1<<7); //запуск внутреннего генератора, включение бита XOSCEN (бит 26)
  writeRegs();
  delay(500);
  readRegs();
  registers_FM[16] = 0x40;//регистр 0х02 (старший байт) бит 14 -> DMUTE = 1 (Mute disable)
  registers_FM[17] = 0x01;//регистр 0х02 (младший байт) бит 1 -> ENABLE = 1 (Powerup Enable)
  writeRegs();
  delay(110);
  readRegs();
  registers_FM[20] |= (1<<3);//регистр 0х04h - старший байт (8 байт массива), бит D11 -> DE = 1 (De-emphasis Russia 50 мкс).
  registers_FM[23] = 0x1b;//регистр 0х05h младьший байт (11 байт массива)
  //биты 7:6 -> BAND = [00] (Band Select),биты 5:4 -> SPACE = [01] (Channel Spacing), 100 kHz для России, биты 3:0 -> VOLUME
  writeRegs();
  delay(110);

  gotoChannel(171); //171, 181, 139, 189
}
void loop() {}

void gotoChannel(int newChannel){
	readRegs();//считываем регистры si4703
	registers_FM[18] = 0x80;//регистр 0х03h старший байт (6 байт массива), бит D15 -> TUNE = 1
	registers_FM[19] = newChannel;//регистр 0х03h младьший байт (7 байт массива), CHANNEL
	writeRegs();//записываем регистры
   	//These steps come from AN230 page 20 rev 0.5
	delay(60); //рекомендуемая минимальная задержка для выполнения установок

    //когда настройка на волну закончится, бит STC в регистре 0Ah установится в 1
    while(1) { //бесконечный цикл, прерывание через break
        readRegs();
        if( (registers_FM[0] & (1<<6)) != 0) break; //Tuning complete!
    }

    readRegs();
    registers_FM[18] = 0; //очистим вит TUNE в регистре 03h когда частота настроена
    writeRegs();

    //Wait for the si4703 to clear the STC as well
    while(1) {
        readRegs();
        if( (registers_FM[18] & (1<<6)) == 0) break; //Tuning complete!
    }
}

void readRegs(void) {
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTA); // отправляем условие "СТАРТ" в шину TWI (регистр TWI в ATmega328)
    while(!(TWCR & (1<<TWINT)));  // ожидаем пока "СТАРТ" отправится
    TWDR = 0B00100001; // в TWDR загружаем 0х10 - адрес Si7703 и флаг чтения (1)
    TWCR = (1<<TWINT)|(1<<TWEN); // сбрасываем бит прерывания TWINT (ставим в 1), активируем шину TWI установкой TWEN
    while(!(TWCR & (1<<TWINT))); // ожидаем когда TWINT обнулится аппаратно (закончится выполнение операции отправки SLA)
    for(byte count = 0; count < 28; count++) {
        TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA); // активируем TWEA чтоб подтвердить прием байта
        while(!(TWCR & (1<<TWINT))); // ожидаем когда TWINT обнулится аппаратно
        registers_FM[count] = TWDR;} // сохраняем принятый байт в массив
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);} // формируем "СТОП" установив TWSTO

void writeRegs(void) { //запись в si4703 начинается с регистра 0x02. Но нет записи в регистры 0x08 и 0x09
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTA); // формируем "СТАРТ" установив TWSTA
    while(!(TWCR & (1<<TWINT)));  // ожидаем пока "СТАРТ" отправится
    registers_FM[15] = 0B00100000; // в TWDR загружаем 0х10 - адрес Si7703 и флаг записи (0) 10000|0
    for(byte count = 15; count < 28; count++) {
        TWDR = registers_FM[count];
        TWCR = (1<<TWINT)|(1<<TWEN); // сбрасываем бит прерывания TWINT (ставим в 1), активируем шину TWI установкой TWEN
        while(!(TWCR & (1<<TWINT)));} // ожидаем когда TWINT обнулится аппаратно (закончится выполнение операции отправки SLA)
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);} // формируем "СТОП" установив TWSTO

void reset_Si4703(void) {
    DDRB |= (1 << DDB1); // через регистр направления DDRB назначаем вывод PB1 выходным (OUTPUT), ставим бит №1 в HIGH
    PORTB |= (1 << PORTB1); // через регистр данных порта PORTB устанавливаем на выводе PB1 значение HIGH
    delay(1);} // время что бы выйти из сброса
