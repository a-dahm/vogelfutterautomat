#include <Arduino.h>

/* Konfiguration */
//Für Debugging-Zwecke: aktiviert die UART-Schnittstelle
#define SERIAL_DEBUG

//Bestimmt, ob Daten zwischengespeichert werden oder direkt hochgeladen werden
//Mögliche Werte:
// DIREKT_UPLOAD - Aufnahmen werden direkt per WLAN an den Server übertragen
// TAEGLICHER_UPLOAD - Aufnahmen werden auf der SD-Karte zwischengespeichert und 1x täglich bei Auslösung des RTC-Alarms hochgeladen
// LOKAL - Aufnahmen werden nur auf der SD-Karte gespeichert
#define DATEN_MODUS DIREKT_UPLOAD

//Wie lange soll gewartet werden, bevor ein Foto aufgenommen wird. Wird für automatische Belichtungssteuerung verwendet.
#define WARTE_VOR_FOTO 2

//Bestimmen, wie lang, in Sekunden, die jeweiligen Audioaufnahmen ausfallen sollen. Maximalwert: 240 Sekunden
#define AUDIOAUFNAHME_KURZ_DAUER 5
#define AUDIOAUFNAHME_LANG_DAUER 60

//WLAN-Einstellungen
#define WIFI_SSID "****************"
#define WIFI_PASSWORD "****************"
#define UPLOAD_SERVER "xxx.xxx.xxx.xxx"
#define UPLOAD_PORT 80
#define UPLOAD_PATH "/upload.php"

/* Definitionen für die einzelnen Module */
/*    Kamera */
#include "esp_camera.h"
//Definition der Pin-Belegung für das OV2640 Kameramodul
#define CAMERA_MODEL_XIAO_ESP32S3
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13
#define LED_GPIO_NUM      21

bool cam_initialized = false;
bool cam_init();
bool take_and_upload_photo();
bool take_and_save_photo();
bool take_and_upload_photo_httpclient();

/*    SD-Karte */
#include "FS.h"
#include "SD.h"
bool storage_initialized = false;
bool storage_init();

/*    WLAN */
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
bool wifi_initialized = false;
void wifi_init();
void wifi_deinit();
WiFiClient client;
#include <HTTPClient.h>

/*    RTC-Modul */
#include <Wire.h>
#include "RTClib.h"
bool rtc_initialized = false;
RTC_DS3231 rtc;
bool rtc_alarm_was_fired = false;
bool rtc_init();
bool rtc_set_time();
void rtc_print_config();

/*    Mikrofon */
#include <ESP_I2S.h>
#include "SPI.h"
I2SClass I2S;
#define SAMPLE_RATE 16000U
#define SAMPLE_BITS 16
#define WAV_HEADER_SIZE 44
#define VOLUME_GAIN 2
bool mic_init();
bool record_and_upload_wav(int duration_in_seconds);
bool record_and_save_wav(int duration_in_seconds);
void generate_wav_header(uint8_t *wav_header, uint32_t wav_size, uint32_t sample_rate);

/*    Arduino Kommunikation */
#define PIN_ARDUINO_SHUTDOWN_SIGNAL D0
void signal_shutdown();

/*    Debug-Ausgabe */
void log_debug(const char *msg) {
#ifdef SERIAL_DEBUG
  Serial.println(msg);
#endif
}

