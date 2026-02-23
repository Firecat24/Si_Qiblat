#include <QMC5883LCompass.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <Arduino.h>
#include "A4988.h"
#include <LiquidCrystal_I2C.h>

//----------------------------------------------//

#define satuButton 12

//----------------------------------------------//

QMC5883LCompass compass;
bool firstButtonPressedTemp, satuButtonState;
int GPSBaud = 9600;

TinyGPSPlus gps;
LiquidCrystal_I2C lcd(0x27,16,2);
SoftwareSerial gpsSerial(3, 2);
const int relayPin = 4; //newwwwwwwwwwwwwwwwwwwwwww
bool masuk = false;
bool tombolBerhenti = false;
bool sudahDua = false;
bool gpsNyala = false;
bool sudahKalibrasi = true;
bool sudahKalibrasiDua = true;
unsigned long buttonPressStartTime = 0;
const unsigned long debounceDelay = 50;
const unsigned long requiredPressDuration = 2000;
float declinationAngle = 0;
double tahunGPS, bulanGPS, hariGPS, jamGPS, menitGPS, detikGPS, bujurGPS, lintangGPS;
float a,b,c,d,e,f,azi;
int Step = 6;
int Dire  = 5;
int Sleep = 7;
int MS1 = 10;
int MS2 = 9;
int MS3 = 8;

//PENGATURAN MOTOR STEPPER DRIVER A4988

const int spr = 200;
float RPM = 15;
int Microsteps = 16;
A4988 stepper(spr, Dire, Step, MS1, MS2, MS3);

//----------------------------------------------//

float perhitunganArahKiblat(float lintangTempat, float bujurTempat){
  float lintangKabah = 21.42083333;
  float bujurKabah = 39.82777778;
  float radians = 0.0174603174603175;
  float degrees = 57.2957795;
  float aa,bb,CC,a,b,C,sin_a,sin_C,cotan_b,cos_a,cotan_C,cotan_B,Bc,B2,B,sudut_arah_kiblat;
  aa = 90 - lintangTempat;
  bb = 90 - lintangKabah;
  CC = bujurTempat - bujurKabah;
  a = radians * aa;
  b = radians * bb;
  C = radians * CC;
  sin_a = sin(a);
  sin_C = sin(C);
  cotan_b = 1/tan(b);
  cos_a = cos(a);
  cotan_C = 1/tan(C);
  cotan_B = (cotan_b*sin_a)/sin_C - cos_a*cotan_C;
  Bc = pow(cotan_B, -1);
  B2 = atan(Bc);
  B = B2 * degrees;
  sudut_arah_kiblat = 360 - B;
  return sudut_arah_kiblat;
}

//----------------------------------------------//

float kompas(float a, float b, float c, float d, float e, float f) {
    float kalibrasi[4];
    float total = 0;
    float kalibrasiAkhir = 0;
    compass.setCalibrationOffsets(a, b, c);
    compass.setCalibrationScales(d, e, f);

    for (int i = 0; i < 20; i++) {
        float azimuth;
        compass.read();

        float currentAzimuth = compass.getAzimuth();
        if (currentAzimuth >= 0 && currentAzimuth <= 180) {
            azimuth = currentAzimuth;
        } else if (currentAzimuth >= -180 && currentAzimuth < 0) {
            azimuth = 360 + currentAzimuth;
        } else {
            continue;
        }

        if (i < 16) {
            delay(100);
            continue;
        } else {
            kalibrasi[i - 1] = azimuth;
            total += kalibrasi[i - 1];
        }
    }

    kalibrasiAkhir = total / 4;
    return kalibrasiAkhir;
}


//-----------------------------------------------//

void setup() {
  pinMode(relayPin, OUTPUT); //newwwwwwwwwwwww
  digitalWrite(relayPin, HIGH); //newwwwwwwwwwwww
  Serial.begin(9600);
  pinMode(satuButton, INPUT_PULLUP);
  pinMode(Step, OUTPUT);
  pinMode(Dire,  OUTPUT);
  pinMode(Sleep,  OUTPUT);
  gpsSerial.begin(GPSBaud);
  lcd.backlight();
  lcd.init();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("ALAT DIHIDUPKAN");

  delay(2000);

  compass.init();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("MENCARI SINYAL");
  lcd.setCursor(0,1);
  lcd.print("GPS");

  delay(2000);
}


