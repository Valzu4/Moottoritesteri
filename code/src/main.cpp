/* TOUCHSCREEN
Max x = 850
Max y = 900

Min x = 200
Min y = 120

x kasvaa alhaalta ylös, Y vasemmalta oikealle
*/

#include <Arduino.h>

#include "SPI.h"
#include "Adafruit_GFX.h"
#include <Adafruit_HX8357.h>
#include "TouchScreen.h"

//#define testing 1

// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10
#define TFT_RST 10

#define TS_MINX 850
#define TS_MAXX 200
#define TS_MINY 120
#define TS_MAXY 900

#define YP A8  // must be an analog pin, use "An" notation!
#define XM A11  // must be an analog pin, use "An" notation!
#define YM A9   // can be a digital pin
#define XP A10


#define motorcountdef 16

#define resistanceMin 0.20
#define resistanceMax 1
#define rotationTimeMin 1500
#define rotationTimeMax 4000
#define currentMax 600
#define shortdef 4
#define resistanceTreshold 5
#define touchdelay 100
#define minVin 0.61

#define R1 19600
#define R2 60400
#define Rfet 0.45

Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

int motorcount = 15;
int rotatecount = 1;
int menu = 0;
int motorselect = 0;
int psuError = 0;
int testMode = 0;

int mux(int nro);
void draw (int menu);
float psuVolt();
int min1(int rotationTimes[], int n);
int max1(int rotationTimes[], int n);
void test4(int count, int rotations);
void test8(int count, int rotations);
void test12(int count, int rotations);
void test16(int count, int rotations);
void returnHome(int num);

class Motor {
  public:
    int nro;
    float resistance;
    int current;
    int motorstate = 0;
    int lastRotationTimes[2];
    

    // {resistanssi, }
    int errors[6];

    void add(int iNro);
    void On();
    void Off();
    float resRead();
    int currentRead();
    void test(int rotations);
    void returnHome(int motor);
    void home();

    Motor(){
      for (int i = 0; i < 5 ; i++)
      {
        errors[i] = 0;
      }
      errors[5] = 1;
      lastRotationTimes[0] = 0;
      lastRotationTimes[1] = 0;
    };

  private:

};

void Motor::add(int iNro){
  nro = iNro;
}

void Motor::On(){
  int a = nro + 22;
  digitalWrite(a, 1);
  motorstate = 1;
}

void Motor::Off(){
  int a = nro + 22;
  digitalWrite(a, 0);
  motorstate = 0;
}

float Motor::resRead(){
  mux(nro);
  int aPin = 54;
  
  if (nro >=0 && nro < 4){
    aPin = 54;
  } else if (nro >=4 && nro < 8){
    aPin = 55;
  } else if (nro >=8 && nro < 12) {
    aPin = 56;
  } else if (nro >= 12 && nro < 16){
    aPin = 57;
  }

  analogReadResolution(12);
  float volt = 3.3/4096*analogRead(aPin);
  analogReadResolution(10);

  //volt div: 60.4k = R1 19.6K = R2, 100 = Draining resistor
  float res = 0.245 * psuVolt() / (psuVolt() - volt);

  resistance = res;
  return res;
}

int Motor::currentRead(){
  mux(nro);
  digitalWrite(5, 0);
  delay(100);

  int aPin = 54;
  
  if (nro >=0 && nro < 4){
    aPin = 54;
  } else if (nro >=4 && nro < 8){
    aPin = 55;
  } else if (nro >=8 && nro < 12) {
    aPin = 56;
  } else if (nro >= 12 && nro < 16){
    aPin = 57;
  }

  analogReadResolution(12);
  //float volt = 3.3 / 4096 * analogRead(aPin);
  analogReadResolution(10);
  float cur = analogRead(aPin);
  // KORJAA KAAVA!!!!!
  //float cur = ((12-volt) * (R1 + R2))/(Rfet * R2);

  /*
  if(analogRead(aPin) < 300){
    cur = 0;
  }
  */
  current = cur;

  if (cur > currentMax){
    return 0;
  } else {
    return 1;
  }
  
}

void Motor::test(int rotations){
  errors[5] = 1;
  int digitalPin;
  if (nro >= 0 && nro <= 3){
    digitalPin = 50;
  } else if (nro >= 4 && nro <= 7){
    digitalPin = 51;
  } else if (nro >= 8 && nro <= 11){
    digitalPin = 52;
  } else if (nro >= 12 && nro <= 15){
    digitalPin = 53;
  } else {
    digitalPin = 0;
  }

  int rotated = 0;
  int rotationTime[rotations];

  for (int j = 0; j < rotations; j++){
    rotationTime[j] = 0;
  }
  
  
  if(errors[0]==0 && errors[1] == 0){
    delay(500);
    On();
    int timer1 = millis();
    while(rotated < rotations){
      if (currentRead() !=  0){
        errors[4]++;
        break;
      }

      if(digitalRead(digitalPin) == 1 && millis() - timer1 > 1000){
        rotationTime[rotated] = millis() - timer1;
        rotated++;
        Off();
        delay(500);
        On();
        timer1 = millis();
      }
      TSPoint p = ts.getPoint();
      p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
      p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
      if (p.x > 170 && p.x < 310 && p.y > 400 && p.y < 470){
        Off();
        ::returnHome(nro);
        menu = 2;
        draw(menu);
        break;
      }
    }

    Off();
    errors[5] = 0;
    for (int h = 0; h < rotations; h++)
    {
      if (rotationTime[h] > rotationTimeMax){
        errors[2]++;
      } else if (rotationTime[h] < rotationTimeMin){
        errors[3]++;
      }
      lastRotationTimes[0] = min1(rotationTime, rotations);
      lastRotationTimes[1] = max1(rotationTime, rotations);
    }
  }
  delay(500);
  ::returnHome(nro);
}

Motor motor[motorcountdef];

float psuVolt(){
  float volt = 3.3 / 4096 * analogRead(A5) * 4.08163;
  return(volt);
}

void resTest(){
  
  // Turn all motors off
  for (int i = 0; i < 16; i++)
  {
    motor[i].Off();
  }

  for(int k = 0; k < 6; k++){
    motor[k].errors[0] = 0;
    motor[k].errors[1] = 0;
  }

  // Motor resistance drive on
  digitalWrite(5, 1);
  delay(500);
  
  for (int i = 0; i < 16; i++)
  {
    motor[i].resRead();
    if (motor[i].resistance < resistanceMin){
      motor[i].errors[0]++;
    } else if (motor[i].resistance > resistanceMax){
      motor[i].errors[1]++;
    } else {}
  }

  if(psuVolt() < minVin){
    psuError++;
  }
  // Motor resistance drive off
  delay(10);
  digitalWrite(5, 0);
  
}

void motorTest(int count, int rotations){
  resTest();
  if (count < 4){
    test4(count, rotations);
  } else if (count >= 4 && count < 8){
    test8(count, rotations);
  } else if (count >= 8 && count < 12){
    test12(count, rotations);
  } else if (count >= 12 && count < 16){
    test16(count, rotations);
  }
  
  menu = 4;
  draw(menu);
  
}