/* Hauptfunktion */
void setup() {
  pinMode(PIN_ARDUINO_SHUTDOWN_SIGNAL, OUTPUT);
  digitalWrite(PIN_ARDUINO_SHUTDOWN_SIGNAL, LOW);
  
#ifdef SERIAL_DEBUG
  Serial.begin(115200);
#endif
  //RTC initialisieren. Falls notwendig wird die aktuelle Zeit per WLAN/NTP gesetzt.
  //Es wird auch geprüft, ob der Alarm zurzeit ausgelöst ist.
  rtc_init();
  rtc_print_config();

  if(rtc_initialized && rtc_alarm_was_fired) {
    Serial.println("RTC wake");

    if(!mic_init()) {
      log_debug("ERR mic init failed");
    } else {
      #if DATEN_MODUS==DIREKT_UPLOAD
        record_and_upload_wav(AUDIOAUFNAHME_LANG_DAUER);
      #elif DATEN_MODUS==TAEGLICHER_UPLOAD || DATEN_MODUS==LOKAL
        record_and_save_wav(AUDIOAUFNAHME_LANG_DAUER);
      #endif

      #if DATEN_MODUS==TAEGLICHER_UPLOAD
        //TODO: alle Dateien von der SD-Karte hochladen
      #endif
    }
  } else {
    Serial.println("PIR wake");

    DateTime now = rtc.now();
    if(rtc_initialized && (now.hour() < 4 || now.hour() > 21)) { //TODO: Konfigurationsmöglichkeit
      log_debug("Keine Aufnahme in der Nacht.");
      signal_shutdown();
    }

    if(!cam_init()) {
      log_debug("ERR cam init failed");
    }else {
#if DATEN_MODUS==DIREKT_UPLOAD
      take_and_upload_photo();
#elif DATEN_MODUS==TAEGLICHER_UPLOAD || DATEN_MODUS==LOKAL
      take_and_save_photo();
#endif
    }

    if(!mic_init()) {
      log_debug("ERR mic init failed");
    } else {
#if DATEN_MODUS==DIREKT_UPLOAD
      record_and_upload_wav(AUDIOAUFNAHME_KURZ_DAUER);
#elif DATEN_MODUS==TAEGLICHER_UPLOAD || DATEN_MODUS==LOKAL
      record_and_save_wav(AUDIOAUFNAHME_KURZ_DAUER);
#endif
    }
  }
  Serial.println("DONE");
  wifi_deinit();
  signal_shutdown();
  while(1);
}

void loop() {
  if(rtc_initialized) {
    DateTime time = rtc.now();
    log_debug(time.timestamp(DateTime::TIMESTAMP_FULL).c_str());

    if(rtc.alarmFired(1)) {
      log_debug("RTC ALARM");
      delay(1000);
      rtc.clearAlarm(1);
    }
  } else {
    log_debug("INIT RTC FAILED");
  }
  
  delay(1000);
}

void signal_shutdown() {
  //Falls der RTC Alarm während der Laufzeit erneut ausgelöst wurde, zurücksetzen.
  //Zur Sicherheit, aber eigentlich nur für kurze Alarm-Abstände notwendig (Debug-Zwecke)
  if(rtc_initialized && rtc.alarmFired(1)) {
    rtc.clearAlarm(1);
  }

  digitalWrite(PIN_ARDUINO_SHUTDOWN_SIGNAL, HIGH);
  while(1) {
    if(rtc_initialized && rtc.alarmFired(1)) {
      rtc.clearAlarm(1);
    }
    delay(1000);
  }
}

//Initialisiert das Real-Time-Clock Modul und gibt zurück, ob dieser Vorgang erfolgreich war. Genauer bedeutet dies:
// 1. I2C Kommunikation wird hergestellt
// 2. Prüfe, ob Alarm ausgelöst wurde
// 3. Falls das RTC-Modul seinen Zustand verloren hat, neu konfigurieren
// 4. Alarm setzen
bool rtc_init() {
  log_debug("BEGIN rtc_init");
  if(rtc_initialized) {
    log_debug("rtc_init: already initialised");
    return true;
  }
  if(!rtc.begin()) {
    log_debug("rtc_init ERR rtc.begin()");
    return false;
  }
  
  rtc_alarm_was_fired = rtc.alarmFired(1);
  //Falls das RTC-Modul zurückgesetzt wurde wird es hier neu konfiguriert
  if(rtc.lostPower()) {
    log_debug("rtc_init: power was lost, initializing...");
    rtc.writeSqwPinMode(DS3231_OFF); //SQW-Pin kann als Signal-Generator verwendet werden, hier jedoch als Alarm-Pin
    rtc.disable32K(); //Signal-Generator-Pin wird nicht verwendet, deaktivieren
    log_debug("rtc_init: lost power, re-init");
    if(!rtc_set_time()) {
      return false;
    }
    rtc.disableAlarm(2);
  }
  
  //Alarme zurücksetzen
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);

  //Alarm für 5 Uhr morgens stellen
  //DS3231_A1_Hour -> Alarm auslösen, wenn Stunde, Minute und Sekunde mit der gestellten Zeit übereinstimmen
  rtc.setAlarm1(DateTime(2000, 1, 1, 5, 0, 0), DS3231_A1_Hour);

  rtc_initialized = true;
  log_debug("rtc_init DONE");
  return true;
}

