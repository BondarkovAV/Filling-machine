#include <LiquidCrystal.h>
#include <HX711_ADC.h>
#include <Keypad.h> // Клавиатура 4 на 4
#include <EEPROM.h> // Запись в память
// Задаём имя пинов дисплея
constexpr uint8_t PIN_RS = 14;
constexpr uint8_t PIN_EN = 15;
constexpr uint8_t PIN_DB4 = 16;
constexpr uint8_t PIN_DB5 = 17;
constexpr uint8_t PIN_DB6 = 18;
constexpr uint8_t PIN_DB7 = 19;
LiquidCrystal lcd(PIN_RS, PIN_EN, PIN_DB4, PIN_DB5, PIN_DB6, PIN_DB7); //объект для дисплея
// Переменные для клавиатуры
const byte ROWS = 4; // 4 строки
const byte COLS = 4; // 4 столбца
char keys[ROWS][COLS] = {
  {'1','2','3','0'},
  {'4','5','6','0'},
  {'7','8','9','C'},
  {'.','0','0','0'}
};
byte rowPins[ROWS] = {A1, A2, A3, A4};
byte colPins[COLS] = {A5, A6, A7, A8}; 
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
// Объявляем переменные
int first_speed_left_fc = 2;       // первая скорость левого частотника
int second_speed_left_fc = 3;       // вторая скорость левого частотника
int first_speed_right_fc = 4;       // первая скорость правого частотника
int second_speed_right_fc = 5;       // вторая скорость правого частотника
int injector_left = 6;     // левый клапан инжектора
int injector_right = 7;     // правый клапан инжектора
HX711_ADC TenzoLeft(8, 9);   // parameters: dt pin, sck pin
HX711_ADC TenzoRight(10, 11);   // parameters: dt pin, sck pin
int major_pneumo = 12; // Общий пневмо цилиндр и старт частотников
int pedal = 13; // Педаль старта\стопа цикла
int tare = 20; // Кнопка тары и настроек
bool flag_pedal = false;
bool flag_tare = false;
unsigned long current_time;
unsigned long delta_time;
int potenc_weight = A0; // Потенциометр установки веса
float current_weight_left = 0;
float current_weight_right = 0;
float max_weight_right = 0;
float max_weight_left = 0;
String text_weight_right;
String text_weight_left;
// Состояния
bool settings = false;
bool settings_left = false;
bool tareeng = false;
bool filling = false;
bool start = false;
bool stop = false;
bool first_cycle_left = false;
bool first_cycle_right = false;
bool second_cycle_left = false;
bool second_cycle_right = false;
bool end_cycle_left = false;
bool end_cycle_right = false;
bool end = false;


void setup() {
  Serial.begin(9600);

  // Забираем максимальные вес из памяти
  //max_weight = (float)((analogRead(potenc_weight) / 150.0) + 3.5);
  EEPROM.put(5, 10.5); // Inizialization weight left
  EEPROM.get(0, max_weight_right);
  EEPROM.get(5, max_weight_left);
  text_weight_right = max_weight_right;
  text_weight_left = max_weight_left;

  pinMode (tare, INPUT_PULLUP);
  pinMode (pedal, INPUT_PULLUP);

  pinMode(first_speed_left_fc, OUTPUT);
  digitalWrite(first_speed_left_fc, HIGH);
  pinMode(second_speed_left_fc, OUTPUT);
  digitalWrite(second_speed_left_fc, HIGH);
  pinMode(first_speed_right_fc, OUTPUT);
  digitalWrite(first_speed_right_fc, HIGH);
  pinMode(second_speed_right_fc, OUTPUT);
  digitalWrite(second_speed_right_fc, HIGH);
  pinMode(injector_left, OUTPUT);
  digitalWrite(injector_left, HIGH);
  pinMode(injector_right, OUTPUT);
  digitalWrite(injector_right, HIGH);
  pinMode(major_pneumo, OUTPUT);
  digitalWrite(major_pneumo, HIGH);

  lcd.begin(16, 2);            
  lcd.setCursor(0, 0);                // Устанавливаем курсор в колонку 0 и строку 0
  lcd.print("Filling  Machine");         // Печатаем первую строку
  lcd.setCursor(0, 1);
  lcd.print("Initialization..");

  TenzoLeft.begin();                     // start connection to HX711
  TenzoLeft.start(2000);                 // load cells gets 2000ms of time to stabilize
  TenzoLeft.setCalFactor(103563.64);        // calibration factor for load cell => strongly dependent on your individual setup
  TenzoRight.begin();
  TenzoRight.start(2000);                 // load cells gets 2000ms of time to stabilize
  TenzoRight.setCalFactor(102326.58);


  lcd.clear();
  lcd.begin(16, 2);
}