void test4(int count, int rotations){
  for (int g = 0; g <= count; g++) {
    motor[g].errors[5] = 1;
  }
  
  int stopTimer1 = 0;

  for(int i = 0; i <= count; i++){
    delay(500);

    int rotated = 0;
    int rotationTime[rotations];

    for (int j = 0; j < rotations; j++)
    {
      rotationTime[j] = 0;
    }
    
    if(motor[i].errors[0]==0 && motor[i].errors[1] == 0){
      motor[i].On();
      int timer1 = millis();

      while(rotated < rotations){
        if (motor[i].currentRead() != 0){
          motor[i].errors[4]++;
          break;
        }

        if(digitalRead(50) == 1 && millis() - timer1 > 1000){
          rotationTime[rotated] = millis() - timer1;
          rotated++;
          motor[i].Off();
          stopTimer1 = millis();
        }
        

        if(motor[i].motorstate == 0 && millis() - stopTimer1 > 500){
          motor[i].On();
          timer1 = millis();
        }

        TSPoint p = ts.getPoint();
        p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
        p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
        if (p.x > 170 && p.x < 310 && p.y > 400 && p.y < 470){
          motor[i].Off();

          draw(6);
          for (int i = 0; i < 4; i++){
            returnHome(i);
          }

          menu = 2;
          draw(menu);
          break;
        }
      }

      motor[i].Off();
      motor[i].errors[5] = 0;

      for (int h = 0; h < rotations; h++)
      {
        if (rotationTime[h] > rotationTimeMax){
          motor[i].errors[2]++;
        } else if (rotationTime[h] < rotationTimeMin){
          motor[i].errors[3]++;
        }

        motor[i].lastRotationTimes[0] = min1(rotationTime, rotations);

        motor[i].lastRotationTimes[1] = max1(rotationTime, rotations);

      }
    }
    
    if (menu == 2){
        break;
      }

  }
  delay(500);
  for (int i = 0; i < 4; i++){
    returnHome(i);
  }
}

void test8(int count, int rotations){
  for (int g = 0; g <= count; g++) {
    motor[g].errors[5] = 1;
  }

  int rotated1 = 0;
  int rotated2 = 0;
  int rotationTime1[rotations];
  int rotationTime2[rotations];
  int timer1 = 0;
  int timer2 = 0;
  int stopTimer1 = 0;
  int stopTimer2 = 0;

  for(int i = 0; i <= 3; i++){
    delay(500);
    rotated1 = 0;
    rotated2 = 0;

    for (int j = 0; j < rotations; j++)
    {
      rotationTime1[j] = 0;
      rotationTime2[j] = 0;
    }
    
    if(motor[i].errors[0] == 0 && motor[i].errors[1] == 0){
      motor[i].On();
      timer1 = millis();
    } else{
      rotated1 = rotations;
    }

    if(motor[i+4].errors[0] == 0 && motor[i+4].errors[1] == 0 && i + 4 <= count){
      motor[i+4].On();
      timer2 = millis();
    } else {
      rotated2 = rotations;
    }
      

    while(rotated1 < rotations+1 || rotated2 < rotations+1){

      if(motor[i].motorstate == 1 && digitalRead(50) == 1 && millis() - timer1 > 1000){
        rotationTime1[rotated1] = millis() - timer1;
        rotated1++;
        motor[i].Off();
        stopTimer1 = millis();
      }

      if(motor[i+4].motorstate == 1 && digitalRead(51) == 1 && millis() - timer2 > 1000){
        rotationTime2[rotated2] = millis() - timer2;
        rotated2++;
        motor[i].Off();
        stopTimer1 = millis();
      }

      if (rotated1 >= rotations){
        motor[i].Off();
        motor[i].errors[5] = 0;
        rotated1 = rotations+1;
      }

      if (rotated2 >= rotations){
        motor[i+4].Off();
        motor[i+4].errors[5] = 0;
        rotated2 = rotations+1;
      }

      if(motor[i].motorstate == 1 && motor[i].currentRead() != 0){
        motor[i].errors[4]++;
        break;
      }

      if(motor[i+4].motorstate == 1 && motor[i+4].currentRead() != 0){
        motor[i+4].errors[4]++;
        break;
      }
      
      if(motor[i].motorstate == 0 && motor[i+4].motorstate == 0){
        if (stopTimer1 > 500 && stopTimer2 > 500){
          if (rotated1 < rotations){
            motor[i].On();
            timer1 = millis();
          }
          
          if (rotated2 < rotations){
            motor[i+4].On();
            timer2 = millis();
          }
        }
        
      }

      TSPoint p = ts.getPoint();

      if(motor[i].motorstate == 1 && digitalRead(50) == 1 && millis() - timer1 > 1000){
        rotationTime1[rotated1] = millis() - timer1;
        rotated1++;
        motor[i].Off();
        stopTimer1 = millis();
      }

      if(motor[i+4].motorstate == 1 && digitalRead(51) == 1 && millis() - timer2 > 1000){
        rotationTime2[rotated2] = millis() - timer2;
        rotated2++;
        motor[i].Off();
        stopTimer1 = millis();
      }

      p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
      p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
      if (p.x > 170 && p.x < 310 && p.y > 400 && p.y < 470){
        motor[i].Off();
        motor[i+4].Off();

        draw(6);
        for (int i = 0; i < 8; i++){
          returnHome(i);
        }

        menu = 2;
        draw(menu);
        break;
      }
    }

    motor[i].Off();
    motor[i+4].Off();

    
    for (int h = 0; h < rotations; h++){
      if (motor[i].errors[0] == 0){
        if (rotationTime1[h] > rotationTimeMax){
          motor[i].errors[2]++;
        } else if (rotationTime1[h] < rotationTimeMin){
          motor[i].errors[3]++;
        }
      }
      
      if(motor[i+4].errors[0] == 0){
        if (rotationTime2[h] > rotationTimeMax){
          motor[i+4].errors[2]++;
        } else if (rotationTime2[h] < rotationTimeMin){
          motor[i+4].errors[3]++;
        }
      }

      motor[i].lastRotationTimes[0] = min1(rotationTime1, rotations);
      motor[i+4].lastRotationTimes[0] = min1(rotationTime2, rotations);

      motor[i].lastRotationTimes[1] = max1(rotationTime1, rotations);
      motor[i+4].lastRotationTimes[1] = max1(rotationTime2, rotations);

    }
    if (menu == 2){
      break;
    }
  
  }
  delay(500);
  for (int i = 0; i < 8; i++){
    returnHome(i);
  }
}