bool rtc_set_time() {
  log_debug("BEGIN rtc_set_time");
  wifi_init(); //TODO: disable wifi at end of function
  WiFiUDP udp;
  NTPClient ntp(udp);
  //2 Stunden verschiebung zwischen UTC (Unix Time) und Berlin TZ
  ntp.setTimeOffset(2 * 60 * 60);
  ntp.begin();
  
  if(!ntp.update()) {
    log_debug("rtc_set_time ERR ntp.update()");
    return false;
  }
  
  log_debug("rtc_set_time: setting time");
  rtc.adjust(DateTime(ntp.getEpochTime()));

  ntp.end();
  return true;
}

//Gibt die aktuelle Uhrzeit und den Zustand von Alarm1 aus.
void rtc_print_config() {
  #ifndef SERIAL_DEBUG
  return;
  #endif
  if(!rtc_initialized) {
    Serial.println("rtc_print_config ERR not initialized");
    return;
  }
  Serial.println("Time       = " + rtc.now().timestamp(DateTime::timestampOpt::TIMESTAMP_FULL));
  DateTime alarm = rtc.getAlarm1();
  Serial.println("Alarm 1 Time = " + alarm.timestamp(DateTime::timestampOpt::TIMESTAMP_FULL));

  Ds3231Alarm1Mode alarm_mode = rtc.getAlarm1Mode();
  Serial.print("Alarm 1 Mode = ");
  switch (alarm_mode) {
  case DS3231_A1_PerSecond:
    Serial.println("DS3231_A1_PerSecond");
    break;
  case DS3231_A1_Second:
    Serial.println("DS3231_A1_Second");
    break;
  case DS3231_A1_Minute:
    Serial.println("DS3231_A1_Minute");
    break;
  case DS3231_A1_Hour:
    Serial.println("DS3231_A1_Hour");
    break;
  case DS3231_A1_Date:
    Serial.println("DS3231_A1_Date");
    break;
  case DS3231_A1_Day:
    Serial.println("DS3231_A1_Day");
    break;
  default:
    Serial.println("ERR");
  }
}

//Erstellt einen String anhand der aktuellen Uhrzeit im Format "YYYY-MM-DD_hh-mm-ss". Wird für Vergebung von Dateinamen verwendet.
String rtc_get_filename() {
  if(!rtc_initialized) {
    Serial.println("rtc_get_filename ERR not initialized");
    return String("default");
  }
  String fn = rtc.now().timestamp(DateTime::timestampOpt::TIMESTAMP_FULL);
  fn.replace(":", "-");
  return fn;
}

