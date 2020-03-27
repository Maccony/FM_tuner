#include <Wire.h>

byte massiv_reg[32]; //Выделяем массив для 16 регистров (размер регистра 16 бит => 2 байта)
byte massiv_FM[6] = {23, 40, 142, 146, 156, 171};
byte counter = 0;

#define XOSCEN 7


void setup(){
    Wire.begin(); //Инициализация библиотеки Wire и подключение контроллера к шине I2C в качестве мастера
    DDRB |= (1 << DDB1); // через регистр направления DDRB назначаем вывод PB1 выходным (OUTPUT), ставим бит №1 в HIGH
    PORTB |= (1 << PORTB1); // через регистр данных порта PORTB устанавливаем на выводе PB1 значение HIGH
    delay(1); //дадим Si4703 время что бы выйти из сброса

    func_Read_Regs(); //считываем регистры si4703
    massiv_reg[26] |= (1<<7);//запуск внутреннего генератора, включение бита XOSCEN (бит 26).
    func_Writ_Regs();//записываем регистры
    delay(500); //рекомендуемая минимальная задержка для стабилизации внутреннего генератора

    func_Read_Regs(); //считываем регистры si4703
    massiv_reg[16] = 0x40;//регистр 0х02 (старший байт) бит 14 -> DMUTE = 1 (Mute disable)
    massiv_reg[17] = 0x01;//регистр 0х02 (младший байт) бит 1 -> ENABLE = 1 (Powerup Enable)
    func_Writ_Regs();//записываем регистры
    delay(110); //рекомендуемая минимальная задержка для стабилизации

    func_Read_Regs(); //считываем регистры si4703
    massiv_reg[20] |= (1<<3);//регистр 0х04h - старший байт (8 байт массива), бит D11 -> DE = 1 (De-emphasis Russia 50 мкс).
    massiv_reg[23] = 0x1b;//регистр 0х05h младьший байт (11 байт массива)
    //биты 7:6 -> BAND = [00] (Band Select),биты 5:4 -> SPACE = [01] (Channel Spacing), 100 kHz для России, биты 3:0 -> VOLUME
    func_Writ_Regs();//записываем регистры
    delay(110);//рекомендуемая минимальная задержка для выполнения установок

    //DDRB = DDRB | B0010000; // назначаем вывод PB4 входным INPUT (у остальных выводов значение не меняем)
    DDRB &= ~(1 << 4); // назначаем вывод PB4 входным INPUT, установим PB4 в LOW.
    gotoChannel(massiv_FM[counter]);
}

void loop(){
    if(PINB & (1 << 4)) { //если на входе PB4 значение HIGH (кнопка нажата), то...
        counter = counter + 1;
        gotoChannel(massiv_FM[counter]);
        delay(500);
        if(counter > 4)  counter = 0;
    }
}

void gotoChannel(byte newChannel){
    func_Read_Regs();//считываем регистры si4703
    massiv_reg[18] = 0x80;//регистр 0х03h старший байт (6 байт массива), бит D15 -> TUNE = 1
    massiv_reg[19] = newChannel;//регистр 0х03h младьший байт (7 байт массива), CHANNEL
    func_Writ_Regs();//записываем регистры
    //These steps come from AN230 page 20 rev 0.5
    delay(60);//рекомендуемая минимальная задержка для выполнения установок

    //когда настройка на волну закончится, бит STC в регистре 0Ah установится в 1
    while(1) { //бесконечный цикл, прерывание через break
        func_Read_Regs();
        if( (massiv_reg[0] & (1<<6)) != 0) break; //Tuning complete!
    }

    func_Read_Regs();
    massiv_reg[18] = 0; //очистим вит TUNE в регистре 03h когда частота настроена
    func_Writ_Regs();

    //Wait for the si4703 to clear the STC as well
    while(1) {
        func_Read_Regs();
        if( (massiv_reg[18] & (1<<6)) == 0) break; //Tuning complete!
    }
}

//ФУНКЦИЯ ЧТЕНИЯ ИЗ РЕГИСТРОВ (считывает весь набор регистров управления Si4703 (от 0x00 до 0x0F)
//чтение из Si4703 начинается с регистра 0x0A и до 0x0F, затем возвращается к 0x00 далее до 0х09
void func_Read_Regs(void){
    byte max = Wire.requestFrom(0x10, 32); //Запрашиваем 32 байта у ведомого устройства с адресом 0х10 (SI4703).
    for (byte x = 0; x < max; x++) { //считываем байты из регистров в массив
      massiv_reg[x] = Wire.read(); //считываем байт переданный от ведомого устройства I2C
  }
}

//ФУНКЦИЯ ЗАПИСИ В РЕГИСТРЫ
void func_Writ_Regs(void) {
    Wire.beginTransmission(0x10); //мастер I2C готовится к передаче данных на указанный адрес
        //запись в si4703 начинается с регистра 0x02. Но мы не должны писать в регистры 0x08 и 0x09
    for(byte x = 16 ; x < 28 ; x++) { //из массива выбираем байты регистров 0x02 - 0x07
        Wire.write(massiv_reg[x]); //создаем очередь на запись в регистры от 0x02 до 0x07 (сначала старшый байт потом младший)
    }
    Wire.endTransmission();//заканчиваем передачу данных
}