void test12(int count, int rotations){
  for (int g = 0; g <= count; g++) {
    motor[g].errors[5] = 1;
  }
  int rotated1 = 0;
  int rotated2 = 0;
  int rotated3 = 0;
  int rotationTime1[rotations];
  int rotationTime2[rotations];
  int rotationTime3[rotations];
  int timer1 = 0;
  int timer2 = 0;
  int timer3 = 0;
  int stopTimer1 = 0;
  int stopTimer2 = 0;
  int stopTimer3 = 0;
  for(int i = 0; i <= 3; i++){
    delay(500);

    rotated1 = 0;
    rotated2 = 0;
    rotated3 = 0;
    

    for (int j = 0; j < rotations; j++)
    {
      rotationTime1[j] = 0;
      rotationTime2[j] = 0;
      rotationTime3[j] = 0;
    }
    
    if(motor[i].errors[0] == 0 && motor[i].errors[1] == 0){
      motor[i].On();
      timer1 = millis();
    } else{
      rotated1 = rotations;
    }
    if(motor[i+4].errors[0] == 0 && motor[i+4].errors[1] == 0 && i + 4 <= count){
      motor[i+4].On();
      timer2 = millis();
    } else {
      rotated2 = rotations;
    }
    if(motor[i+8].errors[0] == 0 && motor[i+8].errors[1] == 0 && i + 8 <= count){
      motor[i+8].On();
      timer3 = millis();
    } else {
      rotated3 = rotations;
    }
      

    while(rotated1 < rotations+1 || rotated2 < rotations+1 || rotated3 < rotations+1){

      if(motor[i].motorstate == 1 && digitalRead(50) == 1 && millis() - timer1 > 1000){
        rotationTime1[rotated1] = millis() - timer1;
        rotated1++;
        motor[i].Off();
        stopTimer1 = millis();
      }

      if(motor[i+4].motorstate == 1 && digitalRead(51) == 1 && millis() - timer2 > 1000){
        rotationTime2[rotated2] = millis() - timer2;
        rotated2++;
        motor[i+4].Off();
        stopTimer2 = millis();
      }

      if(motor[i+8].motorstate == 1 && digitalRead(52) == 1 && millis() - timer3 > 1000){
        rotationTime3[rotated3] = millis() - timer3;
        rotated3++;
        motor[i+8].Off();
        stopTimer3 = millis();
      }

      if(motor[i].motorstate == 1 && motor[i].currentRead() != 0){
        motor[i].errors[4]++;
        break;
      }

      if(motor[i+4].motorstate == 1 && motor[i+4].currentRead() != 0){
        motor[i+4].errors[4]++;
        break;
      }

      if(motor[i+8].motorstate == 1 && motor[i+8].currentRead() != 0){
        motor[i+8].errors[4]++;
        break;
      }

      if (rotated1 >= rotations){
        motor[i].Off();
        motor[i].errors[5] = 0;
        rotated1 = rotations+1;
      }

      if (rotated2 >= rotations){
        motor[i+4].Off();
        motor[i+4].errors[5] = 0;
        rotated2 = rotations+1;
      }

      if (rotated3 >= rotations){
        motor[i+8].Off();
        motor[i+8].errors[5] = 0;
        rotated3 = rotations+1;
      }

      if(motor[i].motorstate == 0 && motor[i+4].motorstate == 0 && motor[i+8].motorstate == 0){
        if (millis() - stopTimer1 > 500 && millis() - stopTimer2 > 500 && millis() - stopTimer3 > 500){
          if (rotated1 < rotations){
            motor[i].On();
            timer1 = millis();
          }
          
          if (rotated2 < rotations){
            motor[i+4].On();
            timer2 = millis();
          }

          if (rotated3 < rotations){
            motor[i+8].On();
            timer3 = millis();
          }
        }
        
      }

      
      TSPoint p = ts.getPoint();

      if(motor[i].motorstate == 1 && digitalRead(50) == 1 && millis() - timer1 > 1000){
        rotationTime1[rotated1] = millis() - timer1;
        rotated1++;
        motor[i].Off();
        stopTimer1 = millis();
      }

      if(motor[i+4].motorstate == 1 && digitalRead(51) == 1 && millis() - timer2 > 1000){
        rotationTime2[rotated2] = millis() - timer2;
        rotated2++;
        motor[i+4].Off();
        stopTimer2 = millis();
      }

      if(motor[i+8].motorstate == 1 && digitalRead(52) == 1 && millis() - timer3 > 1000){
        rotationTime3[rotated3] = millis() - timer3;
        rotated3++;
        motor[i+8].Off();
        stopTimer3 = millis();
      }

      p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
      p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
      if (p.x > 170 && p.x < 310 && p.y > 400 && p.y < 470){
        motor[i].Off();
        motor[i+4].Off();
        motor[i+8].Off();

        
        draw(6);
        for (int i = 0; i < 12; i++){
          returnHome(i);
        }
        
        menu = 2;
        draw(menu);
        break;
      }
    }
    
    motor[i].Off();
    motor[i+4].Off();
    motor[i+8].Off();

    for (int h = 0; h < rotations; h++){
      if (motor[i].errors[0] == 0){
        if (rotationTime1[h] > rotationTimeMax){
          motor[i].errors[2]++;
        } else if (rotationTime1[h] < rotationTimeMin){
          motor[i].errors[3]++;
        }
      }
      
      if(motor[i+4].errors[0] == 0){
        if (rotationTime2[h] > rotationTimeMax){
          motor[i+4].errors[2]++;
        } else if (rotationTime2[h] < rotationTimeMin){
          motor[i+4].errors[3]++;
        }
      }

      if(motor[i+8].errors[0] == 0){
        if (rotationTime3[h] > rotationTimeMax){
          motor[i+8].errors[2]++;
        } else if (rotationTime3[h] < rotationTimeMin){
          motor[i+8].errors[3]++;
        }
      }
    }
     
      motor[i].lastRotationTimes[0] = min1(rotationTime1, rotations);
      motor[i+4].lastRotationTimes[0] = min1(rotationTime2, rotations);
      motor[i+8].lastRotationTimes[0] = min1(rotationTime3, rotations);

      motor[i].lastRotationTimes[1] = max1(rotationTime1, rotations);
      motor[i+4].lastRotationTimes[1] = max1(rotationTime2, rotations);
      motor[i+8].lastRotationTimes[1] = max1(rotationTime3, rotations);
    
    if (menu == 2){
      break;
    }
  
  }
  delay(500);
  for (int i = 0; i < 12; i++){
    returnHome(i);
  }
  
}