//Initialisiert die Kamera und gibt zurück, ob der Vorgang erfolgreich war.
//Hier wird die Pin-Belegung des Moduls, die Bildqualität sowie weitere Sensor-Einstellungen (z.B. Spiegelung) gesetzt.
bool cam_init() {
  log_debug("Initialising camera...");
  static camera_config_t config = {
      .pin_pwdn       = PWDN_GPIO_NUM,
      .pin_reset      = RESET_GPIO_NUM,
      .pin_xclk       = XCLK_GPIO_NUM,
      .pin_sccb_sda   = SIOD_GPIO_NUM,
      .pin_sccb_scl   = SIOC_GPIO_NUM,
      .pin_d7         = Y9_GPIO_NUM,
      .pin_d6         = Y8_GPIO_NUM,
      .pin_d5         = Y7_GPIO_NUM,
      .pin_d4         = Y6_GPIO_NUM,
      .pin_d3         = Y5_GPIO_NUM,
      .pin_d2         = Y4_GPIO_NUM,
      .pin_d1         = Y3_GPIO_NUM,
      .pin_d0         = Y2_GPIO_NUM,
      .pin_vsync      = VSYNC_GPIO_NUM,
      .pin_href       = HREF_GPIO_NUM,
      .pin_pclk       = PCLK_GPIO_NUM,

      .xclk_freq_hz   = 20000000,
      .pixel_format   = PIXFORMAT_JPEG,
      .frame_size     = FRAMESIZE_UXGA, //Die Auflösung des Fotos: FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
      .jpeg_quality   = 12, //JPEG-Qualität: zwischen 0 und 63 (niedriger = bessere Qualität)
      .fb_count       = 1,
      .fb_location    = CAMERA_FB_IN_PSRAM, //Bestimmt, dass die Frame-Buffer im PSRAM angelegt werden.
      .grab_mode      = CAMERA_GRAB_WHEN_EMPTY
  };
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("\tFAILED Camera init failed");
    return false;
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_brightness(s, -1);     // -2 to 2
  s->set_contrast(s, 0);       // -2 to 2
  s->set_saturation(s, 0);     // -2 to 2
  s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
  s->set_wb_mode(s, 1);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, 1);//  // 0 = disable , 1 = enable
  s->set_aec2(s, 1);           // 0 = disable , 1 = enable
  s->set_ae_level(s, 0);       // -2 to 2
  s->set_aec_value(s, 250);    // 0 to 1200
  s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);       // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
  s->set_bpc(s, 0);            // 0 = disable , 1 = enable
  s->set_wpc(s, 1);            // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
  s->set_lenc(s, 1);           // 0 = disable , 1 = enable
  s->set_hmirror(s, 1);        // 0 = disable , 1 = enable
  s->set_vflip(s, 1);          // 0 = disable , 1 = enable
  s->set_dcw(s, 1);            // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

  Serial.println("\tFinished.");
  return true;
}

//Stellt die Verbindung zum WLAN her und gibt zurück, ob die Verbindung erfolgreich war.
//TODO: Momentan gibt es hier keine Maximalzeit; es wird unendlich lang probiert.
void wifi_init() {
  if(wifi_initialized) return;
  Serial.print("Initialisiere WLAN...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("\tWLAN-Verbindung hergestellt");
  wifi_initialized = true;
}

//Trennt die WLAN-Verbindung.
void wifi_deinit() {
  if(!wifi_initialized) return;
  Serial.println("Deaktiviere WLAN...");
  WiFi.disconnect(); 
  WiFi.mode(WIFI_OFF);
  Serial.println("\tWLAN-Verbindung getrennt");
  wifi_initialized = false;
}

bool storage_init() {
  if(storage_initialized) return true;
  Serial.println("Initialisiere SD-Karte...");
  if(!SD.begin(21)) {
    Serial.println("\tFEHLER SD.begin() ist fehlgeschlagen");
    return false;
  }
  
  Serial.println("\tSD-Karte initialisiert");
  storage_initialized = true;
  return true;
}

//Nimmt ein Foto auf und startet sofort den Upload an den Server.
bool take_and_upload_photo() {
  Serial.println("Foto aufnehmen und hochladen...");
  delay(WARTE_VOR_FOTO * 1000);
  Serial.println("\tFrame-Buffer holen...");
  camera_fb_t * fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("\tFEHLER Frame-Buffer konnte nicht akquiriert werden");
    return false;;
  }
  
  if(!wifi_initialized) {
    wifi_init();
  }
  
  Serial.print("\tIP-Addresse: ");
  Serial.println(WiFi.localIP());
  Serial.print("\tVerbindung zum Server ");
  Serial.print(UPLOAD_SERVER);
  Serial.println("...");

  unsigned long transmission_start_time = millis();
  if(!client.connect(UPLOAD_SERVER, UPLOAD_PORT)) {
    Serial.println("\tFEHLER Server-Verbindung fehlgeschlagen");
    return false;
  }

  Serial.println("\tVerbunden");
  String fn = rtc_get_filename();
  Serial.println("\tDateiname: " + fn);
  String head = "--File\r\nContent-Disposition: form-data; name=\"uploadFile\"; filename=\"" + fn + ".jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--File--\r\n";

  uint32_t totalLen = fb->len + head.length() + tail.length();

  Serial.println("\tBeginne Upload...");
  Serial.println("\t\tHeaders");
  client.println("POST /esp32/upload.php HTTP/1.1");
  client.println("Host: " + String(UPLOAD_SERVER));
  client.println("Content-Length: " + String(totalLen));
  client.println("Content-Type: multipart/form-data; boundary=File");
  client.println();
  client.print(head);

  uint8_t *fbBuf = fb->buf;
  size_t fbLen = fb->len;
  size_t packetSize = 4096;
  Serial.print("\t\tBilddaten (");
  Serial.print(fbLen);
  Serial.println("b)");
  Serial.println("Fortschritt:");
  for(size_t n = 0; n < fbLen; n += packetSize) Serial.print(".");
  Serial.println();

  for (size_t n = 0; n<fbLen; n = n + packetSize) {
    if (n + packetSize < fbLen) {
      client.write(fbBuf, packetSize);
      fbBuf += packetSize;
    }
    else if (fbLen % packetSize > 0) {
      size_t remainder = fbLen % packetSize;
      client.write(fbBuf, remainder);
    }
    Serial.print(".");
  }
  Serial.println();
  Serial.println("\t\tFrame-Buffer freigeben und Kamera deaktivieren");
  esp_camera_fb_return(fb);
  esp_camera_deinit();
  Serial.println("\t\tFooter");
  client.print(tail);
  
  Serial.print("\tUpload abgeschlossen. Dauer: ");
  Serial.print(millis() - transmission_start_time);
  Serial.println(" ms");
  Serial.println("\tWarte auf Antwort des Servers...");

  int timeoutTimer = 10000;
  long startTimer = millis();
  String status = "";
  boolean done = false;
  while (!done && (startTimer + timeoutTimer) > millis()) {
    delay(10);
    //Es wird nur die erste Zeile ausgelesen, um den HTTP Response Code zu prüfen
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
      if (c == '\r' || c == '\n') {
        if (status.length() > 0) {
          done = true;
          break;
        }
      }
      else {
        status += String(c);
      }
    }
  }
  Serial.println();
  Serial.println("\tBeende Server-Verbindung");
  client.stop();
  
  Serial.print("\tStatus: ");
  Serial.println(String(status).c_str());
  if(status.indexOf("200") > -1) {
    return true;
  } else {
    return false;
  }
}

