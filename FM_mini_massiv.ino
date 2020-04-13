// ARDUINO PB1 - Si4702 RST, (PC5) A5 - SCLK, (PC4) A4 - SDIO
// 24(звезда), 28(комеди радио), 36(дорожное радио), 40(комс. правда), 142(вести фм), 151(новое), 156(искатель), 163(европа), 171(ретро), 185(юмор), 196(авторадио), 200(дача)
byte registers_FM[12]; // задаем массив для считывания регистров Si4703
byte statusRSSI;
byte channels[12] = {24, 28, 36, 40, 142, 151, 156, 163, 171, 185, 196, 200};
byte number = 4;

void setup() {
    Serial.begin(9600);
    Serial.println(PORTB);
    reset_Si4703(); // сброс si4703 (теперь регистры доступны на запись и чтение)
    TWBR=0x20; // задаем скорость передачи (при 8 мГц получается 100 кГц)
    readRegs();
    registers_FM[10] |= (1<<7); //запуск внутреннего генератора, включение бита 15 -> XOSCEN (регистр 0х07h Test1)
    writeRegs();
    delay(500);
    //регистр 0х02 (старший байт) бит 14 -> DMUTE = 1 (Mute disable)
    //регистр 0х02 (младший байт) бит 0 -> ENABLE = 1 (Powerup Enable)
    updateRegs(0, 0x40, 1, 0x01, 110);
    //регистр 0х04h - старший байт, бит D11 -> DE = 1 (De-emphasis Russia 50 мкс).
    //регистр 0х05h младьший байт, биты 7:6 -> BAND = [00] (Band Select),биты 5:4 -> SPACE = [01] (Channel Spacing), 100 kHz для России, биты 3:0 -> VOLUME
    updateRegs(4, 0x08, 7, 0x1b, 110);
    //tuneChannel(channels[number]);
    //attachInterrupt(0, buttonPrint, RISING);
}
void loop() {
    //if((PINB & (1 << PINB4)) != 0) { //если на входе PB4 значение HIGH (кнопка нажата), то... = 16
    //    number = number + 1;
    //tuneChannel(channels[number]);}
    Serial.println(PORTB);
    delay(1000);
}

void updateRegs(byte numberByte1, byte valueByte1, byte numberByte2, byte valueByte2, byte delayByte) {
    readRegs();
    registers_FM[numberByte1] = valueByte1;
    registers_FM[numberByte2] = valueByte2;
    writeRegs();
    delay(delayByte);
}

void tuneChannel(byte newChannel) {
    // регистр 0х03h старший байт, бит D15 -> TUNE = 1
    // регистр 0х03h младьший байт, CHANNEL = нужный канал
    updateRegs(2, 0x80, 3, newChannel, 60);

    //когда настройка на волну закончится, в регистре 0Ah, бит STC установится в 1
    while(1) { // бесконечный цикл, прерывание через break
        readRegs();
        if((statusRSSI & (1<<6)) != 0) break;} //Tuning complete!

    registers_FM[2] &= ~(1<<7); //очистим бит TUNE в регистре 03h когда частота настроена
    writeRegs();
    while(1) { //Wait for the si4703 to clear the STC as well
        readRegs();
        if((statusRSSI & (1<<6)) == 0) break;} // Tuning complete!
}

//ФУНКЦИЯ ЧТЕНИЯ ИЗ РЕГИСТРОВ (считывает весь набор регистров управления Si4703 (от 0x00 до 0x0F)
//чтение из Si4703 начинается с регистра 0x0A и до 0x0F, затем возвращается к 0x00 далее до 0х07 (0х08-0х09 не читеем за ненадобностью)
void readRegs(void) {
    connect_TWI(0B00100001); // обращаемся к устройству 0х10 (адрес Si4703) и добавляем флаг чтения (1) 10000|1
    for(byte count = 0; count < 28; count++) {
        TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA); // активируем TWEA чтоб подтвердить прием байта
        while(!(TWCR & (1<<TWINT))); // ожидаем когда TWINT обнулится аппаратно
        if(count == 0) statusRSSI = TWDR;
        if(count > 15) registers_FM[count - 16] = TWDR;} // в массив сохраняем только байты регистров 0х02 - 0х07
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);} // формируем "СТОП" установив TWSTO
//запись в si4703 начинается с регистра 0x02. Но мы не должны писать в регистры 0x08 и 0x09
void writeRegs(void) {
    connect_TWI(0B00100000); // обращаемся к устройству 0х10 (адрес Si7703) и добавляем флаг записи (0) 10000|0
    for(byte count = 0; count < 12; count++) { // в si4703 запись возможна только в регистры 0x02 - 0х07 (12 байт)
        TWDR = registers_FM[count];
        TWCR = (1<<TWINT)|(1<<TWEN); // сбрасываем бит прерывания TWINT (ставим в 1), активируем шину TWI установкой TWEN
        while(!(TWCR & (1<<TWINT)));} // ожидаем когда TWINT обнулится аппаратно (закончится выполнение операции отправки SLA)
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);} // формируем "СТОП" установив TWSTO

void connect_TWI(byte address) {
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTA); // отправляем условие "СТАРТ" в шину TWI
    while(!(TWCR & (1<<TWINT)));  // ожидаем пока "СТАРТ" отправится
    TWDR = address; // в TWDR загружаем 0х10 - адрес Si4703 и флаг чтения/записи
    TWCR = (1<<TWINT)|(1<<TWEN); // сбрасываем бит прерывания TWINT (ставим в 1), активируем шину TWI установкой TWEN
    while(!(TWCR & (1<<TWINT)));} // ожидаем когда TWINT обнулится аппаратно (закончится выполнение операции отправки SLA)

void reset_Si4703(void) {// сброс si4703 (теперь регистры доступны на запись и чтение)
    DDRB |= (1 << DDB1); // через регистр направления DDRB назначаем вывод PB1 выходным (OUTPUT), ставим бит №1 в HIGH
    PORTB |= (1 << PORTB1); // через регистр данных порта PORTB устанавливаем на выводе PB1 значение HIGH
    delay(1);} // время что бы выйти из сброса