void test16(int count, int rotations){
  for (int g = 0; g <= count; g++) {
    motor[g].errors[5] = 1;
  }
  
  int rotated1 = 0;
  int rotated2 = 0;
  int rotated3 = 0;
  int rotated4 = 0;
  int rotationTime1[rotations];
  int rotationTime2[rotations];
  int rotationTime3[rotations];
  int rotationTime4[rotations];
  int timer1 = 0;
  int timer2 = 0;
  int timer3 = 0;
  int timer4 = 0;
  int stopTimer1 = 0;
  int stopTimer2 = 0;
  int stopTimer3 = 0;
  int stopTimer4 = 0;

  for(int i = 0; i <= 3; i++){
    delay(500);

    rotated1 = 0;
    rotated2 = 0;
    rotated3 = 0;
    rotated4 = 0;

    for (int j = 0; j < rotations; j++)
    {
      rotationTime1[j] = 0;
      rotationTime2[j] = 0;
      rotationTime3[j] = 0;
      rotationTime4[j] = 0;
    }
    
    if(motor[i].errors[0] == 0 && motor[i].errors[1] == 0){
      motor[i].On();
      timer1 = millis();
    } else{
      rotated1 = rotations;
    }
    if(motor[i+4].errors[0] == 0 && motor[i+4].errors[1] == 0 && i+4 <= count){
      motor[i+4].On();
      timer2 = millis();
    } else {
      rotated2 = rotations;
    }
    if(motor[i+8].errors[0] == 0 && motor[i+8].errors[1] == 0 && i+8 <= count){
      motor[i+8].On();
      timer3 = millis();
    } else {
      rotated3 = rotations;
    }
    if(motor[i+12].errors[0] == 0 && motor[i+12].errors[1] == 0 && i+12 <= count){
      motor[i+12].On();
      timer4 = millis();
    } else {
      rotated4 = rotations;
    }

    while(rotated1 < rotations+1 || rotated2 < rotations+1 || rotated3 < rotations+1 || rotated4 < rotations+1){

      if(motor[i].motorstate == 1 && digitalRead(50) == 1 && millis() - timer1 > 1000){
        rotationTime1[rotated1] = millis() - timer1;
        rotated1++;
        motor[i].Off();
        stopTimer1 = millis();
      }

      if(motor[i+4].motorstate == 1 && digitalRead(51) == 1 && millis() - timer2 > 1000){
        rotationTime2[rotated2] = millis() - timer2;
        rotated2++;
        motor[i+4].Off();
        stopTimer2 = millis();
      }

      if(motor[i+8].motorstate == 1 && digitalRead(52) == 1 && millis() - timer3 > 1000){
        rotationTime3[rotated3] = millis() - timer3;
        rotated3++;
        motor[i+8].Off();
        stopTimer3 = millis();
      }

      if(motor[i+12].motorstate == 1 && digitalRead(53) == 1 && millis() - timer4 > 1000){
        rotationTime4[rotated4] = millis() - timer4;
        rotated4++;
        motor[i+12].Off();
        stopTimer4 = millis();
      }

      if(motor[i].motorstate == 1 && motor[i].currentRead() != 0){
        motor[i].errors[4]++;
        break;
      }

      if(motor[i+4].motorstate == 1 && motor[i+4].currentRead() != 0){
        motor[i+4].errors[4]++;
        break;
      }

      if(motor[i+8].motorstate == 1 && motor[i+8].currentRead() != 0){
        motor[i+8].errors[4]++;
        break;
      }

      if(motor[i+12].motorstate == 1 && motor[i+12].currentRead() != 0){
        motor[i+12].errors[4]++;
        break;
      }

      if (rotated1 >= rotations){
        motor[i].Off();
        motor[i].errors[5] = 0;
        rotated1 = rotations+1;
      }

      if (rotated2 >= rotations){
        motor[i+4].Off();
        motor[i+4].errors[5] = 0;
        rotated2 = rotations+1;
      }

      if (rotated3 >= rotations){
        motor[i+8].Off();
        motor[i+8].errors[5] = 0;
        rotated3 = rotations+1;
      }

      if (rotated4 >= rotations){
        motor[i+12].Off();
        motor[i+12].errors[5] = 0;
        rotated4 = rotations+1;
      }

      
      
      if(motor[i].motorstate == 0 && motor[i+4].motorstate == 0 && motor[i+8].motorstate == 0 && motor[i+12].motorstate == 0){
        if (stopTimer1 > 500 && stopTimer2 > 500 && stopTimer3 > 500 && stopTimer4 > 500){
          if (rotated1 < rotations){
            motor[i].On();
            timer1 = millis();
          }
          
          if (rotated2 < rotations){
            motor[i+4].On();
            timer2 = millis();
          }

          if (rotated3 < rotations){
            motor[i+8].On();
            timer3 = millis();
          }

          if (rotated4 < rotations){
            motor[i+12].On();
            timer4 = millis();
          }
        }
        
      }


      TSPoint p = ts.getPoint();

      if(motor[i].motorstate == 1 && digitalRead(50) == 1 && millis() - timer1 > 1000){
        rotationTime1[rotated1] = millis() - timer1;
        rotated1++;
        motor[i].Off();
        stopTimer1 = millis();
      }

      if(motor[i+4].motorstate == 1 && digitalRead(51) == 1 && millis() - timer2 > 1000){
        rotationTime2[rotated2] = millis() - timer2;
        rotated2++;
        motor[i+4].Off();
        stopTimer2 = millis();
      }

      if(motor[i+8].motorstate == 1 && digitalRead(52) == 1 && millis() - timer3 > 1000){
        rotationTime3[rotated3] = millis() - timer3;
        rotated3++;
        motor[i+8].Off();
        stopTimer3 = millis();
      }

      if(motor[i+12].motorstate == 1 && digitalRead(53) == 1 && millis() - timer4 > 1000){
        rotationTime4[rotated4] = millis() - timer4;
        rotated4++;
        motor[i+12].Off();
        stopTimer4 = millis();
      }

      p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
      p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
      if (p.x > 170 && p.x < 310 && p.y > 400 && p.y < 470){
        motor[i].Off();
        motor[i+4].Off();
        motor[i+8].Off();
        motor[i+12].Off();

        draw(6);
        for (int i = 0; i < 16; i++){
          returnHome(i);
        }

        menu = 2;
        draw(menu);
        break;
      }

    }

    motor[i].Off();
    motor[i+4].Off();
    motor[i+8].Off();
    motor[i+12].Off();

    for (int h = 0; h < rotations; h++){
      if (motor[i].errors[0] == 0){
        if (rotationTime1[h] > rotationTimeMax){
          motor[i].errors[2]++;
        } else if (rotationTime1[h] < rotationTimeMin){
          motor[i].errors[3]++;
        }
      }
      
      if(motor[i+4].errors[0] == 0){
        if (rotationTime2[h] > rotationTimeMax){
          motor[i+4].errors[2]++;
        } else if (rotationTime2[h] < rotationTimeMin){
          motor[i+4].errors[3]++;
        }
      }

      if(motor[i+8].errors[0] == 0){
        if (rotationTime3[h] > rotationTimeMax){
          motor[i+8].errors[2]++;
        } else if (rotationTime3[h] < rotationTimeMin){
          motor[i+8].errors[3]++;
        }
      }

      if(motor[i+12].errors[0] == 0){
        if (rotationTime4[h] > rotationTimeMax){
          motor[i+12].errors[2]++;
        } else if (rotationTime4[h] < rotationTimeMin){
          motor[i+12].errors[3]++;
        }
      }
      
      motor[i].lastRotationTimes[0] = min1(rotationTime1, rotations);
      motor[i+4].lastRotationTimes[0] = min1(rotationTime2, rotations);
      motor[i+8].lastRotationTimes[0] = min1(rotationTime3, rotations);
      motor[i+12].lastRotationTimes[0] = min1(rotationTime4, rotations);

      motor[i].lastRotationTimes[1] = max1(rotationTime1, rotations);
      motor[i+4].lastRotationTimes[1] = max1(rotationTime2, rotations);
      motor[i+8].lastRotationTimes[1] = max1(rotationTime3, rotations);
      motor[i+12].lastRotationTimes[1] = max1(rotationTime4, rotations);

    }

    if (menu == 2){
      break;
    }


  }
  delay(500);
  for (int i = 0; i < 16; i++)
  {
    returnHome(i);
  }
  
}

