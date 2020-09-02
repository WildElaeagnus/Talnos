/*
  Блок электроники для крутого моддинга вашего ПК, возможности:
  - Вывод основных параметров железа на внешний LCD дисплей
  - Температура: CPU, GPU, материнская плата, самый горячий HDD
  - Уровень загрузки: CPU, GPU, RAM, видеопамять
  - Графики изменения вышеперечисленных параметров по времени
  - Температура с внешних датчиков (DS18B20)
  - Текущий уровень скорости внешних вентиляторов
  - Управление большим количеством 12 вольтовых 2, 3, 4 проводных вентиляторов
  - Автоматическое управление скоростью пропорционально температуре
  - Ручное управление скоростью из интерфейса программы
  - Управление RGB светодиодной лентой
  - Управление цветом пропорционально температуре (синий - зелёный - жёлтый - красный)
  - Ручное управление цветом из интерфейса программы

  Программа HardwareMonitorPlus  https://github.com/AlexGyver/PCdisplay
  - Запустить OpenHardwareMonitor.exe
  - Options/Serial/Run - запуск соединения с Ардуиной
  - Options/Serial/Config - настройка параметров работы
   - PORT address - адрес порта, куда подключена Ардуина
   - TEMP source - источник показаний температуры (процессор, видеокарта, максимум проц+видео, датчик 1, датчик 2)
   - FAN min, FAN max - минимальные и максимальные обороты вентиляторов, в %
   - TEMP min, TEMP max - минимальная и максимальная температура, в градусах Цельсия
   - Manual FAN - ручное управление скоростью вентилятора в %
   - Manual COLOR - ручное управление цветом ленты
   - LED brightness - управление яркостью ленты
   - CHART interval - интервал обновления графиков

  Что идёт в порт: 0-CPU temp, 1-GPU temp, 2-mother temp, 3-max HDD temp, 4-CPU load, 5-GPU load, 6-RAM use, 7-GPU memory use,
  8-maxFAN, 9-minFAN, 10-maxTEMP, 11-minTEMP, 12-manualFAN, 13-manualCOLOR, 14-fanCtrl, 15-colorCtrl, 16-brightCtrl, 17-LOGinterval, 18-tempSource
*/
// ------------------------ НАСТРОЙКИ ----------------------------
// настройки пределов скорости и температуры по умолчанию (на случай отсутствия связи)
byte speedMIN = 60, speedMAX = 90, tempMIN = 30, tempMAX = 70;
#define DRIVER_VERSION 0    // 0 - маркировка драйвера кончается на 4АТ, 1 - на 4Т
#define COLOR_ALGORITM 0    // 0 или 1 - разные алгоритмы изменения цвета (строка 222)
#define ERROR_DUTY 90       // скорость вентиляторов при потере связи
#define ERROR_BACKLIGHT 0   // 0 - гасить подсветку при потере сигнала, 1 - не гасить
#define ERROR_UPTIME 0      // 1 - сбрасывать uptime при потере связи, 0 - нет
// ------------------------ НАСТРОЙКИ ----------------------------

// ----------------------- ПИНЫ ---------------------------

#define FAN_PIN_0 9           // на мосфет вентиляторов
#define FAN_PIN_1 10           
#define FAN_PIN_2 11
#define LED_FAN_PIN 5

#define TSD_0 98 //нидние пороги вентилляторов 0-100
#define TSD_1 99
#define TSD_2 20
#define LOW_TSD_2 98
         

//#define R_PIN 5             // на мосфет ленты, красный
//#define G_PIN 4             // на мосфет ленты, зелёный
//#define B_PIN 6             // на мосфет ленты, синий
#define BTN1 A3             // первая кнопка
#define BTN2 A2             // вторая кнопка
//#define SENSOR_PIN 14       // датчик температуры
// ----------------------- ПИНЫ ---------------------------

// -------------------- БИБЛИОТЕКИ ---------------------
#include <OneWire.h>            // библиотека протокола датчиков
#include <DallasTemperature.h>  // библиотека датчика
#include <string.h>             // библиотека расширенной работы со строками
#include <Wire.h>               // библиотека для соединения
#include <LiquidCrystal_I2C.h>  // библтотека дислея
#include <TimerOne.h>           // библиотека таймера
#include "SoftPWM.h"            // библа для диода питания чтоб глаза не жег
// -------------------- БИБЛИОТЕКИ ---------------------