void loop() {
  while (gpsSerial.available() > 0){
    gps.encode(gpsSerial.read());
    if (gps.location.isUpdated() == 1){
      if (gps.date.year() > 2023){
        if (sudahDua == false){
          tahunGPS = gps.date.year();
          bulanGPS = gps.date.month();
          hariGPS = gps.date.day();
          jamGPS = gps.time.hour();
          menitGPS = gps.time.minute();
          detikGPS = gps.time.second();
          bujurGPS = gps.location.lng();
          lintangGPS = gps.location.lat();
          digitalWrite(Step, LOW);
          digitalWrite(Sleep, HIGH);
          digitalWrite(Dire, LOW);
          stepper.begin(RPM, Microsteps);

          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("MENDAPATKAN");
          lcd.setCursor(0,1);
          lcd.print("SINYAL GPS");

          delay(2000);

          sudahDua = true;
          sudahKalibrasi = false;
          digitalWrite(relayPin, LOW); //newwwwwwwwwwwww
          
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("LAKUKAN");
          lcd.setCursor(0,1);
          lcd.print("KALIBRASI");

          delay(3000);

          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("TEKAN TOMBOL SE-");
          lcd.setCursor(0,1);
          lcd.print("LAMA 3 DETIK");
        }
      }
    }
  }
  if(sudahDua == true){
    satuButtonState = digitalRead(satuButton);
    if (satuButtonState == LOW && sudahKalibrasi == false && firstButtonPressedTemp == false) {
      firstButtonPressedTemp = true;
      buttonPressStartTime = millis();
    } 

    if (satuButtonState == HIGH && sudahKalibrasi == false && masuk == false && tombolBerhenti == false && millis() - buttonPressStartTime < requiredPressDuration && firstButtonPressedTemp == true) {
      firstButtonPressedTemp = false;
    } 

    if (satuButtonState == HIGH && sudahKalibrasi == false && tombolBerhenti == false && millis() - buttonPressStartTime >= requiredPressDuration && firstButtonPressedTemp == true) {
      firstButtonPressedTemp = false;

      compass.init();

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("KALIBRASI KOMPASS");
      lcd.setCursor(0,1);
      lcd.print("AKAN DIMULAI");

      delay(5000);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("PUTAR-PUTAR ALAT");
      lcd.setCursor(0,1);
      lcd.print("KEKANAN KEKIRI");

      compass.calibrate();
      a = compass.getCalibrationOffset(0);
      b = compass.getCalibrationOffset(1);
      c = compass.getCalibrationOffset(2);
      d = compass.getCalibrationScale(0);
      e = compass.getCalibrationScale(1);
      f = compass.getCalibrationScale(2);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("KALIBRASI SUKSES");
      lcd.setCursor(0,1);
      lcd.print("LETAKKAN ALAT");

      delay(2000);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("ALAT SIAP");
      lcd.setCursor(0,1);
      lcd.print("DIGUNAKAN");

      delay(2000);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("TEKAN 1X UNTUK");
      lcd.setCursor(0,1);
      lcd.print("ARAH KIBLAT");
      sudahKalibrasiDua = false;
      masuk = true;
    }

    if (satuButtonState == HIGH && masuk == true && sudahKalibrasiDua == false && millis() - buttonPressStartTime < requiredPressDuration && firstButtonPressedTemp == true) {
      firstButtonPressedTemp = false;
      
      azi = kompas(a,b,c,d,e,f);

      delay(1000);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("BUJUR");
      lcd.setCursor(0,1);
      lcd.print(bujurGPS);

      delay(5000);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("LINTANG");
      lcd.setCursor(0,1);
      lcd.print(lintangGPS);

      delay(2000);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("AZIMUTH ALAT");
      lcd.setCursor(0,1);
      lcd.print(azi);

      delay(2000);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("SUDUT KIBLAT");
      lcd.setCursor(0,1);
      lcd.print(perhitunganArahKiblat(lintangGPS, bujurGPS));

      delay(2000);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("MENGARAH UTARA");
      lcd.setCursor(0,1);
      lcd.print("SEJATI");

      delay(1000);

      stepper.rotate(-azi);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("ARAH UTARA");
      lcd.setCursor(0,1);
      lcd.print("SEJATI");

      delay(3000);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("MENGARAH ");
      lcd.setCursor(0,1);
      lcd.print("KEKIBLAT");

      delay(1000);

      stepper.rotate(perhitunganArahKiblat(lintangGPS, bujurGPS));

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("ARAH KIBLAT");
      lcd.setCursor(0,1);
      lcd.print("TEKAN TOMBOL 1X");
      masuk = false;
      tombolBerhenti = true;
    }

    if (satuButtonState == HIGH && tombolBerhenti == true && sudahKalibrasiDua == false && millis() - buttonPressStartTime < requiredPressDuration && firstButtonPressedTemp == true) {
      firstButtonPressedTemp = false;

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("MENUJU PARKIR");

      delay(2000);

      stepper.rotate(-perhitunganArahKiblat(lintangGPS, bujurGPS));
      stepper.rotate(azi);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("POSISI PARKIR");
      masuk = true;
      tombolBerhenti = false;

      delay(5000);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("ALAT SIAP DIGU-");
      lcd.setCursor(0,1);
      lcd.print("NAKAN KEMBALI");
    }
  }
}