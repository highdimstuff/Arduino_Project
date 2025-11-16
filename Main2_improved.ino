#include <LiquidCrystal.h> // Use the standard, built-in library

// Initialize the library with the pin numbers
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// LDR pins
const int ldrPin1 = A5; // Road 1, Sensor 1
const int ldrPin2 = A4; // Road 1, Sensor 2
const int ldrPin3 = A2; // Road 2, Sensor 1
const int ldrPin4 = A1; // Road 2, Sensor 2

// Parameters
int threshold = 350;
float speed_thr = 55; // cm/s
const float LDR_DISTANCE = 4; // cm

// Variables
float speed_clc1 = 0;
float speed_clc2 = 0;
float length_1 = 0;
float length_2 = 0;

// LED pins
const int ledPin1 = 8;
const int ledPin2 = 7;

// Time stamps for Road 1
unsigned long t1_b = 0;
unsigned long t1_ub = 0;
unsigned long t2_b = 0;
unsigned long t2_ub = 0;

// Time stamps for Road 2
unsigned long t3_b = 0;
unsigned long t3_ub = 0;
unsigned long t4_b = 0;
unsigned long t4_ub = 0;

// LED timing
const long ledDuration = 1000;
unsigned long ledStartTime_1 = 0;
unsigned long ledStartTime_2 = 0;

// Output reset timing
unsigned long outStartTime_1 = 0;
unsigned long outStartTime_2 = 0;


// ------------------- OUTPUT -------------------
void Output(float speed_clc, float length, int ledPin) {
    String objectType = (speed_clc > speed_thr) ? "Too Fast" : "Normal Speed";

    Serial.println("\n--- Measurement Complete ---");
    Serial.print("Road: "); Serial.println((ledPin == ledPin1) ? "1" : "2");
    Serial.print("Speed (cm/s): "); Serial.println(speed_clc, 3);
    Serial.print("Length (cm): "); Serial.println(length, 3);
    Serial.print("Classification: "); Serial.println(objectType);
    
    if (speed_clc > speed_thr) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Speed: "); lcd.print(speed_clc, 2); lcd.print("cm/s");
        lcd.setCursor(0, 1);
        lcd.print("SLOW DOWN!");
    } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Speed: "); lcd.print(speed_clc, 2); lcd.print("cm/s");
        lcd.setCursor(0, 1);
        lcd.print("Length: "); lcd.print(length, 2); lcd.print("cm");
    }
}


// ------------------- RESET -------------------
void Reset(float &speed_clc, unsigned long &outStartTime) {
    t1_b = t1_ub = t2_b = t2_ub = 0;
    t3_b = t3_ub = t4_b = t4_ub = 0;
    length_1 = length_2 = 0;
    speed_clc = 0;

    if (outStartTime != 0) {
        if (millis() - outStartTime >= ledDuration) {
            outStartTime = 0;
            lcd.clear();
        }
    }
}


// ------------------- READ SIGNAL -------------------
void Read_signal(int ldrValue1, int ldrValue2,
                 unsigned long &t_b1, unsigned long &t_b2,
                 unsigned long &t_ub1, unsigned long &t_ub2) {
    if (ldrValue1 > threshold && t_b1 == 0) t_b1 = micros();
    if (ldrValue2 > threshold && t_b2 == 0) t_b2 = micros();
    if (ldrValue1 < threshold && t_b1 != 0 && t_ub1 == 0) t_ub1 = micros();
    if (ldrValue2 < threshold && t_b2 != 0 && t_ub2 == 0) t_ub2 = micros();
    //Subdue false triggering (say a leaf has fallen or bird flew over)
    //Waiting condition in second ldr
    if (t_b1 ==0 && t_b2 !=0 && t_ub1 == 0 && t_ub2!=0){
    //Wait for 2 sec
      if(micros()-t_b2>=2000000){
        t_b2 = 0;
        t_ub2= 0;
      }
    }
    //Same condition for the first ldr
    if (t_b1 !=0 && t_b2 ==0 && t_ub1 != 0 && t_ub2==0){
        if(micros()-t_b1>=2000000){
        t_b1 = 0;
        t_ub1=0;
      }
    }
}


// ------------------- OPERATE -------------------
void Operate(unsigned long t_b1, unsigned long t_b2,
             unsigned long t_ub1, unsigned long t_ub2,
             unsigned long &ledStartTime, float &speed_clc,
             float &length, int ledPin) {
    if (t_b1 != 0 && t_b2 != 0 && t_ub1 != 0 && t_ub2 != 0){
    Serial.print("LDR1 time: ");
    Serial.println(t_b1);
    Serial.print("LDR2 time: ");
    Serial.println(t_b2);
    float del_t_travel = (t_b2 - t_b1) / 1000000.0;
    float del_t_blocked1 = (t_ub1 - t_b1) / 1000000.0;
    float del_t_blocked2 = (t_ub2 - t_b2) / 1000000.0;
    speed_clc = LDR_DISTANCE / del_t_travel;
    length = speed_clc * (del_t_blocked1 + del_t_blocked2) / 2;
    if (t_b2>t_b1){
    digitalWrite(ledPin, HIGH);
    ledStartTime = millis();
    }
    }
}


// ------------------- LED OFF -------------------
void led_off(int ledPin, unsigned long &ledStartTime) {
    if (digitalRead(ledPin) == HIGH) {
        if (millis() - ledStartTime >= ledDuration) {
            digitalWrite(ledPin, LOW);
            ledStartTime = 0;
        }
    }
}


// ------------------- SETUP -------------------
void setup() {
    pinMode(ledPin1, OUTPUT);
    pinMode(ledPin2, OUTPUT);
    Serial.begin(9600);
    lcd.begin(16, 2);
    lcd.print("System Ready");
}


// ------------------- LOOP -------------------
void loop() {
    led_off(ledPin1, ledStartTime_1);
    led_off(ledPin2, ledStartTime_2);
    //Read the inputs from the LDRs
    int ldrValue1 = analogRead(ldrPin1);
    int ldrValue2 = analogRead(ldrPin2);
    int ldrValue3 = analogRead(ldrPin3);
    int ldrValue4 = analogRead(ldrPin4);

    Read_signal(ldrValue1, ldrValue2, t1_b, t2_b, t1_ub, t2_ub);
    Read_signal(ldrValue3, ldrValue4, t3_b, t4_b, t3_ub, t4_ub);

    Operate(t1_b, t2_b, t1_ub, t2_ub, ledStartTime_1, speed_clc1, length_1, ledPin1);
    Operate(t3_b, t4_b, t3_ub, t4_ub, ledStartTime_2, speed_clc2, length_2, ledPin2);
    if (speed_clc1>0 && speed_clc2>0){
        digitalWrite(ledPin1, HIGH);
        ledStartTime_1 = millis();
        digitalWrite(ledPin2, HIGH);
        ledStartTime_2 = millis();
    }
    if ( speed_clc1>0 ){
         Output(speed_clc1, length_1, ledPin1);
         outStartTime_1 = millis();
         Reset(speed_clc1, outStartTime_1);
         }
    //Serial.print("LDR 1: ");Serial.println(ldrValue1);
    //Serial.print("LDR 2: ");Serial.println(ldrValue2);
    if (speed_clc2>0) {
        Output(speed_clc2, length_2, ledPin2);
        outStartTime_2 = millis();
        Reset(speed_clc2, outStartTime_2);
    }
    
    //delay(500);
}
