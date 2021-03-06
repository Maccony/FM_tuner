#include <Wire.h>

// управляется порт ввода-вывода общего назначения тремя регистрами (DDRB, PINB, PORTB)
const byte buttonPin = 12; //D12 PB4(12pin)
const byte resetPin = 9; //D9 PB1(9pin) (Arduino) - RST (Si4703)
const byte address_SI4703 = 0x10; //I2C адрес (7-бит) Si4703

byte massiv_reg[32]; //Выделяем массив для 16 регистров (размер регистра 16 бит => 2 байт)
unsigned long myTimer; // создаем переменную для таймера (переполнение таймера через 49.7 суток)

void setup(){
    Wire.begin(); //Инициализация библиотеки Wire и подключение контроллера к шине I2C в качестве мастера
    func_Reset_si4703(); //сброс si4703 (теперь регистры доступны на запись и чтение)

	func_Read_Regs(); //считываем регистры si4703
	massiv_reg[26] |= (1<<7);//запуск внутреннего генератора, включение бита XOSCEN (бит 26).
	func_Writ_Regs();//записываем регистры
	if (millis() - myTimer >= 500) {   // таймер на 500 мс (2 раза в сек)
    		myTimer = millis();
	}
	//delay(500);
	//рекомендуемая минимальная задержка для стабилизации внутреннего генератора

	func_Read_Regs(); //считываем регистры si4703
	massiv_reg[16] = 0x40;//регистр 0х02 (старший байт) бит 14 -> DMUTE = 1 (Mute disable)
	massiv_reg[17] = 0x01;//регистр 0х02 (младший байт) бит 1 -> ENABLE = 1 (Powerup Enable)
	func_Writ_Regs();//записываем регистры
	if (millis() - myTimer >= 110) {   // таймер на 110 мс
    		myTimer = millis();
	}
	//delay(110);
	//рекомендуемая минимальная задержка для стабилизации

	func_Read_Regs(); //считываем регистры si4703
	massiv_reg[20] |= (1<<3);//регистр 0х04h - старший байт (8 байт массива), бит D11 -> DE = 1 (De-emphasis Russia 50 мкс).
	massiv_reg[23] = 0x1b;//регистр 0х05h младьший байт (11 байт массива)
	//биты 7:6 -> BAND = [00] (Band Select),биты 5:4 -> SPACE = [01] (Channel Spacing), 100 kHz для России, биты 3:0 -> VOLUME
	func_Writ_Regs();//записываем регистры
	if (millis() - myTimer >= 110) {   // таймер на 110 мс
    		myTimer = millis();
	}
	//delay(110);
	//рекомендуемая минимальная задержка для выполнения установок

	gotoChannel(171);
}

void loop(){
}

void gotoChannel(int newChannel){
	func_Read_Regs();//считываем регистры si4703
	massiv_reg[18] = 0x80;//регистр 0х03h старший байт (6 байт массива), бит D15 -> TUNE = 1
	massiv_reg[19] = newChannel;//регистр 0х03h младьший байт (7 байт массива), CHANNEL
	func_Writ_Regs();//записываем регистры
   	//These steps come from AN230 page 20 rev 0.5
	if (millis() - myTimer >= 60) {   // таймер на 60 мс
    		myTimer = millis();
	}
    	//delay(60);
	//рекомендуемая минимальная задержка для выполнения установок

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
    int max = Wire.requestFrom(address_SI4703, 32); //Запрашиваем 32 байта у ведомого устройства с адресом 0х10 (SI4703).
    for (int x = 0; x < max; x++) { //считываем байты из регистров в массив
	    massiv_reg[x] = Wire.read(); //считываем байт переданный от ведомого устройства I2C
	}
}

//ФУНКЦИЯ ЗАПИСИ В РЕГИСТРЫ
void func_Writ_Regs(void) {
    Wire.beginTransmission(address_SI4703); //мастер I2C готовится к передаче данных на указанный адрес
        //запись в si4703 начинается с регистра 0x02. Но мы не должны писать в регистры 0x08 и 0x09
    for(int x = 16 ; x < 28 ; x++) { //из массива выбираем байты регистров 0x02 - 0x07
        Wire.write(massiv_reg[x]); //создаем очередь на запись в регистры от 0x02 до 0x07 (сначала старшый байт потом младший)
    }
    Wire.endTransmission();//заканчиваем передачу данных
}

//ФУНКЦИЯ СБРОСА Si4703 по I2C
void func_Reset_si4703(void){
    pinMode(resetPin, OUTPUT);// задаем режим работы D2 на выход (соединен с RST)
    digitalWrite(resetPin, LOW); //после чего перезагружаем Si4703
        delay(1); //небольшая задержка что бы уровни успели выставится
    digitalWrite(resetPin, HIGH); //возвратим Si4703 в исходное состояние, оставив SDIO=0 и SEN=1(с помощью встроенного резистора).
        delay(1); //дадим Si4703 время что бы выйти из сброса
}
