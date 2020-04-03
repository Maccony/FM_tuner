// ARDUINO PB1 - Si4702 RST, (PC5) A5 - SCLK, (PC4) A4 - SDIO
byte registers_FM[12]; // задаем переменную для считывания регистров Si4703
byte statusRSSI;
byte high0A_Status; // TUNE, 0
byte high02_Power; // DMUTE, SEEK, 16
byte low02_Power; // ENABLE, 17
byte high03_Channel; // TUNE, 18
byte low03_Channel; // CHAN, 19
byte high04_System1; // DE, 20
byte low05_System2; // BAND, SPACE, VOLUME, 23
byte high07_Test; // XOSCEN, 26

void setup() {
    reset_Si4703(); // сброс si4703 (теперь регистры доступны на запись и чтение)
    TWBR=0x20; // задаем скорость передачи (при 8 мГц получается 100 кГц)
    readRegs();
    registers_FM[10] |= (1<<7); //запуск внутреннего генератора, включение бита 15 -> XOSCEN (регистр 0х07h Test1)
    writeRegs();
    delay(500);
    readRegs();
    registers_FM[0] |= (1<<6); //регистр 0х02 (старший байт) бит 14 -> DMUTE = 1 (Mute disable)
    registers_FM[1] |= (1<<0); //регистр 0х02 (младший байт) бит 0 -> ENABLE = 1 (Powerup Enable)
    writeRegs();
    delay(110);
    readRegs();
    registers_FM[4] |= (1<<3); //регистр 0х04h - старший байт (8 байт массива), бит D11 -> DE = 1 (De-emphasis Russia 50 мкс).
    registers_FM[7] = 0x1b; //регистр 0х05h младьший байт (11 байт массива)
    //биты 7:6 -> BAND = [00] (Band Select),биты 5:4 -> SPACE = [01] (Channel Spacing), 100 kHz для России, биты 3:0 -> VOLUME
    writeRegs();
    delay(110);

    gotoChannel(171); //171, 181, 139, 189
}
void loop() {
    if(PINB & (1 << PINB4)) { //если на входе PB4 значение HIGH (кнопка нажата), то...
        delay(500);
    }
}

void gotoChannel(byte newChannel){
	readRegs(); //считываем регистры si4703
	registers_FM[2] |= (1<<7);//регистр 0х03h старший байт, бит D15 -> TUNE = 1
	registers_FM[3] = newChannel;//регистр 0х03h младьший байт (7 байт массива), CHANNEL
	writeRegs();//записываем регистры
   	//These steps come from AN230 page 20 rev 0.5
	delay(60); //рекомендуемая минимальная задержка для выполнения установок

    //когда настройка на волну закончится, в регистре 0Ah, бит STC установится в 1
    while(1) { // бесконечный цикл, прерывание через break
        readRegs();
        if( (statusRSSI & (1<<6)) != 0) break;} //Tuning complete!
    registers_FM[2] &= ~(1<<7); //очистим бит TUNE в регистре 03h когда частота настроена
    writeRegs();
    while(1) { //Wait for the si4703 to clear the STC as well
        readRegs();
        if((statusRSSI & (1<<6)) == 0) break;} // Tuning complete!
}
//ФУНКЦИЯ ЧТЕНИЯ ИЗ РЕГИСТРОВ (считывает весь набор регистров управления Si4703 (от 0x00 до 0x0F)
//чтение из Si4703 начинается с регистра 0x0A и до 0x0F, затем возвращается к 0x00 далее до 0х09
void readRegs(void) {
    connect_TWI(0B00100001); // обращаемся к устройству 0х10 (адрес Si7703) и добавляем флаг чтения (1) 10000|1
    for(byte count = 0; count < 28; count++) {
        TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA); // активируем TWEA чтоб подтвердить прием байта
        while(!(TWCR & (1<<TWINT))); // ожидаем когда TWINT обнулится аппаратно
        switch (count) {
            case 0: high0A_Status = TWDR; break;
            case 16: high02_Power = TWDR; break;
            case 17: low02_Power = TWDR; break;
            case 18: high03_Channel = TWDR; break;
            case 19: low03_Channel = TWDR; break;
            case 20: high04_System1 = TWDR; break;
            case 23: low05_System2 = TWDR; break;
            case 26: high07_Test = TWDR; break;
        }
        if(count == 0) statusRSSI = TWDR;
        if(count > 15) registers_FM[count - 16] = TWDR;} // в массив сохраняем только байты регистров 0х02 - 0х07
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO); // формируем "СТОП" установив TWSTO

}
//запись в si4703 начинается с регистра 0x02. Но мы не должны писать в регистры 0x08 и 0x09
void writeRegs(void) {
    connect_TWI(0B00100000); // обращаемся к устройству 0х10 (адрес Si7703) и добавляем флаг записи (0) 10000|0
    for(byte count = 0; count < 12; count++) { // запись в si4703 начинается с регистра 0x02. Но нет записи в регистры 0x08 и 0x09
        TWDR = registers_FM[count];
        TWCR = (1<<TWINT)|(1<<TWEN); // сбрасываем бит прерывания TWINT (ставим в 1), активируем шину TWI установкой TWEN
        while(!(TWCR & (1<<TWINT)));} // ожидаем когда TWINT обнулится аппаратно (закончится выполнение операции отправки SLA)
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);} // формируем "СТОП" установив TWSTO

void connect_TWI(byte address) {
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTA); // отправляем условие "СТАРТ" в шину TWI (регистр TWI в ATmega328)
    while(!(TWCR & (1<<TWINT)));  // ожидаем пока "СТАРТ" отправится
    TWDR = address; // в TWDR загружаем 0х10 - адрес Si7703 и флаг чтения (1)
    TWCR = (1<<TWINT)|(1<<TWEN); // сбрасываем бит прерывания TWINT (ставим в 1), активируем шину TWI установкой TWEN
    while(!(TWCR & (1<<TWINT)));} // ожидаем когда TWINT обнулится аппаратно (закончится выполнение операции отправки SLA)

void reset_Si4703(void) {// сброс si4703 (теперь регистры доступны на запись и чтение)
    DDRB |= (1 << DDB1); // через регистр направления DDRB назначаем вывод PB1 выходным (OUTPUT), ставим бит №1 в HIGH
    PORTB |= (1 << PORTB1); // через регистр данных порта PORTB устанавливаем на выводе PB1 значение HIGH
    delay(1);} // время что бы выйти из сброса
