#include <Arduino.h>

int current_count;

#define A_PIN 8
#define B_PIN 9
#define LED_PIN 13

#define INTERVAL_MICRO 1000

unsigned long start_micros;
unsigned long led_micros;
int led_state;

void setup() {
  // put your setup code here, to run once:
  pinMode(A_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(A_PIN, 0);
  digitalWrite(B_PIN, 0);
  digitalWrite(LED_PIN, 0);

  current_count = 0;
  start_micros = micros();
  led_micros = micros();
  led_state = 0;
  
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long current_micros = micros();
  if((current_micros - start_micros) > INTERVAL_MICRO) {
    start_micros = current_micros;
    switch(current_count) {
      case 0:
        digitalWrite(A_PIN, 1);
        digitalWrite(B_PIN, 0);
        break;
      case 1:
        digitalWrite(A_PIN, 1);
        digitalWrite(B_PIN, 1);
        break;
      case 2:
        digitalWrite(A_PIN, 0);
        digitalWrite(B_PIN, 1);
        break;
      case 3:
        digitalWrite(A_PIN, 0);
        digitalWrite(B_PIN, 0);
        break;
    }
    current_count++;
    if (current_count >= 4) current_count = 0;
  }

  if((current_micros - led_micros) > 1E6) {
    led_micros = current_micros;
    led_state = !led_state;
    digitalWrite(LED_PIN, led_state);
  }
}
