/* ---------------------------------------------
  Controling 4WD Rover

  ==PARTS==
  - 2 Arduino Uno/Nano
  - 2 H-bridge IC (SN754410)
  - 1 Shift Register (74HC595)
  - 2 2.4Ghz rf transivers (nRF24L01)
  - 1 Joystick
  - 1 4wd chassis (DAGU robot)

  ==PRINCIPLE==
  Shist register pins QA-QH are connected to all 8 h-bridge A pins (1A-2A,3A-4A).

  ==PWM & DAC==
  PWM Enable Pins must be filtered (Low Pass Filtering)
  for nice tork and smooth control of the motors' speed.
  On this toppic:
  https://www.allaboutcircuits.com/technical-articles/turn-your-pwm-into-a-dac/
  https://create.arduino.cc/projecthub/Arduino_Scuola/build-a-simple-dac-for-your-arduino-4c00bd

  --------------------------------------------- */

// Shift Register pins (digital!)
const int SER = 2;
const int LATCH = 3;
const int CLK = 4;

// H-bridge Enable pins (digital!)
const int EN1 = 5;
const int EN2 = 6;

// Initializing constants
int valSpeed = 0;
int valSteer = 0;
int velocity = 0;
int steer = 0;
int minpower = 100;
int minsteer = 100;
int idle = 510;     // idle pot value
int straight = 506; // idle pot value
int joystick[2];    // 2 element array holding Joystick readings
boolean debug = false;


/* - CONNECTIONS: nRF24L01 Modules See:
  http://arduino-info.wikispaces.com/Nrf24L01-2.4GHz-HowTo
   1 - GND
   2 - VCC  => 3.3V (!!)
   3 - CE => pin 9
   4 - CSN => pin 10
   5 - SCK => pin 13
   6 - MOSI => pin 11
   7 - MISO => pin 12
*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#define CE_PIN   9
#define CSN_PIN 10

const uint64_t pipe = 0xE8E8F0F0E1LL; // Define the transmit pipe

RF24 radio(CE_PIN, CSN_PIN); // Create a Radio

void setup()
{
  if (debug) {
    Serial.begin(9600);
    Serial.println("Nrf24L01 Receiver Starting");
  }
  delay(1000);

  // Set Enable pins
  pinMode(EN1, OUTPUT);
  pinMode(EN2, OUTPUT);

  // Set Shift Register pins
  pinMode(SER, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(CLK, OUTPUT);

  // Initiating radio
  radio.begin();
  radio.openReadingPipe(1, pipe);
  radio.startListening();;

  brake(); //Initialize with motor stopped
}

void loop()
{
  // RADIO TRANSMISSION
  if ( radio.available() )
  {
    // Read the data payload until we've received everything
    bool done = false;
    while (!done)
    {
      // Fetch the data payload
      done = radio.read( joystick, sizeof(joystick) );
    }
    valSpeed = joystick[1];
    valSteer = joystick[0];
    if (Serial && debug) {
      Serial.print("valSpeed = ");
      Serial.print(valSpeed);
      Serial.print(" valSteer = ");
      Serial.println(valSteer);
    }

    //check if engine is running
    if (valSpeed > idle + minpower || valSpeed < idle - minpower) {

      //go forward
      if (valSpeed > idle + minpower)
      {
        velocity = map(valSpeed, idle, 1023, 0, 255);
        forward(velocity, valSteer);
      }

      //go reverse
      if (valSpeed < idle - minpower)
      {
        velocity = map(valSpeed, idle, 0, 0, 255);
        reverse(velocity, valSteer);
      }
    }
    //skid steer
    else if (valSteer > 1024 - minsteer)
    {
      skid("L");
    }
    else if (valSteer < 0 + minsteer) {
      skid("R");
    }
    else
    {
      brake();
    }
  }
  else {
    if (Serial && debug) {
      Serial.println("No radio available");
    }
  }
}


// Motor goes forward at given rate (from 0-255)
void forward (int rate, int valsteer)
{
  digitalWrite(EN1, LOW);
  digitalWrite(EN2, LOW);

  digitalWrite(LATCH, LOW);
  shiftOut(SER, CLK, MSBFIRST, B10101010);
  digitalWrite(LATCH, HIGH);

  // Steer Left
  if (valsteer > straight + minsteer)
  {
    steer = map(valsteer, straight + minsteer, 1024, 255, 0);
    analogWrite(EN1, rate);
    analogWrite(EN2, steer);
  }

  // Steer Right
  else if (valsteer < straight - minsteer)
  {
    steer = map(valsteer, straight - minsteer, 0, 255, 0);
    analogWrite(EN1, steer);
    analogWrite(EN2, rate);
  }

  // Same speed on both sides
  else
  {
    analogWrite(EN1, rate);
    analogWrite(EN2, rate);
  }
  if (Serial && debug) {
    Serial.print("====> ");
    Serial.print("Speed = ");
    Serial.print(rate);
    Serial.print(" Steer = ");
    Serial.println(steer);
  }
}

//Motor goes reverse at given rate (from 0-255)
void reverse (int rate, int valsteer)
{
  digitalWrite(EN1, LOW);
  digitalWrite(EN2, LOW);

  digitalWrite(LATCH, LOW);
  shiftOut(SER, CLK, MSBFIRST, B01010101);
  digitalWrite(LATCH, HIGH);

  // Steer Left
  if (valsteer > straight + minsteer)
  {
    steer = map(valsteer, straight + minsteer, 1024, 255, 0);
    analogWrite(EN1, rate);
    analogWrite(EN2, steer);
  }

  // Steer Right
  else if (valsteer < straight - minsteer)
  {
    steer = map(valsteer, straight - minsteer, 0, 255, 0);
    analogWrite(EN1, steer);
    analogWrite(EN2, rate);
  }


  // Same speed on both sides
  else
  {
    analogWrite(EN1, rate);
    analogWrite(EN2, rate);
  }
  if (Serial && debug) {
    Serial.print("<==== ");
    Serial.print("Speed = ");
    Serial.print(rate);
    Serial.print(" Steer = ");
    Serial.println(steer);
  }

}

void skid (int valsteer) {
  int rate = 255;
  digitalWrite(EN1, LOW);
  digitalWrite(EN2, LOW);
  digitalWrite(LATCH, LOW);

  if (valsteer == "L")
  {
    shiftOut(SER, CLK, MSBFIRST, B01011010);
  }
  if (valsteer == "R")
  {
    shiftOut(SER, CLK, MSBFIRST, B10100101);
  }
  digitalWrite(LATCH, HIGH);
  analogWrite(EN1, rate);
  analogWrite(EN2, rate);
}

//Stops motor
void brake ()
{
  digitalWrite(EN1, LOW);
  digitalWrite(EN2, LOW);
  analogWrite(EN1, 0);
  analogWrite(EN2, 0);

  if (Serial && debug) {
    Serial.println("----- Vehicule Stopped!! -----");
  }
}