//Nimmt ein Foto auf und speichert dieses auf der SD-Karte.
bool take_and_save_photo() {
  Serial.println("Foto aufnehmen und auf SD-Karte speichern...");
  Serial.println("\tEin paar Sekunden warten...");
  delay(WARTE_VOR_FOTO * 1000);
  Serial.println("\tFrame-Buffer holen...");
  camera_fb_t * fb = esp_camera_fb_get();

  if(!fb) {
    Serial.println("\tFEHLER Frame-Buffer konnte nicht akquiriert werden");
    return false;
  }
  
  Serial.println("\tInitialisiere SD-Karte...");
  if(!SD.begin(21)){
      log_debug("\tFEHLER SD-Karte konnte nicht initialisiert werden");
      return false;
  }

  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
      log_debug("\tFEHLER Keine SD-Karte eingesteckt");
      return false;
  }

  //Das Foto erhält einen Dateinamen anhand der aktuellen Uhrzeit
  String timestamp = rtc_get_filename();
  String fileName = "/" + timestamp + ".jpg";
  Serial.println("\tFoto wird gespeichert unter: " + fileName);
  File file = SD.open(fileName.c_str(), FILE_WRITE);
  if(!file) {
    Serial.println("\tFEHLER Foto konnte nicht gespeichert werden");
    return false;
  }
  file.write(fb->buf, fb->len);
  file.close();
  
  Serial.println("\t\tFrame-Buffer freigeben und Kamera deaktivieren");
  esp_camera_fb_return(fb);
  esp_camera_deinit();
  return true;
}