void loop() {
  
  lcd.setCursor(0, 0);                // Устанавливаем курсор в колонку 0 и строку 0
  lcd.print("  WEIGHT (Kg):  ");          // Печатаем первую строку
  TenzoLeft.update();                              // достаем данные из тензодатчика
  TenzoRight.update();
  current_weight_left = TenzoLeft.getData();
  current_weight_right = TenzoRight.getData();                           // присваиваем данные переменной
  current_weight_left = current_weight_left * (-1);
  current_weight_right = current_weight_right * (-1); //invert weight

  Serial.print(current_weight_left);
  Serial.println(" KG");
  Serial.print(current_weight_right);
  Serial.println(" KG");

 if (current_weight_left < 0)
 {
  current_weight_left = current_weight_left * (-1);
  lcd.setCursor(0, 1);                // Устанавливаем курсор в колонку 0 и строку 1
  lcd.print("-");                     // Печатаем минус
 }
 else
 {
  lcd.setCursor(0, 1);                // Устанавливаем курсор в колонку 0 и строку 1
  lcd.print(" ");                     // Стираем минус
 }
  lcd.setCursor(1, 1); // set cursor to secon row
  lcd.print(current_weight_left, 2); // print out the retrieved value to the second row

  if (current_weight_right < 0)
 {
  current_weight_right = current_weight_right * (-1);
  lcd.setCursor(7, 1);                // Устанавливаем курсор в колонку 0 и строку 1
  lcd.print("-");                     // Печатаем минус
 }
 else
 {
  lcd.setCursor(7, 1);                // Устанавливаем курсор в колонку 0 и строку 1
  lcd.print(" ");                     // Стираем минус
 }
  lcd.setCursor(8, 1); // set cursor to secon row
  lcd.print(current_weight_right, 2); // print out the retrieved value to the second row
  

  //***************************Error tenzo weight****************************
  if (current_weight_left > 15){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("  HIGH WEIGHT:  ");
    lcd.setCursor(0, 1);
    lcd.print("      LEFT      ");
    delay(1000);
    lcd.clear();
    lcd.begin(16, 2);
  }

  if (current_weight_right > 15){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("  HIGH WEIGHT:  ");
    lcd.setCursor(0, 1);
    lcd.print("     RIGHT      ");
    delay(1000);
    lcd.clear();
    lcd.begin(16, 2);
  }

  bool but_tare = !digitalRead(tare);
  bool but_pedal = !digitalRead(pedal);

  if (but_tare && !flag_tare){
    flag_tare = true;
    current_time = millis();
  }

  if (!but_tare && flag_tare){
    flag_tare = false;
    delta_time = millis() - current_time;
    if (delta_time < 1000){
      tareeng = true;
    }
    else {
      settings = true;
    }
  }

  if (settings){
    lcd.clear();
    lcd.begin(16, 2);
    EEPROM.get(0, max_weight_right);
    while (true) {
      lcd.setCursor(0, 0);
      lcd.print("RIGHT MAX KG:   ");
      //Serial.println(analogRead(potenc_weight));
      //max_weight = (float)((analogRead(potenc_weight) / 150.0) + 3.5);
      //String text_weight(max_weight, 3);
      char key = keypad.getKey();
      if (key){
        Serial.println(key);
        if (key != 'C'){
          String key_str = String(key);
          if (text_weight_right == "0"){
            text_weight_right = key_str;
          }
          else{
            text_weight_right = text_weight_right + key_str;
          }
          max_weight_right = text_weight_right.toFloat();
          Serial.println(max_weight_right);
        }
        else if (key == 'C') {
          text_weight_right = "0";
          Serial.println(text_weight_right);
          max_weight_right = text_weight_right.toFloat();
          lcd.clear();
          lcd.begin(16, 2);
        }
      }
      lcd.setCursor(0, 1);
      lcd.print(text_weight_right + " ");
      bool but_tare = !digitalRead(tare);
      //delay(100);
      
      if (but_tare){
        settings = false;
        settings_left = true;
        EEPROM.put(0, max_weight_right);
        Serial.println(max_weight_right);
        lcd.clear();
        lcd.begin(16, 2);
        delay(500);
        break;
      }
    }
  }

  if (settings_left){
    lcd.clear();
    lcd.begin(16, 2);
    EEPROM.get(0, max_weight_left);
    while (true) {
      lcd.setCursor(0, 0);
      lcd.print("LEFT MAX KG:    ");
      //Serial.println(analogRead(potenc_weight));
      //max_weight = (float)((analogRead(potenc_weight) / 150.0) + 3.5);
      //String text_weight(max_weight, 3);
      char key = keypad.getKey();
      if (key){
        Serial.println(key);
        if (key != 'C'){
          String key_str = String(key);
          if (text_weight_left == "0"){
            text_weight_left = key_str;
          }
          else{
            text_weight_left = text_weight_left + key_str;
          }
          max_weight_left = text_weight_left.toFloat();
          Serial.println(max_weight_left);
        }
        else if (key == 'C') {
          text_weight_left = "0";
          Serial.println(text_weight_left);
          max_weight_left = text_weight_left.toFloat();
          lcd.clear();
          lcd.begin(16, 2);
        }
      }
      lcd.setCursor(0, 1);
      lcd.print(text_weight_left + " ");
      bool but_tare = !digitalRead(tare);
      //delay(100);
      
      if (but_tare){
        settings_left = false;
        EEPROM.put(5, max_weight_left);
        Serial.println(max_weight_left);
        lcd.clear();
        lcd.begin(16, 2);
        delay(500);
        break;
      }
    }
  }

  if (tareeng){
    lcd.clear();
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print("   TAREENG...   ");
    lcd.setCursor(0, 1); 
    lcd.print("   TENZO LEFT   ");
    TenzoLeft.start(2000);
    lcd.setCursor(0, 1); 
    lcd.print("  TENZO RIGHT   ");
    TenzoRight.start(2000);
    delay (1000);
    lcd.clear();
    lcd.begin(16, 2);
    tareeng = false;
  }

  if (but_pedal && !flag_pedal){
    flag_pedal = true;
  }

  if (!but_pedal && flag_pedal){
    flag_pedal = false;
    filling = true;
  }
//--------------------FILLING CYCLE--------------------
  if (filling) {
    lcd.clear();
    lcd.begin(16, 2);
    start = true;
    delay(200);
    //----------------Cycle start-------------------
    while(true){
      bool but_tare = !digitalRead(tare);
      bool but_pedal = !digitalRead(pedal);
      if (but_pedal && !stop){
        stop = true;
        current_time = millis();
      }
      if (!but_pedal && stop){
        stop = false;
        delta_time = millis() - current_time;
        if (delta_time > 500){
          lcd.clear();
          lcd.begin(16, 2);
          lcd.setCursor(0, 0);
          lcd.print("  Stop filling  ");
          goto exit;
        }
      }

      if (start){
        //Major pneumo lowering
        digitalWrite(major_pneumo, LOW);
        lcd.setCursor(0, 0);
        lcd.print("lowering inject!");
        for (int h = 1; h < 6000; h++) {
          TenzoLeft.update();
          TenzoRight.update();
          current_weight_left = TenzoLeft.getData();
          current_weight_right = TenzoRight.getData();
          current_weight_left = current_weight_left * (-1);
          current_weight_right = current_weight_right * (-1);
          if (current_weight_left > 3 || current_weight_right > 3){
            digitalWrite(major_pneumo, HIGH);
            lcd.setCursor(0, 0);
            lcd.print("Canister error! ");
            delay(500);
            start = false;
            goto exit;
          }
        }
        //Open injectors
        digitalWrite(injector_left, LOW);
        digitalWrite(injector_right, LOW);
        lcd.setCursor(0, 0);
        lcd.print(" Open injectors ");
        lcd.setCursor(0, 1);
        lcd.print("    FC START    ");
        delay(500);
        TenzoLeft.update();
        TenzoRight.update();
        current_weight_left = TenzoLeft.getData();
        current_weight_right = TenzoRight.getData();
        current_weight_left = current_weight_left * (-1);
        current_weight_right = current_weight_right * (-1);
        lcd.clear();
        lcd.begin(16, 2);
        lcd.setCursor(0, 0);
        lcd.print("Filling canister");
        start = false;
        first_cycle_left = true;
        first_cycle_right = true;
      }

      if (first_cycle_left){
        lcd.setCursor(0, 0);
        lcd.print("Filling canister");
        if (current_weight_left <= (max_weight_left - 1)){
          TenzoLeft.update();
          current_weight_left = TenzoLeft.getData();
          current_weight_left = current_weight_left * (-1);
          lcd.setCursor(0, 1);
          lcd.print(current_weight_left, 2);
          digitalWrite(first_speed_left_fc, LOW);
        }
        else{
          first_cycle_left = false;
          digitalWrite(first_speed_left_fc, HIGH);
          second_cycle_left = true;
          lcd.begin(16, 2);
        }
      }

      if (first_cycle_right){
        lcd.setCursor(0, 0);
        lcd.print("Filling canister");
        if (current_weight_right <= (max_weight_right - 1)){
          TenzoRight.update();
          current_weight_right = TenzoRight.getData();
          current_weight_right = current_weight_right * (-1);
          lcd.setCursor(7, 1);
          lcd.print(current_weight_right, 2);
          digitalWrite(first_speed_right_fc, LOW);
        }
        else{
          first_cycle_right = false;
          digitalWrite(first_speed_right_fc, HIGH);
          second_cycle_right = true;
          lcd.begin(16, 2);
        }
      }

      if(second_cycle_left){
        lcd.setCursor(0, 0);
        lcd.print("Filling canister");
        if (current_weight_left <= max_weight_left){
          TenzoLeft.update();
          current_weight_left = TenzoLeft.getData();
          current_weight_left = current_weight_left * (-1);
          lcd.setCursor(0, 1);
          lcd.print(current_weight_left, 2);
          digitalWrite(second_speed_left_fc, LOW);
        }
        else{
          digitalWrite(second_speed_left_fc, HIGH);
          digitalWrite(injector_left, HIGH);
          second_cycle_left = false;
          end_cycle_left = true;
          lcd.begin(16, 2);
          lcd.setCursor(0, 1);
          lcd.print("Done!    ");
        }
      }

      if(second_cycle_right){
        lcd.setCursor(0, 0);
        lcd.print("Filling canister");
        if (current_weight_right <= max_weight_right){
          TenzoRight.update();
          current_weight_right = TenzoRight.getData();
          current_weight_right = current_weight_right * (-1);
          lcd.setCursor(7, 1);
          lcd.print(current_weight_right, 2);
          digitalWrite(second_speed_right_fc, LOW);
        }
        else{
          digitalWrite(second_speed_right_fc, HIGH);
          digitalWrite(injector_right, HIGH);
          second_cycle_right = false;
          end_cycle_right = true;
          lcd.begin(16, 2);
          lcd.setCursor(7, 1);
          lcd.print("Done!    ");
        }
      }

      if(end_cycle_left && end_cycle_right){
        end_cycle_left = false;
        end_cycle_right = false;
        lcd.clear();
        lcd.begin(16, 2);
        lcd.setCursor(0, 0);
        lcd.print("Done filling!");
        exit:
        digitalWrite(first_speed_left_fc, HIGH);
        digitalWrite(second_speed_left_fc, HIGH);
        digitalWrite(first_speed_right_fc, HIGH);
        digitalWrite(second_speed_right_fc, HIGH);
        delay(300);
        digitalWrite(injector_left, HIGH);
        digitalWrite(injector_right, HIGH);
        delay(300);
        digitalWrite(major_pneumo, HIGH);
        filling = false;
        settings = false;
        tareeng = false;
        start = false;
        first_cycle_left = false;
        first_cycle_right = false;
        second_cycle_left = false;
        second_cycle_right = false;
        end_cycle_left = false;
        end_cycle_right = false;
        end = false;
        lcd.begin(16, 2);
        break;
      }
    }
  lcd.begin(16, 2);
  }
}
