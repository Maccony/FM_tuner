// PD0-RXD, PD1-TXD = Serial; PD2-INT0, PD3-INT1 = Interrupt; PC4-SDA, PC5-SCL = TWI;
// ARDUINO PB1 - Si4702 RST, (PC5) A5 - SCLK, (PC4) A4 - SDIO
byte registers_FM[12]; // задаем массив для считывания регистров Si4703
void onBit(byte addrReg, byte numBit, byte timeState, byte valueOne); // прототип функции (правило хорошего тона)
void readRegs(void);
void writeRegs(void);
void updateRegs(void);
void reset_Si4703(void);
void connect_TWI(byte address);

void setup() { // функция setup выполняется один раз при загрузке
    DDRD &= ~(1 << DDD2); // в регистре направления DDRB назначаем вывод PD2 входным (INPUT), для отлова прерывания GPIO2 (STC)
    reset_Si4703(); // сброс si4703 (теперь регистры доступны на запись и чтение)
    TWBR=0x20; // задаем скорость передачи TWI (при 8 мГц получается 100 кГц)
    onBit(10, 7, 250, 1); // запуск внутреннего генератора, включение бита XOSCEN[15]=1 в регистре 0х07h
    onBit(1, 0, 55, 1); // регистр 0х02 бит ENABLE[0]=1 (Powerup Enable), включаем питание
    onBit(0, 6, 55, 1); // регистр 0х02 бит DMUTE[14]=1 (Mute disable), включаем звук
    updateRegs(); // настраиваем остальные регистры
    attachInterrupt(0, interruptTune, FALLING);} // прерывание GPIO2 от флага STC

void loop() {
    if (!(~PIND & (1 << PIND4))) onBit(0, 0, 1, 1); // если на входе PB4 значение HIGH (кнопка нажата), то поиск нужной радиостанции, регистр 0х02h установим бит SEEK=1
    delay(500);}

void interruptTune(void) {onBit(0, 0, 1, 0);} // функция реакции на прерывание GPIO2, очистим бит SEEK в регистре 02h когда частота настроена

void onBit(byte addrReg, byte numBit, byte timeState, byte valueOne) {
    readRegs();
    if(valueOne == 1) registers_FM[addrReg] |= (1 << numBit);
    else registers_FM[addrReg] &= ~(1 << numBit);
    writeRegs();
    delay(timeState + timeState);}

void updateRegs(void) {
    readRegs();
    registers_FM[4] = 0x48; // регистр 0х04h бит STCIEN[14]=1 (interrupt), бит DE[11]=1 (De-emphasis Russia 50 мкс)
    registers_FM[5] = 0x04; // регистр 0х05h биты GPIO2[3:2] = [01]
    registers_FM[7] = 0x1b; // регистр 0х05h биты BAND[7:6] = [00] (Band Select), биты SPACE[5:4] = [01] (Channel Spacing)-100 kHz для России, биты VOLUME[3:0]
    registers_FM[6] = 0x0C; // регистр 0х05h бит SEEKTH[15:8]=0x0C
    registers_FM[9] = 0x7F; // регистр 0х06h SKSNR[7:4]=0x07, SKCNT[3:0]=0x0F
    writeRegs();
    delay(110);}

//ФУНКЦИЯ ЧТЕНИЯ ИЗ РЕГИСТРОВ (считывает весь набор регистров управления Si4703 (от 0x00 до 0x0F)
//чтение из Si4703 начинается с регистра 0x0A и до 0x0F, затем возвращается к 0x00 далее до 0х07 (0х08-0х09 не читаем за ненадобностью)
void readRegs(void) {
    connect_TWI(0b00100001); // обращаемся к устройству 0х10 (адрес Si4703) и добавляем флаг чтения (1) 10000|1
    for(byte count = 0; count < 28; count++) {
        TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA); // активируем TWEA чтоб подтвердить прием байта
        while(!(TWCR & (1<<TWINT))); // ожидаем когда TWINT обнулится аппаратно
        if(count > 15) registers_FM[count - 16] = TWDR;} // в массив сохраняем только байты регистров 0х02 - 0х07
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);} // формируем "СТОП" установив TWSTO

void writeRegs(void) { // запись в si4703 начинается с регистра 0x02. Но мы не должны писать в регистры 0x08 и 0x09
    connect_TWI(0b00100000); // обращаемся к устройству 0х10 (адрес Si7703) и добавляем флаг записи (0) 10000|0
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