bool mic_init() {
  Serial.println("Initialisiere Mikrofon...");
  Serial.println("\tBeginne I2S-Kommunikation");
  I2S.setPinsPdmRx(42, 41);

  if (!I2S.begin(I2S_MODE_PDM_RX, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
    Serial.println("\tFEHLER I2S-Kommunikation fehlgeschlagen");
    return false;
  }

  Serial.println("\tInitialisierung abgeschlossen");
  return true;
}

/*
  Führt eine Audioaufnahme durch und läd diese direkt als WAV-Datei hoch (ohne Verwendung der SD-Karte)
  Ablauf:
  - 1. Header generieren
  - 2. Audioaufnahme durchführen (Dauer wird bestimmt durch RECORD_TIME)
  - 3. Samples skalieren um Lautstärke zu erhöhen (bestimmt durch VOLUME_GAIN)
  - 4. WLAN aktivieren
  - 5. Verbindung zum Zielserver herstellen (UPLOAD_SERVER / UPLOAD_PORT)
  - 6. HTTP POST durchführen, um WAV-Datei hochzuladen
  - 7. Auf Antwort des Servers warten, um festzustellen, ob das Hochladen erfolgreich war
  - 8. WLAN deaktivieren
*/
bool record_and_upload_wav(int duration_in_seconds) {
  Serial.println("Audio aufnehmen und hochladen...");
  Serial.println("\tGeneriere WAV-Header");
  uint32_t record_size = (SAMPLE_RATE * SAMPLE_BITS / 8) * duration_in_seconds;
  uint32_t sample_size = 0;
  uint8_t wav_header[WAV_HEADER_SIZE];
  generate_wav_header(wav_header, record_size, SAMPLE_RATE);

  Serial.println("\tLege Buffer für Samples an");
  uint8_t *rec_buffer = (uint8_t *)ps_malloc(record_size);
  if (rec_buffer == NULL) {
    Serial.println("\tFEHLER malloc ist fehlgeschlagen");
    return false;
  }

  Serial.println("\tStarte Aufnahme...");
  sample_size = I2S.readBytes((char *)rec_buffer, record_size);
  if (sample_size == 0) {
    Serial.println("\tFEHLER Aufnahme fehlgeschlagen");
    return false;
  } else {
    Serial.printf("\t%d bytes aufgenommen\n", sample_size);
  }
  
  Serial.println("\tErhöhe Lautstärke der Aufnahme");
  for (uint32_t i = 0; i < sample_size; i += SAMPLE_BITS/8) {
    (*(uint16_t *)(rec_buffer+i)) <<= VOLUME_GAIN;
  }

  if(!wifi_initialized) {
    wifi_init();
    if(!wifi_initialized)
      return false;
  }
  Serial.println("\tVerbindung zum Server wird hergestellt...");
  if(!client.connect(UPLOAD_SERVER, UPLOAD_PORT)) {
    Serial.println("\tFEHLER Server-Verbindung fehlgeschlagen");
    return false;
  }
  Serial.println("\tVerbindung zum Server erfolgreich");
  String fn = rtc_get_filename();
  Serial.println("\tDateiname: " + fn);

  Serial.println("\tBeginne Upload...");
  String head = "--File\r\nContent-Disposition: form-data; name=\"uploadFile\"; filename=\"" + fn + ".wav\"\r\nContent-Type: audio/wav\r\n\r\n";
  String tail = "\r\n--File--\r\n";

  uint32_t totalLen = head.length() + WAV_HEADER_SIZE + record_size + tail.length();
  log_debug("\t\tHeader");
  client.println("POST " + String(UPLOAD_PATH) + " HTTP/1.1");
  client.println("Host: " + String(UPLOAD_SERVER));
  client.println("Content-Length: " + String(totalLen));
  client.println("Content-Type: multipart/form-data; boundary=File");
  client.println();

  Serial.println("\t\tForm-Header");
  client.print(head);
  Serial.println("\t\tWAV-Header");
  client.write(wav_header, WAV_HEADER_SIZE);
  Serial.println("\t\tWAV-Daten");
  client.write(rec_buffer, record_size);
  Serial.println("\t\tForm-Footer");
  client.print(tail);
  Serial.println("\t\tUpload abgeschlossen");
  Serial.println("\tBuffer wird freigegeben");
  free(rec_buffer);

  Serial.println("\tWarte auf Antwort des Servers");
  String status = "";
  boolean done = false;
  //Auf Antwort des Servers warten
  while (!done) {
    delay(10);
    //Es wird nur die erste Zeile ausgelesen, um den HTTP Response Code zu prüfen
    while (client.available()) {
      char c = client.read();
      if (c == '\r' || c == '\n') {
        if (status.length() > 0) {
          done = true;
          break;
        }
      }
      else {
        status += String(c);
      }
    }
  }
  client.stop();
  
  Serial.print("\tAntwort: ");
  Serial.println(status);
  if(status.indexOf("200") > -1) {
    return true;
  } else {
    return false;
  }
}

bool record_and_save_wav(int duration_in_seconds) {
  Serial.println("Audio aufnehmen und hochladen...");
  Serial.println("\tGeneriere WAV-Header");
  uint32_t record_size = (SAMPLE_RATE * SAMPLE_BITS / 8) * duration_in_seconds;
  Serial.println("Buffergroesse: " + record_size);
  uint32_t sample_size = 0;
  uint8_t wav_header[WAV_HEADER_SIZE];
  generate_wav_header(wav_header, record_size, SAMPLE_RATE);

  Serial.println("\tLege Buffer für Samples an");
  uint8_t *rec_buffer = (uint8_t *)ps_malloc(record_size);
  if (rec_buffer == NULL) {
    Serial.println("\tFEHLER malloc ist fehlgeschlagen");
    return false;
  }

  Serial.println("\tStarte Aufnahme...");
  sample_size = I2S.readBytes((char *)rec_buffer, record_size);
  if (sample_size == 0) {
    Serial.println("\tFEHLER Aufnahme fehlgeschlagen");
    return false;
  } else {
    Serial.printf("\t%d bytes aufgenommen\n", sample_size);
  }
  
  Serial.println("\tErhöhe Lautstärke der Aufnahme");
  for (uint32_t i = 0; i < sample_size; i += SAMPLE_BITS/8) {
    (*(uint16_t *)(rec_buffer+i)) <<= VOLUME_GAIN;
  }


  Serial.println("\tInitialisiere SD-Karte...");
  if(!SD.begin(21)){
      log_debug("\tFEHLER SD-Karte konnte nicht initialisiert werden");
      return false;
  }

  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
      log_debug("\tFEHLER Keine SD-Karte eingesteckt");
      return false;
  }

  //Die Datei erhält einen Dateinamen anhand der aktuellen Uhrzeit
  String timestamp = rtc.now().timestamp(DateTime::timestampOpt::TIMESTAMP_FULL);
  timestamp.replace(":", "-");
  String fileName = "/" + timestamp + ".wav";
  Serial.println("\tAufnahme wird gespeichert unter: " + fileName);
  File file = SD.open(fileName.c_str(), FILE_WRITE);
  if(!file) {
    Serial.println("\tFEHLER Aufnahme konnte nicht gespeichert werden");
    return false;
  }
  file.write(wav_header, WAV_HEADER_SIZE);
  file.write(rec_buffer, record_size);
  file.close();
  free(rec_buffer);

  return true;
}