void returnHome(int num){

  int timer = millis();
  if(motor[num].errors[0] == 0 && motor[num].errors[1] == 0){
    //num == 0 || num == 4 || num == 8 || num == 12
    if(num >=0 && num <= 3){
      timer = millis();
      motor[num].On();
      delay(5);
      while (digitalRead(50) == 0){
      }
      motor[num].Off();
      
    } else if (num >=4 && num <= 7){
      timer = millis();
      motor[num].On();
      delay(5);
      while (digitalRead(51) == 0){
      }
      motor[num].Off();
      
    } else if (num >= 8 && num <= 11){
      timer = millis();
      motor[num].On();
      delay(5);
      while (digitalRead(52) == 0){
      }
      motor[num].Off();
      
    } else if (num >= 12 && num <= 15){
      timer = millis();
      motor[num].On();
      delay(5);
      while (digitalRead(53) == 0){
      }
      motor[num].Off();
    
    }
  }
  
}

int mux (int nro){
  if (nro < 16 && nro >= 0){
    if (nro == 0 || nro == 4 || nro == 8 || nro == 12) {
      digitalWrite(2, 0);
      digitalWrite(3, 0);
      return 0;
    } else if (nro == 1 || nro == 5 || nro == 9 || nro == 13){
      digitalWrite(2, 1);
      digitalWrite(3, 0);
      return 0;
    } else if (nro == 2 || nro == 6 || nro == 10 || nro == 14){
      digitalWrite(2, 0);
      digitalWrite(3, 1);
      return 0;
    } else if (nro == 3 || nro == 7 || nro == 11 || nro == 15){
      digitalWrite(2, 1);
      digitalWrite(3, 1);
      return 0;
    }
  } else {
    
  }
}

void autoInit(){
  resTest();

  int check[16];
  for (int i = 0; i < 16; i++){
    check[i] = 0;
  }
  check[16] = 1;
  for (int j = 0; j < 16; j++){
    if(motor[j].resistance < resistanceTreshold){
      check[j] = 1;
    } else {
      check[j] = 0;
    }
  }

  int count = 16;
  
  while(count >= 0 && check[count] != 1){
    count--;
  }
  
  motorcount = count;

  for (int k = 0; k < 16; k++){
    returnHome(k);
  }
  
}

int min1(int rotationTimes[], int n){
  int minimum = rotationTimes[0];
  for (int i = 0; i < n; i++){
    if (rotationTimes[i] < minimum){
      minimum = rotationTimes[i];
    }
  }
  return minimum;
}

int max1(int rotationTimes[], int n){
  int maximum = rotationTimes[0];
  for (int i = 0; i < n; i++){
    if (rotationTimes[i] > maximum){
      maximum = rotationTimes[i];
    }
  }
  return maximum;
}