// -------- АВТОВЫБОР ОПРЕДЕЛЕНИЯ ДИСПЛЕЯ-------------
// Если кончается на 4Т - это 0х27. Если на 4АТ - 0х3f
#if (DRIVER_VERSION)
LiquidCrystal_I2C lcd(0x27, 20, 4);
#else
LiquidCrystal_I2C lcd(0x3f, 20, 4);
#endif
// -------- АВТОВЫБОР ОПРЕДЕЛЕНИЯ ДИСПЛЕЯ-------------

#define printByte(args)  write(args);
//#define TEMPERATURE_PRECISION 9

//яркость дисплея
#define LCD_BR 3 //пин яркости дисплея управляемый через ПВМ
#define LCD_timer_SPD 17
#define LCD_STEP 10

//пин диода питания
#define POWER_LED_PIN 12
#define POWER_LED_BR 3

// стартовый логотип
byte logo0[8] = {0b00011, 0b00110,  0b01110,  0b11111,  0b11011,  0b11001,  0b00000,  0b00000};
byte logo1[8] = {0b10000, 0b00001,  0b00001,  0b00001,  0b00000,  0b10001,  0b11011,  0b11111};
byte logo2[8] = {0b11100, 0b11000,  0b10001,  0b11011,  0b11111,  0b11100,  0b00000,  0b00000};
byte logo3[8] = {0b00000, 0b00001,  0b00011,  0b00111,  0b01101,  0b00111,  0b00010,  0b00000};
byte logo4[8] = {0b11111, 0b11111,  0b11011,  0b10001,  0b00000,  0b00000,  0b00000,  0b00000};
byte logo5[8] = {0b00000, 0b10000,  0b11000,  0b11100,  0b11110,  0b11100,  0b01000,  0b00000};
// значок градуса!!!! lcd.write(223);
byte degree[8] = {0b11100,  0b10100,  0b11100,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};
// правый край полосы загрузки
byte right_empty[8] = {0b11111,  0b00001,  0b00001,  0b00001,  0b00001,  0b00001,  0b00001,  0b11111};
// левый край полосы загрузки
byte left_empty[8] = {0b11111,  0b10000,  0b10000,  0b10000,  0b10000,  0b10000,  0b10000,  0b11111};
// центр полосы загрузки
byte center_empty[8] = {0b11111, 0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111};
// блоки для построения графиков
byte row8[8] = {0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row7[8] = {0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row6[8] = {0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row5[8] = {0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row4[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111};
byte row3[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111};
byte row2[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111};
byte row1[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111};

char inData[82];       // массив входных значений (СИМВОЛЫ)
int PCdata[20];        // массив численных значений показаний с компьютера
byte PLOTmem[6][16];   // массив для хранения данных для построения графика (16 значений для 6 параметров)
byte blocks, halfs;
byte index = 0;
int display_mode = 6;
uint32_t Backlight_timer = 0;
String string_convert;
unsigned long timeout, uptime_timer, plot_timer;
boolean lightState, reDraw_flag = 1, updateDisplay_flag, updateTemp_flag, timeOut_flag = 1;
float duty; //, LEDcolor
int k, b, R, G, B, Rf, Gf, Bf;
byte mainTemp;
byte lines[] = {4, 5, 7, 6};
byte plotLines[] = {0, 1, 4, 5, 6, 7};    // 0-CPU temp, 1-GPU temp, 2-CPU load, 3-GPU load, 4-RAM load, 5-GPU memory
String perc;
unsigned long sec, mins, hrs;
byte temp1, temp2;
boolean btn1_sig, btn2_sig, btn1_flag, btn2_flag;

// Названия для легенды графиков
const char plot_0[] = "CPU";
const char plot_1[] = "GPU";
const char plot_2[] = "RAM";

const char plot_3[] = "temp";
const char plot_4[] = "load";
const char plot_5[] = "mem";
// названия ниже должны совпадать с массивами сверху и идти по порядку!
static const char *plotNames0[]  = {
  plot_0, plot_1, plot_0, plot_1, plot_2, plot_1
};
static const char *plotNames1[]  = {
  plot_3, plot_3, plot_4, plot_4, plot_4, plot_5
};
// 0-CPU temp, 1-GPU temp, 2-CPU load, 3-GPU load, 4-RAM load, 5-GPU memory

//переменные для регулировкия яркости дисплея

uint32_t backlight_timer;
uint16_t backlight_period = LCD_timer_SPD;
int16_t PWM;
byte loop_flag;
uint16_t serial_data;

//переменные для дебага (некоторые тут уже не просто для дебага)
float FAN_DB;
byte FAN_BUTTON;
byte DUTY_DB;
uint16_t DUTY_0;
uint16_t DUTY_1;
uint16_t DUTY_2;
uint16_t LED_DUTY;
int duty_I;
float TSD_0_F = TSD_0;
float TSD_1_F = TSD_1;
float TSD_2_F = TSD_2;
float LOW_TSD_2_F = LOW_TSD_2;
bool comeback_flag;
u8 FAN_MODE;

void setup() {
  Serial.begin(9600);
  Timer1.initialize(40); // поставить частоту ШИМ 25 кГц (40 микросекунд)
  // Пины D3 и D11 - 31.4 кГц
  TCCR2B = 0b00000001;  // x1
  TCCR2A = 0b00000001;  // phase correct
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(LCD_BR, OUTPUT); //lcd pwm
  pinMode(FAN_PIN_0, OUTPUT);
  pinMode(FAN_PIN_1, OUTPUT);
  pinMode(FAN_PIN_2, OUTPUT);
  pinMode(LED_FAN_PIN, OUTPUT);
  // инициализация дисплея
  lcd.init();
  lcd.backlight();
  lcd.clear();            // очистить дисплей
  //show_logo();            // показать логотип

  Timer1.pwm(FAN_PIN_0, 1023);
  Timer1.pwm(FAN_PIN_1, 0);
 // Timer2.pwm(FAN_PIN_2, 1023);
  analogWrite(FAN_PIN_2, TSD_2); //включаем черный вентиллятор на минималку
  analogWrite(LED_FAN_PIN, 255);

  delay(1000);               // на 2 секунды
  lcd.clear();               // очистить дисплей
  PCdata[8] = speedMAX;
  PCdata[9] = speedMIN;
  PCdata[10] = tempMAX;
  PCdata[11] = tempMIN;
  loop_flag = 1;
  while(lcd_backl(loop_flag) != 0 ){ //плавно включаем подсветку дисплея
  lcd_backl(loop_flag);  
    }
  // для дебага
  pinMode(A1, INPUT);
  //ШИМ на диод питания
  SoftPWMBegin();
  SoftPWMSet(POWER_LED_PIN, POWER_LED_BR);  
}
// 8-maxFAN, 9-minFAN, 10-maxTEMP, 11-minTEMP, 12-mnlFAN


// ------------------------------ ОСНОВНОЙ ЦИКЛ -------------------------------
void loop() {
  buttonsTick();                        
  dutyCalculate();                    // посчитать скважность для вентиляторов              
  updateDisplay();                    // обновить показания на дисплее
  timeoutTick();  //не хочу отключать дисплей пока дебажу
  parsing();    

   if (loop_flag != 0 )  lcd_backl(loop_flag);
   
      if (duty >= TSD_0) DUTY_0 = (duty/100) * 1023;
      else DUTY_0 = (TSD_0_F * 1023)/100;
      
      if (duty >= TSD_1) DUTY_1 = (duty/100) * 1023;
      else DUTY_1 = 0; //(TSD_1_F/100) * 1023;
      
      if (duty >= TSD_2 ) DUTY_2 = (duty/100) * 255;
      else{ 
      if (duty <= LOW_TSD_2)   DUTY_2 = (TSD_0_F/100) * 255;
      else DUTY_2 = (LOW_TSD_2_F * 1023)/100; //в 5.0 не исправленно
      
      }
      LED_DUTY = duty;
      if (millis() - Backlight_timer >= 10000) {
      Timer1.pwm(FAN_PIN_0, DUTY_0); // 0-1023 duty_cycle/ 100)* 1023 //норм способ регулировки скважности Timer1.pwm(FAN_PIN_0, duty * 10);(было)
      Timer1.pwm(FAN_PIN_1, DUTY_1); // 0-1023 FAN_3 черный
      analogWrite(FAN_PIN_2, DUTY_2); //74 0-255
      analogWrite(LED_FAN_PIN, (duty/100)*255);
      
      Backlight_timer = millis();
      }
}
// ------------------------------ ОСНОВНОЙ ЦИКЛ -------------------------------




void buttonsTick() {
  btn1_sig = !digitalRead(BTN1);
  btn2_sig = !digitalRead(BTN2);
  if (btn1_sig && !btn1_flag) {
    display_mode++;
    reDraw_flag = 1;
    if (display_mode > 1) display_mode = 0; //display_mode > 7
    btn1_flag = 1;
  }
  if (!btn1_sig && btn1_flag) {
    btn1_flag = 0;
  }
  if (btn2_sig && !btn2_flag) {
    //теперь эта кнопка дебажит вентилляторы
    FAN_BUTTON ++;
    FAN_MODE++;
    if (FAN_MODE >= 3) FAN_MODE = 0;
//    display_mode--;
//    reDraw_flag = 1;
//    if (display_mode < 0) display_mode = 1; //display_mode = 2;
    btn2_flag = 1;
  }
  if (!btn2_sig && btn2_flag) {
    btn2_flag = 0;
  }
}

void dutyCalculate() {
  if (PCdata[12] == 1)                  // если стоит галочка ManualFAN
    duty = PCdata[14];                  // скважность равна установленной ползунком
  else {                                // если нет
    switch (PCdata[18]) {
      case 0: mainTemp = PCdata[0];                   // взять опорную температуру как CPU
        break;
      case 1: mainTemp = PCdata[1];                   // взять опорную температуру как GPU
        break;
      case 2: mainTemp = max(PCdata[0], PCdata[1]);   // взять опорную температуру как максимум CPU и GPU
        break;
      case 3: mainTemp = temp1;
        break;
      case 4: mainTemp = temp2;
        break;
    }
    duty = map(mainTemp, PCdata[11], PCdata[10], PCdata[9], PCdata[8]);
    duty = constrain(duty, PCdata[9], PCdata[8]);
  }
  if (!timeOut_flag) duty = ERROR_DUTY;               // если пропало соединение, поставить вентиляторы на ERROR_DUTY
}
void parsing() {
  while (Serial.available() > 0) { //И флаг есть то идем работать. защита от вывполнения раз в сепкунду ..заменить на иФ и выполнять раз в секунду
    char aChar = Serial.read();
    if (aChar != 'E') {
      inData[index] = aChar;
      index++;
      inData[index] = '\0';
    } else {
      char *p = inData;
      char *str;
      index = 0;
      String value = "";
      while ((str = strtok_r(p, ";", &p)) != NULL) {
        string_convert = str;
        PCdata[index] = string_convert.toInt();
        index++;
      }
      index = 0;
      updateDisplay_flag = 1;
      updateTemp_flag = 1;
    }
      if (!timeOut_flag) {                                // если связь была потеряна, но восстановилась
      if (!ERROR_BACKLIGHT) loop_flag = 2;            // включить подсветку при появлении сигнала, если разрешено
      if (ERROR_UPTIME) uptime_timer = millis();        // сбросить uptime, если разрешено
    }
    timeout = millis();
   
    timeOut_flag = 1;
    if (loop_flag != 0 )  lcd_backl(loop_flag);
    loop_flag = 1;
    if (comeback_flag){
    SoftPWMSet(POWER_LED_PIN, POWER_LED_BR); 
    display_mode = 0;
    reDraw_flag = 1;
    
    comeback_flag = 0; //состояние в котором происходит нормальная работа
    }
  }
}

void updateDisplay() {
  if (updateDisplay_flag) {
    if (reDraw_flag) {
      lcd.clear();
      switch (display_mode) {
        case 0: draw_labels_1();
          break;
        case 1:  draw_labels_2();
          break;
      }
      reDraw_flag = 0;
    }
    switch (display_mode) {
      case 0: draw_stats_1();
        break;
      case 1: draw_stats_2();
        break;
      case 50: debug();
        break;
    }
    updateDisplay_flag = 0;
  }
}
void draw_stats_1() {
  lcd.setCursor(4, 0); lcd.print(PCdata[0]); lcd.write(223);
  lcd.setCursor(17, 0); lcd.print(PCdata[4]);
  if (PCdata[4] < 10) perc = "% ";
  else if (PCdata[4] < 100) perc = "%";
  else perc = "";  lcd.print(perc);
  lcd.setCursor(4, 1); lcd.print(PCdata[1]); lcd.write(223);
  lcd.setCursor(17, 1); lcd.print(PCdata[5]);
  if (PCdata[5] < 10) perc = "% ";
  else if (PCdata[5] < 100) perc = "%";
  else perc = "";  lcd.print(perc);
  lcd.setCursor(17, 2); lcd.print(PCdata[7]);
  if (PCdata[7] < 10) perc = "% ";
  else if (PCdata[7] < 100) perc = "%";
  else perc = "";  lcd.print(perc);
  lcd.setCursor(17, 3); lcd.print(PCdata[6]);
  if (PCdata[6] < 10) perc = "% ";
  else if (PCdata[6] < 100) perc = "%";
  else perc = "";  lcd.print(perc);

  for (int i = 0; i < 4; i++) {
    byte line = ceil(PCdata[lines[i]] / 10);
    lcd.setCursor(7, i);
    if (line == 0) lcd.printByte(1)
      else lcd.printByte(4);
    for (int n = 1; n < 9; n++) {
      if (n < line) lcd.printByte(4);
      if (n >= line) lcd.printByte(2);
    }
    if (line == 10) lcd.printByte(4)
      else lcd.printByte(3);
  }
}
void draw_stats_2() {
  lcd.setCursor(16, 0); lcd.print((int)duty);
  if ((duty) < 10) perc = "% ";
  else if ((duty) < 100) perc = "%";
  else perc = "";  lcd.print(perc);

  lcd.setCursor(6, 1); lcd.print((int)DUTY_0); //lcd.write(223);
  lcd.setCursor(16, 1); lcd.print(LED_DUTY); //lcd.write(223);
  lcd.setCursor(6, 2); lcd.print((int)DUTY_1); //lcd.write(223);
  lcd.setCursor(16, 2); lcd.print(FAN_MODE); //lcd.write(223);
  lcd.setCursor(6, 3); lcd.print((int)DUTY_2); //lcd.write(223);

  lcd.setCursor(11, 3);
  sec = (long)(millis() - uptime_timer) / 1000;
  hrs = floor((sec / 3600));
  mins = floor(sec - (hrs * 3600)) / 60;
  sec = sec - (hrs * 3600 + mins * 60);
  if (hrs < 10) lcd.print(0);
  lcd.print(hrs);
  lcd.print(":");
  if (mins < 10) lcd.print(0);
  lcd.print(mins);
  lcd.print(":");
  if (sec < 10) lcd.print(0);
  lcd.print(sec);

  byte line = ceil(duty / 10);
  lcd.setCursor(6, 0);
  if (line == 0) lcd.printByte(1)
    else lcd.printByte(4);
  for (int n = 1; n < 9; n++) {
    if (n < line) lcd.printByte(4);
    if (n >= line) lcd.printByte(2);
  }
  if (line == 10) lcd.printByte(4)
    else lcd.printByte(3);
}
void draw_labels_1() {
  lcd.backlight();
  lcd.createChar(0, degree);
  lcd.createChar(1, left_empty);
  lcd.createChar(2, center_empty);
  lcd.createChar(3, right_empty);
  lcd.createChar(4, row8);
  lcd.setCursor(0, 0);
  lcd.print("CPU:");
  lcd.setCursor(0, 1);
  lcd.print("GPU:");
  lcd.setCursor(0, 2);
  lcd.print("GPUmem:");
  lcd.setCursor(0, 3);
  lcd.print("RAMuse:");
}
void draw_labels_2() {
  lcd.createChar(0, degree);
  lcd.createChar(1, left_empty);
  lcd.createChar(2, center_empty);
  lcd.createChar(3, right_empty);
  lcd.createChar(4, row8);

  lcd.setCursor(0, 0);
  lcd.print("FANsp:");
  lcd.setCursor(1, 1);
  lcd.print("FAN1: ");
  lcd.setCursor(11, 1);
  lcd.print("LED:");
  lcd.setCursor(1, 2);
  lcd.print("FAN2:");
  lcd.setCursor(11, 2);
  lcd.print("MOD:");
  lcd.setCursor(1, 3);
  lcd.print("FAN3:");
}
void draw_legend() {
  byte data = PCdata[plotLines[display_mode]];
  lcd.setCursor(16, 2); lcd.print(data);
  if (display_mode > 1) {
    if (data < 10) perc = "% ";
    else if (data < 100) perc = "%";
    else {
      perc = "";
    }
    lcd.print(perc);
  } else {
    if (data < 10) {
      lcd.write(223);
      lcd.print("  ");
    } else if (data < 100) {
      lcd.write(223); lcd.print(" ");
    } else {
      lcd.write(223);
    }
  }
}

void draw_plot() {
  draw_legend();

  for (byte i = 0; i < 16; i++) {                       // каждый столбец параметров
    blocks = floor(PLOTmem[display_mode][i] / 8);       // найти количество целых блоков
    halfs = PLOTmem[display_mode][i] - blocks * 8;      // найти число оставшихся полосок
    for (byte n = 0; n < 4; n++) {
      if (n < blocks) {
        lcd.setCursor(i, (3 - n));
        lcd.printByte(0);
      }
      if (n >= blocks) {
        if (n != 3) {
          lcd.setCursor(i, (3 - n));
          if (halfs > 0) lcd.printByte(halfs);
          for (byte k = n + 1; k < 4; k++) {
            lcd.setCursor(i, (3 - k));
            lcd.print(" ");
          }
          break;
        }
      }
    }
  }
}

void draw_plot_symb() {
  lcd.createChar(0, row8);
  lcd.createChar(1, row1);
  lcd.createChar(2, row2);
  lcd.createChar(3, row3);
  lcd.createChar(4, row4);
  lcd.createChar(5, row5);
  lcd.createChar(6, row6);
  lcd.createChar(7, row7);
  lcd.setCursor(16, 0);
  lcd.print(plotNames0[display_mode]);
  lcd.setCursor(16, 1);
  lcd.print(plotNames1[display_mode]);
}
void timeoutTick() {  
   while (Serial.available() < 1){
    if ((millis() - timeout > 5000) && timeOut_flag) {        
      SoftPWMSet(POWER_LED_PIN, 0); 
      //getTemperature();    
      index = 0;
      updateTemp_flag = 1;
      //getTemperature(); 
      loop_flag = 2;
      if (PWM == 255) lcd.clear();
      lcd.setCursor(5, 1);
      lcd.print("CONNECTION");
      lcd.setCursor(7, 2);
      lcd.print("FAILED");
      //delay(5000);
      if (loop_flag != 0 )  lcd_backl(loop_flag);            
       //выключаем дисплей
     //условие выхода обратно по таймеру
      timeOut_flag = 1;    
      reDraw_flag = 0;
      updateDisplay_flag = 1;
      comeback_flag = 1; // состояние в котором происходит ничего
    }
  }
}

void debug() {
  lcd.clear();
  lcd.setCursor(0, 0);
  for (int j = 0; j < 5; j++) {
    lcd.print(PCdata[j]); lcd.print("  ");
  }
  lcd.setCursor(0, 1);
  for (int j = 6; j < 10; j++) {
    lcd.print(PCdata[j]); lcd.print("  ");
  }
  lcd.setCursor(0, 2);
  for (int j = 10; j < 15; j++) {
    lcd.print(PCdata[j]); lcd.print("  ");
  }
  lcd.setCursor(0, 3);
  for (int j = 15; j < 18; j++) {
    lcd.print(PCdata[j]); lcd.print("  ");
  }
}

int lcd_backl(byte flag) {
 
switch(flag){
    case 1:
    if( (millis() - backlight_timer >= backlight_period) && (PWM < 255)){
        backlight_timer = millis();
        PWM += LCD_STEP;
        if (PWM > 255) PWM = 255;
        analogWrite(LCD_BR, PWM); 
    }
    if (PWM >= 255) flag = 0;    
    return flag;
    break;
    case 2:
    if( (millis() - backlight_timer >= backlight_period) && (PWM > 0)){ 
        backlight_timer = millis();
        PWM -= LCD_STEP;
        if (PWM < 0 ) PWM = 0;
        analogWrite(LCD_BR, PWM);    
       }
     if (PWM <= 0) flag = 0;   
     return flag;
     break;
     case 3:
       if( (millis() - backlight_timer >= backlight_period) && (PWM != 1)){ 
        backlight_timer = millis();
        if (PWM > 1) PWM = 1;
        else PWM = 1;
        analogWrite(LCD_BR, PWM);
       }
     if (PWM == 1) flag = 0;
     return flag;
     break;
     default:
     flag = 0;
     return flag;
     }
}