//Generiert den Header für eine WAV-Datei und schreibt diesen in wav_header
//Siehe: http://soundfile.sapp.org/doc/WaveFormat/
void generate_wav_header(uint8_t *wav_header, uint32_t wav_size, uint32_t sample_rate)
{
  uint32_t size = wav_size + WAV_HEADER_SIZE - 8;
  uint32_t byte_rate = SAMPLE_RATE * SAMPLE_BITS / 8;
  const uint8_t set_wav_header[] = {
    'R', 'I', 'F', 'F',                       // ChunkID
    size, size >> 8, size >> 16, size >> 24,  // ChunkSize
    'W', 'A', 'V', 'E',                       // Format
    'f', 'm', 't', ' ',                       // Subchunk1ID
    0x10, 0x00, 0x00, 0x00,                   // Subchunk1Size = 16 -> PCM
    0x01, 0x00,                               // AudioFormat = 1 -> PCM)
    0x01, 0x00,                               // NumChannels = 1 channel
    sample_rate, sample_rate >> 8, sample_rate >> 16, sample_rate >> 24,  // SampleRate
    byte_rate, byte_rate >> 8, byte_rate >> 16, byte_rate >> 24,          // ByteRate
    0x02, 0x00,                               // BlockAlign
    0x10, 0x00,                               // BitsPerSample = 16 bits
    'd', 'a', 't', 'a',                       // Subchunk2ID
    wav_size, wav_size >> 8, wav_size >> 16, wav_size >> 24,  // Subchunk2Size
  };
  memcpy(wav_header, set_wav_header, sizeof(set_wav_header));
}