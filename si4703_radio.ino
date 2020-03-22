/* Чтобы использовать этот код, соедините следующие контакты: 
   Arduino - плата Si470x 
     3.3 В - VCC 
       GND - GND 
        A5 - SCLK 
        A4 - SDIO 
        D2 - RST 
 */
#include <Wire.h>

//КОНСТАНТЫ
const int STATUS_LED = 13;
const int resetPin = 2; //D2 (Arduino) - RST (Si4703) 
const int SDIO = A4;    //SDA (A4 на Arduino) - SDIO (Si4703) 
const int SCLK = A5;    //SCL (A5 на Arduino) - SCLK (Si4703)
const int address_SI4703 = 0x10; //I2C адрес (7-бит) Si4703

//ПЕРЕМЕННЫЕ
byte massiv_reg[32]; //Выделяем массив для 16 регистров (размер регистра 16 бит) - только чтение

void setup(){
	Serial.begin(9600); //Инициализируем последовательный интерфейс и ожидаем открытия порта:
    while (!Serial); // ожидаем подключение последовательного порта.

    Wire.begin(); //Инициализация библиотеки Wire и подключение контроллера к шине I2C в качестве мастера
    func_reset_si4703(); //сброс si4703 (теперь регистры доступны на запись и чтение)

	func_readRegisters_si4703(); //считываем регистры si4703
	    func_view_serial("Default"); //вывод регистры по умолчанию
	massiv_reg[26] |= (1<<7);//запуск внутреннего генератора, включение бита XOSCEN (бит 26).
	    func_view_serial("XOSCEN in massiv");//вывод XOSCEN в массиве
	func_updateRegisters();//записываем регистры
	delay(500); //рекомендуемая минимальная задержка для стабилизации внутреннего генератора
	func_readRegisters_si4703(); //считываем регистры si4703
	    func_view_serial("XOSCEN in register"); //вывод регистры по умолчанию
	massiv_reg[16] = 0x40;//регистр 0х02 (старший байт) бит 14 -> DMUTE = 1 (Mute disable)
	massiv_reg[17] = 0x01;//регистр 0х02 (младший байт) бит 1 -> ENABLE = 1 (Powerup Enable)
	    func_view_serial("ON in massiv");	
	func_updateRegisters();//записываем регистры
	delay(110); //рекомендуемая минимальная задержка для стабилизации
	
	
	func_readRegisters_si4703(); //считываем регистры si4703
	    func_view_serial("ON in register");	
	massiv_reg[20] |= (1<<3);//регистр 0х04h - старший байт (8 байт массива), бит D11 -> DE = 1 (De-emphasis Russia 50 мкс).
	//(De-emphasis) частотные предискажения, для России принято 50 мкс
	massiv_reg[23] = 0x1b;//регистр 0х05h младьший байт (11 байт массива) 
	//биты 7:6 -> BAND = [00] (Band Select),биты 5:4 -> SPACE = [01] (Channel Spacing), 100 kHz для России, биты 3:0 -> VOLUME
		func_view_serial("BAND in massiv");
	func_updateRegisters();//записываем регистры
	delay(110);//рекомендуемая минимальная задержка для выполнения установок	
	func_readRegisters_si4703(); //считываем регистры si4703
	    func_view_serial("BAND in register"); //вывод регистры по умолчанию	
} 

void loop(){
    Serial.println("Scanning...");
	
	gotoChannel(23);
	
    delay(5000);
}

void gotoChannel(int newChannel){
	
	func_readRegisters_si4703();//считываем регистры si4703
	massiv_reg[18] = 0x80;//регистр 0х03h старший байт (6 байт массива), бит D15 -> TUNE = 1 
	massiv_reg[19] = newChannel;//регистр 0х03h младьший байт (7 байт массива), CHANNEL
		func_view_serial("CHANNEL in massiv");
	func_updateRegisters();//записываем регистры
    //These steps come from AN230 page 20 rev 0.5
    delay(60);//рекомендуемая минимальная задержка для выполнения установок

    //когда настройка на волну закончится, бит STC в регистре 0Ah установится в 1
    while(1) { //бесконечный цикл, прерывание через break
        func_readRegisters_si4703();
        if( (massiv_reg[0] & (1<<6)) != 0) break; //Tuning complete!
        Serial.println("Tuning");
    }

    func_readRegisters_si4703();
    massiv_reg[18] = 0; //очистим вит TUNE в регистре 03h когда частота настроена
    func_updateRegisters();

    //Wait for the si4703 to clear the STC as well
    while(1) {
        func_readRegisters_si4703();
        if( (massiv_reg[18] & (1<<6)) == 0) break; //Tuning complete!
        Serial.println("Waiting...");
    }
}

//ФУНКЦИЯ ОТОБРАЖЕНИЯ
void func_view_serial(char str[]) {
	char reg_name[16] = {'A', 'B', 'C', 'D', 'E', 'F', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	int k = 0;
	Serial.println();
	Serial.println(str);
	for (int x = 0; x <32; x+=2){
		if ( x > 0 ) k = x/2;
		Serial.print(x);
		Serial.print("hi ");
		Serial.print(x+1);
		Serial.print("lo ");		
		Serial.print("0x0");
		Serial.print(reg_name[k]);
		Serial.print(": ");
	    Serial.print(massiv_reg[x], HEX); //отображение в serial в шестнадцатеричном виде
	    Serial.print(" "); //отображение в serial в шестнадцатеричном виде		
	    Serial.println(massiv_reg[x+1], HEX); //отображение в serial в шестнадцатеричном виде		
    }	
}		
//ФУНКЦИЯ ЧТЕНИЯ ИЗ РЕГИСТРОВ (считывает весь набор регистров управления Si4703 (от 0x00 до 0x0F)
//чтение из Si4703 начинается с регистра 0x0A и до 0x0F, затем возвращается к 0x00 далее до 0х09
void func_readRegisters_si4703(){

    int max = Wire.requestFrom(address_SI4703, 32); //Запрашиваем 32 байта у ведомого устройства с адресом 0х10 (SI4703).
    
	Serial.println(max); // количество доступных байт в приемном регистре загруженных с подчиненного устройства I2C
    for (int x = 0; x < max; x++) { //считываем байты из регистров в массив
	    massiv_reg[x] = Wire.read(); //считываем байт переданный от ведомого устройства I2C
	}	
}
//ФУНКЦИЯ ЗАПИСИ В РЕГИСТРЫ
byte func_updateRegisters(void) {
    Wire.beginTransmission(address_SI4703); //мастер I2C готовится к передаче данных на указанный адрес
        //запись в si4703 начинается с регистра 0x02. Но мы не должны писать в регистры 0x08 и 0x09
		
    for(int x = 16 ; x < 28 ; x++) { //из массива выбираем байты регистров 0x02 - 0x07
        Wire.write(massiv_reg[x]); //создаем очередь на запись в регистры от 0x02 до 0x07 (сначала старшый байт потом младший)
    }

    byte error = Wire.endTransmission();//заканчиваем передачу данных
	Serial.println(error);
}
//ФУНКЦИЯ СБРОСА Si4703 по I2C
void func_reset_si4703(){
    pinMode(resetPin, OUTPUT);// задаем режим работы D2 на выход (соединен с RST)
    digitalWrite(resetPin, LOW); //после чего перезагружаем Si4703
        delay(1); //небольшая задержка что бы уровни успели выставится
    digitalWrite(resetPin, HIGH); //возвратим Si4703 в исходное состояние, оставив SDIO=0 и SEN=1(с помощью встроенного резистора).
        delay(1); //дадим Si4703 время что бы выйти из сброса	
}


