//Für Debugging-Zwecke: aktiviert die UART-Schnittstelle
#define ENABLE_SERIAL

#include "LowPower.h"

#define RTC_WAKE_PIN 2
#define PIR_WAKE_PIN 3
#define ESP_POWER_PIN A0
#define ESP_SIGNAL_PIN 10

void isr_rtc_wake();
void isr_pir_wake();

char wakeup_reason = 0;

void setup() {
#ifdef ENABLE_SERIAL
  Serial.begin(115200);
#endif

  pinMode(RTC_WAKE_PIN, INPUT_PULLUP);
  pinMode(PIR_WAKE_PIN, INPUT);
  pinMode(ESP_POWER_PIN, OUTPUT);
  digitalWrite(ESP_POWER_PIN, LOW);
  pinMode(ESP_SIGNAL_PIN, INPUT);
}

void loop() {
#ifdef ENABLE_SERIAL
  Serial.println("Betrete Schlafmodus");
  Serial.flush();
  delay(200);
#endif

  attachInterrupt(digitalPinToInterrupt(RTC_WAKE_PIN), isr_rtc_wake, LOW);
  attachInterrupt(digitalPinToInterrupt(PIR_WAKE_PIN), isr_pir_wake, HIGH);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  detachInterrupt(digitalPinToInterrupt(RTC_WAKE_PIN));
  detachInterrupt(digitalPinToInterrupt(PIR_WAKE_PIN));

  Serial.print(millis());
  Serial.println(" - Aufgeweckt");
  unsigned long timeout = 0;
  switch(wakeup_reason) {
    case 1:
      Serial.println("Aufweckgrund: RTC");
      //RTC wakeup -> 1 Minute Audio-Aufnahme
      timeout = 80 * 1000;
      break;
    case 2:
      Serial.println("Aufweckgrund: PIR");
      //PIR wakeup -> Foto + 5 Sekunden Audio-Aufnahme
      timeout = 45 * 1000;
    break;
    default:
      Serial.println("Aufweckgrund: Unbekannt!");
      break;
  }
  wakeup_reason = 0;

  //ESP hochfahren
  Serial.println("ESP wird hochgefahren");
  digitalWrite(ESP_POWER_PIN, HIGH);
  delay(1000);
  
  Serial.println("Warte auf Signal vom ESP...");
  unsigned long startTime = millis();
  Serial.println(startTime);
  //TODO: timeout
  while(digitalRead(ESP_SIGNAL_PIN) != HIGH) {
    /*if (millis() > (startTime + timeout)) {
      Serial.println("timeout");
      break;
    }
    delay(50);*/
  }
  
  Serial.println("ESP wird heruntergefahren");
  digitalWrite(ESP_POWER_PIN, LOW);
}

void isr_rtc_wake() { 
  wakeup_reason = 1;
}

void isr_pir_wake() {
  wakeup_reason = 2;
}