//------------------- MENU -----------------------//
void draw (int menu){

  if (menu == 0){

    tft.fillScreen(HX8357_BLACK);
    tft.drawRect(20, 190, 280, 120, HX8357_WHITE);
    tft.setTextColor(HX8357_WHITE);
    tft.setCursor(60, 220);
    tft.setTextSize(4);
    tft.println("Valipala");
    tft.setCursor(45, 260);
    tft.print("automaatti");

  } else if (menu == 1){

    tft.fillScreen(HX8357_BLACK);

    tft.setCursor(10, 15);
    tft.setTextSize(2);
    tft.setTextColor(HX8357_WHITE);
    tft.print("Test mode:");
    tft.drawRect(10, 40, 180, 70, HX8357_WHITE);
    tft.drawRect(200, 40, 110, 70, HX8357_WHITE);
    tft.setCursor(55, 60);
    tft.setTextSize(4);
    tft.print("Auto");
    tft.setCursor(240, 60);
    tft.setTextColor(HX8357_RED);
    tft.setTextSize(5);
    tft.print("X");
    tft.setTextColor(HX8357_WHITE);

     
    tft.setTextSize(2);
    tft.setCursor(70, 140);
    tft.print("Rotation count:");
    tft.setCursor(90, 270);
    tft.print("Motor count:");

    tft.setTextSize(4);
    tft.drawRect(110, 160, 100, 70, HX8357_WHITE);
    tft.setCursor(140, 180);
    tft.print(rotatecount);
     
    tft.drawRect(110, 290, 100, 70, HX8357_WHITE);
    tft.setCursor(140, 310);
    tft.print(motorcount + 1);

    tft.setTextSize(6);
    tft.drawCircle(55, 195, 30, HX8357_WHITE);
    tft.drawCircle(265, 195, 30, HX8357_WHITE);
    tft.setCursor(40, 175);
    tft.print("+");
    tft.setCursor(250, 175);
    tft.print("-");
     

    tft.drawCircle(55, 325, 30, HX8357_WHITE);
    tft.drawCircle(265, 325, 30, HX8357_WHITE);
    tft.setCursor(40, 305);
    tft.print("+");
    tft.setCursor(250, 305);
    tft.print("-");
     
     
    tft.drawRect(10, 400, 140, 70, HX8357_WHITE);
    tft.drawRect(170, 400, 140, 70, HX8357_CYAN);
    tft.setCursor(25, 420);
    tft.setTextSize(4);
    tft.setTextColor(HX8357_WHITE);
    tft.print("Start");
    tft.setTextColor(HX8357_CYAN);
    tft.setCursor(195, 420);
    tft.print("Stop");

  } else if (menu == 2){
    tft.fillScreen(HX8357_BLACK);

    tft.setCursor(10, 15);
    tft.setTextSize(2);
    tft.setTextColor(HX8357_WHITE);
    tft.print("Test mode:");
    tft.drawRect(10, 40, 180, 70, HX8357_WHITE);
    tft.drawRect(200, 40, 110, 70, HX8357_WHITE);
    tft.setCursor(30, 60);
    tft.setTextSize(4);
    tft.print("Single");
    tft.setCursor(240, 60);
    tft.setTextColor(HX8357_RED);
    tft.setTextSize(5);
    tft.print("X");
    tft.setTextColor(HX8357_WHITE);

     
    tft.setTextSize(2);
    tft.setCursor(80, 140);
    tft.print("Rotation count:");
    tft.setCursor(90, 270);
    tft.print("Select motor:");

    tft.setTextSize(4);
    tft.drawRect(110, 160, 100, 70, HX8357_WHITE);
    tft.setCursor(140, 180);
    tft.print(rotatecount);
    
    if(motorselect >= 0){
      tft.drawRect(110, 290, 100, 70, HX8357_WHITE);
      tft.setCursor(140, 310);
      tft.print(motorselect + 1);
    } else if (motorselect == -1){
      tft.drawRect(110, 290, 100, 70, HX8357_WHITE);
      tft.setCursor(130, 310);
      tft.print("All");
    }
    

    tft.setTextSize(6);
    tft.drawCircle(55, 195, 30, HX8357_WHITE);
    tft.drawCircle(265, 195, 30, HX8357_WHITE);
    tft.setCursor(40, 175);
    tft.print("+");
    tft.setCursor(250, 175);
    tft.print("-");
     

    tft.drawCircle(55, 325, 30, HX8357_WHITE);
    tft.drawCircle(265, 325, 30, HX8357_WHITE);
    tft.setCursor(40, 305);
    tft.print("+");
    tft.setCursor(250, 305);
    tft.print("-");
     
     
    tft.drawRect(10, 400, 140, 70, HX8357_WHITE);
    tft.drawRect(170, 400, 140, 70, HX8357_CYAN);
    tft.setCursor(25, 420);
    tft.setTextSize(4);
    tft.setTextColor(HX8357_WHITE);
    tft.print("Start");
    tft.setCursor(195, 420);
    tft.setTextColor(HX8357_CYAN);
    tft.print("Stop");
  } else if (menu == 3) {

    tft.fillScreen(HX8357_BLACK);

    tft.setCursor(55, 60);
    tft.setTextSize(4);
    tft.setTextColor(HX8357_WHITE);
    tft.print("Testing...");

    if(testMode == 0){
      tft.setTextSize(2);
      tft.setCursor(70, 140);
      tft.print("Rotation count:");
      tft.setCursor(90, 270);
      tft.print("Motor count:");

      tft.setTextSize(4);
      tft.drawRect(110, 160, 100, 70, HX8357_WHITE);
      tft.setCursor(140, 180);
      tft.print(rotatecount);
      
      tft.drawRect(110, 290, 100, 70, HX8357_WHITE);
      tft.setCursor(140, 310);
      tft.print(motorcount + 1);
    } else if (testMode == 1){
      tft.setTextSize(2);
      tft.setCursor(70, 140);
      tft.print("Rotation count:");
      tft.setCursor(90, 270);
      tft.print("Motor select:");

      tft.setTextSize(4);
      tft.drawRect(110, 160, 100, 70, HX8357_WHITE);
      tft.setCursor(140, 180);
      tft.print(rotatecount);
      
      tft.drawRect(110, 290, 100, 70, HX8357_WHITE);
      tft.setCursor(140, 310);
      tft.print(motorselect + 1);
    }

    tft.drawRect(10, 400, 140, 70, HX8357_CYAN);
    tft.drawRect(170, 400, 140, 70, HX8357_WHITE);
    tft.setCursor(25, 420);
    tft.setTextSize(4);
    tft.setTextColor(HX8357_CYAN);
    tft.print("Start");
    tft.setTextColor(HX8357_WHITE);
    tft.setCursor(195, 420);
    tft.print("Stop");

  } else if (menu == 4){

    #ifdef testing
      motor[4].errors[2] = 2;
    #endif

    tft.fillScreen(HX8357_BLACK);

    tft.setCursor(55, 20);
    tft.setTextSize(4);
    tft.setTextColor(HX8357_WHITE);
    tft.print("Results:");

    tft.setTextSize(2);
    int count = 0;
    for(int i = 0; i < 4; i++){
      for(int j = 0; j < 4; j++){
        if(count <= motorcount){
          tft.setTextColor(HX8357_WHITE);
          tft.drawRect(10 + j*75, 70 + i*80, 65, 70, HX8357_WHITE);
          tft.setCursor(25+j*75, 80+i*80);
          tft.print("M");
          tft.print(count+1);
        

          int error = 0;
          for(int k = 0; k < 6; k++){
            if(motor[count].errors[k] != 0){error++;}
          }

          if (error == 0) {
            tft.setTextColor(HX8357_GREEN);
            tft.setCursor(25+j*75, 80+i*80+25);
            tft.print("Ok");
          } else {
            tft.setTextColor(HX8357_RED);
            tft.setCursor(25+j*75, 80+i*80+25);
            tft.print("!!!");
          }
        }
        
        count++;
      }
    }

    tft.drawRect(80, 400, 140, 70, HX8357_WHITE);
    tft.setTextColor(HX8357_WHITE);
    tft.setTextSize(4);
    tft.setCursor(130, 420);
    tft.print("OK");
  } else if (menu == 5){
    tft.fillScreen(HX8357_BLACK);

    tft.setTextSize(3);
    tft.setCursor(20, 100);
    tft.setTextColor(HX8357_WHITE);
    tft.print("Continuing will");
    tft.setCursor(20, 130);
    tft.print("reset the tester");
    tft.setCursor(40, 200);
    tft.print("Are you sure?");
  

    tft.drawRect(10, 400, 140, 70, HX8357_WHITE);
    tft.drawRect(170, 400, 140, 70, HX8357_WHITE);
    tft.setCursor(40, 420);
    tft.setTextSize(4);
    tft.setTextColor(HX8357_WHITE);
    tft.print("YES");
    tft.setTextColor(HX8357_WHITE);
    tft.setCursor(215, 420);
    tft.print("NO");


  } else if (menu == 6) {
    tft.fillScreen(HX8357_BLACK);
    tft.setTextColor(HX8357_WHITE);
    tft.setCursor(60, 220);
    tft.setTextSize(4);
    tft.println("Returning");
    tft.setCursor(45, 260);
    tft.print("to home ...");
  
  } else if (menu >= 10 && menu <=25){
    tft.fillScreen(HX8357_BLACK);

    int switch1 = 0;
    switch (menu)
    {
    case 10:
      switch1 = 0;
      break;
    
    case 11:
      switch1 = 1;
      break;
    
    case 12:
      switch1 = 2;
      break;

    case 13:
      switch1 = 3;
      break;

    case 14:
      switch1 = 4;
      break;

    case 15:
      switch1 = 5;
      break;

    case 16:
      switch1 = 6;
      break;

    case 17:
      switch1 = 7;
      break;

    case 18:
      switch1 = 8;
      break;

    case 19:
      switch1 = 9;
      break;

    case 20:
      switch1 = 10;
      break;

    case 21:
      switch1 = 11;
      break;

    case 22:
      switch1 = 12;
      break;

    case 23:
      switch1 = 13;
      break;

    case 24:
      switch1 = 14;
      break;

    case 25:
      switch1 = 15;
      break;

    default:
      break;
    }

    tft.setCursor(30, 20);
    tft.setTextSize(3);
    tft.print("Faults for M");
    tft.print(switch1+1);

    tft.setTextSize(2);
    int row = 0;
    if (motor[switch1].errors[0] == 0 && motor[switch1].errors[1] == 0 && motor[switch1].errors[2] == 0 && motor[switch1].errors[3] == 0 && motor[switch1].errors[4] == 0 && motor[switch1].errors[5] == 0){
      tft.setCursor(30, 60);
      tft.setTextColor(HX8357_GREEN);
      tft.print("Motor OK!");
      tft.setTextColor(HX8357_WHITE);

    } else if(motor[switch1].errors[0] == 0 && motor[switch1].errors[1] == 0){
       if(motor[switch1].errors[2]!=0){
        tft.setTextColor(HX8357_YELLOW);
        tft.setCursor(30, 60+30*row);
        tft.print("Rotation long: ");
        tft.print(motor[switch1].errors[2]);
        row++;
        tft.setTextColor(HX8357_WHITE);
      } if(motor[switch1].errors[3]!=0){
        tft.setTextColor(HX8357_YELLOW);
        tft.setCursor(30, 60+30*row);
        tft.print("Rotation short: ");
        tft.print(motor[switch1].errors[3]);
        row++;
        tft.setTextColor(HX8357_WHITE);
      } if(motor[switch1].errors[4]!=0){
        tft.setTextColor(HX8357_RED);
        tft.setCursor(30, 60+30*row);
        tft.print("Over Current: ");
        tft.print(motor[switch1].errors[4]);
        row++;
        tft.setTextColor(HX8357_WHITE);
      } if(motor[switch1].errors[5] =! 0){
        tft.setTextColor(HX8357_YELLOW);
        tft.setCursor(30, 60+30*row);
        tft.print("Not done");
        row++;
        tft.setTextColor(HX8357_WHITE);
      }
    } else if (motor[switch1].errors[0]!=0) {
      tft.setTextColor(HX8357_RED);
      tft.setCursor(30, 60+30*row);
      tft.print("Resistance low: ");
      tft.print(motor[switch1].errors[0]);
      row++;
      tft.setTextColor(HX8357_WHITE);
    } else if(motor[switch1].errors[1]!=0){
      tft.setTextColor(HX8357_YELLOW);
      tft.setCursor(30, 60+30*row);
      tft.print("Resistance high: ");
      tft.print(motor[switch1].errors[1]);
      row++;
      tft.setTextColor(HX8357_WHITE);
    }

    tft.setCursor(30, 350);
    tft.setTextColor(HX8357_WHITE);
    tft.setTextSize(2);
    tft.print("Min rotation: ");
    int minRotation = motor[switch1].lastRotationTimes[0];
    tft.print(minRotation);
    tft.print(" ms");

    tft.setCursor(30, 380);
    tft.setTextColor(HX8357_WHITE);
    tft.setTextSize(2);
    tft.print("Max rotation: ");
    int maxRotation = motor[switch1].lastRotationTimes[1];
    tft.print(maxRotation);
    tft.print(" ms");

    tft.drawRect(80, 400, 140, 70, HX8357_WHITE);
    tft.setTextColor(HX8357_WHITE);
    tft.setTextSize(4);
    tft.setCursor(100, 420);
    tft.print("Back");
  } else if (menu == 100){
    tft.fillScreen(HX8357_BLACK);
    tft.setTextColor(HX8357_RED);
    tft.setCursor(40, 200);
    tft.setTextSize(5); 
    tft.print("PSU Voltage low");
    tft.setTextColor(HX8357_WHITE);
  }
}

void setup() {

  



  // Set motor drive pins to outputs
  for (int i = 0; i < 16; i++)
  {
    pinMode(i+22, OUTPUT);
  }
  pinMode(50, INPUT);
  pinMode(51, INPUT);
  pinMode(52, INPUT);
  pinMode(53, INPUT);

  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A5, INPUT);
  pinMode(A8, INPUT);
  pinMode(A9, INPUT);
  pinMode(A10, INPUT);
  pinMode(A11, INPUT);


  digitalWrite(4, 0);
  digitalWrite(5, 0);

  Serial.begin(9600);

  tft.begin();
  tft.fillScreen(HX8357_BLACK);

  // initialize motor objects
  for (int i = 0; i < motorcountdef; i++)
  {
    motor[i].add(i);
    motor[i].Off();
  }


  menu == 0;
  draw(menu);

  
}

void loop() {
  // Retrieve a point  
  TSPoint p = ts.getPoint();
  p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

  if (p.z > ts.pressureThreshhold && p.x > 0 && p.x < 320 && p.y > 0 && p.y < 480) {
    //ALKU
    if (menu == 0){
      if(p.x > 20 && p.x < 300 && p.y > 190 && p.y < 310){
        testMode = 0;
      
        #ifndef testing
          autoInit();
        #endif
        if (psuError != 0){menu = 100;draw(menu);} else {menu = 1;draw(menu);}
      }

    // AUTO test mode
    } else if (menu == 1) {
      if (p.x > 200 && p.x < 310 && p.y > 40 && p.y < 110){
        // ******** Takaisin nappula *********
        menu = 0;
        draw(menu);
      } else if (p.x > 10 && p.x < 190 && p.y > 40 && p.y < 110){
        // ******** Test mode nappula *********
        menu = 2;
        testMode = 1;
        draw(menu);
      } else if (p.x > 15 && p.x < 85 && p.y > 165 && p.y < 225){
        // ******** rotatecount + nappula *********
        if (rotatecount <= 1){
          rotatecount = 5;
          tft.fillRect(110, 160, 100, 70, HX8357_BLACK);
          tft.setTextSize(4);
          tft.drawRect(110, 160, 100, 70, HX8357_WHITE);
          tft.setCursor(140, 180);
          tft.print(rotatecount);
          delay(touchdelay);
        } else if (rotatecount <= 100){
          rotatecount = rotatecount + 5;
          tft.fillRect(110, 160, 100, 70, HX8357_BLACK);
          tft.setTextSize(4);
          tft.drawRect(110, 160, 100, 70, HX8357_WHITE);
          tft.setCursor(140, 180);
          tft.print(rotatecount);
          delay(touchdelay);
        }
      } else if (p.x > 235 && p.x < 295 && p.y > 165 && p.y < 225){
        // ******** rotatecount - nappula *********
        if (rotatecount > 5) {
          rotatecount = rotatecount - 5;
          tft.fillRect(110, 160, 100, 70, HX8357_BLACK);
          tft.setTextSize(4);
          tft.drawRect(110, 160, 100, 70, HX8357_WHITE);
          tft.setCursor(140, 180);
          tft.print(rotatecount);
          delay(touchdelay);
        } else {
          rotatecount = 1;
          tft.fillRect(110, 160, 100, 70, HX8357_BLACK);
          tft.setTextSize(4);
          tft.drawRect(110, 160, 100, 70, HX8357_WHITE);
          tft.setCursor(140, 180);
          tft.print(rotatecount);
          delay(touchdelay);
        }
      } else if (p.x > 15 && p.x < 85 && p.y > 295 && p.y < 355){
        // ******** Motorcount + nappula *********
        if (motorcount < 15 ){
          motorcount++;
          tft.fillRect(110, 290, 100, 70, HX8357_BLACK);
          tft.setTextSize(4);
          tft.drawRect(110, 290, 100, 70, HX8357_WHITE);
          tft.setCursor(140, 310);
          tft.print(motorcount + 1);
          delay(touchdelay);
        }
      } else if (p.x > 235 && p.x < 295 && p.y > 295 && p.y < 355){
        // ******** Motorcount - nappula *********
        if (motorcount > 0){
          motorcount--;
          tft.fillRect(110, 290, 100, 70, HX8357_BLACK);
          tft.setTextSize(4);
          tft.drawRect(110, 290, 100, 70, HX8357_WHITE);
          tft.setCursor(140, 310);
          tft.print(motorcount + 1);
          delay(touchdelay);
        }
      } else if (p.x > 10 && p.x < 150 && p.y > 400 && p.y < 470){
        // Start nappula
        menu = 3;
        draw(menu);
        #ifndef testing
          motorTest(motorcount, rotatecount);
        #endif
      }
    // SINGLE test mode
    } else if (menu == 2){
      if (p.x > 200 && p.x < 310 && p.y > 40 && p.y < 110){
        // ******** Takaisin nappula *********
        menu = 0;
        draw(menu);
      } else if (p.x > 10 && p.x < 190 && p.y > 40 && p.y < 110){
        // ******** Test mode nappula *********
        menu = 1;
        testMode = 0;
        draw(menu);
      } else if (p.x > 15 && p.x < 85 && p.y > 165 && p.y < 225){
        // ******** rotatecount + nappula *********
        if (rotatecount <= 1){
          rotatecount = 5;
          tft.fillRect(110, 160, 100, 70, HX8357_BLACK);
          tft.setTextSize(4);
          tft.drawRect(110, 160, 100, 70, HX8357_WHITE);
          tft.setCursor(140, 180);
          tft.print(rotatecount);
          delay(touchdelay);
        } else if (rotatecount <= 100){
          rotatecount = rotatecount + 5;
          tft.fillRect(110, 160, 100, 70, HX8357_BLACK);
          tft.setTextSize(4);
          tft.drawRect(110, 160, 100, 70, HX8357_WHITE);
          tft.setCursor(140, 180);
          tft.print(rotatecount);
          delay(touchdelay);
        } 
      } else if (p.x > 235 && p.x < 295 && p.y > 165 && p.y < 225){
        // ******** rotatecount - nappula *********
        if (rotatecount > 5) {
          rotatecount = rotatecount - 5;
          tft.fillRect(110, 160, 100, 70, HX8357_BLACK);
          tft.setTextSize(4);
          tft.drawRect(110, 160, 100, 70, HX8357_WHITE);
          tft.setCursor(140, 180);
          tft.print(rotatecount);
          delay(touchdelay);
        } else {
          rotatecount = 1;
          tft.fillRect(110, 160, 100, 70, HX8357_BLACK);
          tft.setTextSize(4);
          tft.drawRect(110, 160, 100, 70, HX8357_WHITE);
          tft.setCursor(140, 180);
          tft.print(rotatecount);
          delay(touchdelay);
        }
      } else if (p.x > 15 && p.x < 85 && p.y > 295 && p.y < 355){
        // ******** Motorselect + nappula *********
        if (motorselect < 15 ){
          motorselect++;
          tft.fillRect(110, 290, 100, 70, HX8357_BLACK);
          tft.setTextSize(4);
          tft.drawRect(110, 290, 100, 70, HX8357_WHITE);
          tft.setCursor(140, 310);
          tft.print(motorselect + 1);
          delay(touchdelay);
        }
      } else if (p.x > 235 && p.x < 295 && p.y > 295 && p.y < 355){
        // ******** Motorselect - nappula *********
        if (motorselect > -1){
          motorselect--;
          tft.fillRect(110, 290, 100, 70, HX8357_BLACK);
          tft.setTextSize(4);
          if(motorselect >= 0){
            tft.drawRect(110, 290, 100, 70, HX8357_WHITE);
            tft.setCursor(140, 310);
            tft.print(motorselect + 1);
          } else if (motorselect == -1){
            tft.drawRect(110, 290, 100, 70, HX8357_WHITE);
            tft.setCursor(130, 310);
            tft.print("All");
          }
          delay(touchdelay);
        }
      } else if (p.x > 10 && p.x < 150 && p.y > 400 && p.y < 470){
        //start nappula
          menu = 3;
          draw(menu);
          #ifndef testing
            resTest();
            if (motorselect >= 0){
              testMode = 1;
              motor[motorselect].test(rotatecount);
            } else if (motorselect == -1){
              testMode = 0;
              motorcount = 15;
              for (int i = 0; i < 16; i++) {
                if (menu != 2){
                  motor[i].test(rotatecount);
                }
              }
            } 
          #endif
          if (motorselect >= 0){
            menu = 10 + motorselect;
            draw(menu);
          } else if (motorselect = - 1){
            menu = 4;
            draw(menu);
          }
          
          
        }

    } else if(menu == 3){
        // testing mode
        #ifdef testing
        if (p.x > 10 && p.x < 150 && p.y > 400 && p.y < 470){
          menu = 4;
          draw(menu);
        }
        #endif
    } else if (menu == 4){
      //All results
      if (p.x > 10 && p.x < 300 && p.y > 70 && p.y < 380){
        for (int i = 0; i < 4; i++) {
          for (int j = 0; j < 4; j++){
            if (p.x > 10+j*75 && p.x < 10+65+j*75 && p.y > 70+i*80 && p.y < 70+70+i*80){
              if(i==0&&j==0&&motorcount>=0){
                menu = 10;
                draw(menu);
              } else if(i==0&&j==1&&motorcount>=1){
                menu = 11;
                draw(menu);
              } else if(i==0&&j==2&&motorcount>=2){
                menu = 12;
                draw(menu);
              } else if(i==0&&j==3&&motorcount>=3){
                menu = 13;
                draw(menu);
              } else if(i==1&&j==0&&motorcount>=4){
                menu = 14;
                draw(menu);
              } else if(i==1&&j==1&&motorcount>=5){
                menu = 15;
                draw(menu);
              } else if(i==1&&j==2&&motorcount>=6){
                menu = 16;
                draw(menu);
              } else if(i==1&&j==3&&motorcount>=7){
                menu = 17;
                draw(menu);
              } else if(i==2&&j==0&&motorcount>=8){
                menu = 18;
                draw(menu);
              } else if(i==2&&j==1&&motorcount>=9){
                menu = 19;
                draw(menu);
              } else if(i==2&&j==2&&motorcount>=10){
                menu = 20;
                draw(menu);
              } else if(i==2&&j==3&&motorcount>=11){
                menu = 21;
                draw(menu);
              } else if(i==3&&j==0&&motorcount>=12){
                menu = 22;
                draw(menu);
              } else if(i==3&&j==1&&motorcount>=13){
                menu = 23;
                draw(menu);
              } else if(i==3&&j==2&&motorcount>=14){
                menu = 24;
                draw(menu);
              } else if(i==3&&j==3&&motorcount>=15){
                menu = 25;
                draw(menu);
              }
            }
          }
        }
      } else if (p.x > 80 && p.x < 220 && p.y > 400 && p.y < 470){
        menu = 5;
        draw(menu);
      }
    } else if (menu == 5){
      //Clearing warning
      if(p.x > 10 && p.x < 150 && p.y > 400 && p.y < 470){
        for (int i = 0; i < 16; i++){
          for (int j = 0; j < 5; j++){
            motor[i].errors[j] = 0;
          }
        }
        menu = 0;
        draw(menu);
      } else if(p.x > 170 && p.x < 310 && p.y > 400 && p.y < 470){
        if(testMode == 0){
          menu = 4;
          draw(menu);
        } else if (testMode == 1){
          menu = 10 + motorselect;
          draw(menu);
        }
        
      }
    } else if (menu >= 10 && menu <= 25){
      //Single results
      if (p.x > 80 && p.x < 220 && p.y > 400 && p.y < 470){
        if (testMode == 0){
          menu = 4;
          draw(menu);
        } else if (testMode == 1){
          menu = 5;
          draw(menu);
        }
        
      }
    } else if (menu == 100){
      menu = 0; 
      draw(menu);
    }
    
  }

}

