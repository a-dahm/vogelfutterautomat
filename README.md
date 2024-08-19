<p align="center">
    <img src="Bilder/gehaeuse_xray.png" width="500">
</p>

`⚠️ Dieses Projekt befindet sich zurzeit noch in der Prototyp-Phase. Der Prototyp ist funktionstüchtig, jedoch gibt es noch viel zu verbessern, damit das Gerät tatsächlich einen einsetzbaren Zustand erreicht. ⚠️`

# Vogelfutterautomat

Bei diesem Projekt handelt es sich um einen Vogelfutterautomat mit integrierter Kamera und Mikrofon zur automatischen Durchführung von Bild- und Audioaufnahmen von Vögeln. Das Gerät kann die gesammelten Daten per WLAN an ein externes System übertragen oder diese auf einer SD-Karte speichern. Als Stromversorgung dient ein Li-Po-Akku in Kombination mit einer Solarzelle.

## Repository-Übersicht

* Arduino: Firmware für den Arduino Pro Mini 3V
* Bilder: Enthält die in diesem Dokument verwendeten Bilder
* ESP: Firmware für den Seeed Studio XIAO ESP32S3 Sense
* Gehäuse: STL-Dateien für den 3D-Druck des Gehäuses
* Server: Software für den PHP-Server

## Materialübersicht

Das gesamte für den Zusammenbau benötigte Material (mit beispielhaften Links):

* Arduino Pro Mini 3V [Berrybase](https://www.berrybase.de/arduino-pro-mini-328-3.3v/8mhz)
* Seeed Studio XIAO ESP32S3 Sense [Berrybase](https://www.berrybase.de/seeed-xiao-esp32s3-sense-esp32-s3r8-wlan-ble-5.0-ov2640-kamerasensor-8mb-psram-8mb-flash)
* DS3231 Real-Time-Clock-Modul [Eckstein-Shop](https://eckstein-shop.de/DS3231RTCModulI2CAT24C32forArduino2CwithLIR2032BatteryEN)
* PIR Modul (TODO)
* DFRobot Solar Power Manager [Eckstein-Shop](https://eckstein-shop.de/DFRobotSolarPowerManager5VEN)
* Li-Po Akku
* Solarzelle
* OV2640 mit langem Ribbon-Cable und Weitwinkellinse [Amazon](https://amazon.de/dp/B0BXSL76L8)
* 6x M3-Schraube sowie Mutter (Gehäuse)
* 4x M2-Schraube sowie Mutter (Befestigung Platine)
* Lötmaterial (Platine, Verbindungskabel, Stiftleisten)

## Anleitung

### Elektronik

### Firmware

Für das Anpassen und Überspielen der Firmware auf die beiden Mikroprozessor-Boards wird die [Arduino IDE](https://www.arduino.cc/en/software) benötigt. Als Einführung hier die offizielle (englische) Anleitung: [Link](https://docs.arduino.cc/software/ide-v2/tutorials/getting-started-ide-v2/)

Es werden für die Kompilierung die folgenden Bibliotheken benötigt, die zuvor über die Arduino IDE installiert werden müssen:

* [Low-Power (Rocket Scream Electronics)](https://www.arduino.cc/reference/en/libraries/low-power/): Verwaltung des Deep-Sleep-Modus für den Arduino Pro Mini 3V
* [RTCLib (Adafruit)](https://www.arduino.cc/reference/en/libraries/rtclib/): Kommunikation mit dem DS3231 Real-Time-Clock-Modul via I2C auf dem ESP32S3
* [NTPClient (Fabrice Weinberg)](https://www.arduino.cc/reference/en/libraries/ntpclient/): Für die Ermittlung der Uhrzeit via WLAN für das DS3231 Real-Time-Clock-Modul

#### Arduino Pro Mini 3V

Die Firmware für den Arduino Pro Mini befindet sich unter `Arduino/Arduino.ino`. Es sind hier keine Anpassungen am Quellcode notwendig.

Es ist jedoch wichtig zu beachten, dass vor dem Überspielen in der Arduino IDE unter "Werkzeuge" > "Processor" der Wert "ATmega328P (3,3V, 8 MHz)" ausgewählt wird (anstelle der 5V Variante):

<p align="center">
<img src="Bilder/anleitung-arduino-ide-arduino-pro-mini.png"/>
</p>

#### Seeed Studio XIAO ESP32S3 Sense

Die Firmware für den Seeed Studio XIAO ESP32S3 Sense befindet sich unter `ESP/ESP.ino`. Im Quellcode können die folgenden Einstellungen vorgenommen werden:

| Einstellung | Beschreibung | Beispiel |
| ----------- | ------------ | ------------ |
| `SERIAL_DEBUG` | Kann für Debug-Zwecke definiert werden, um Informationen über die UART-Schnittstelle des ESP32S3 ausgeben zu lassen | `//#define SERIAL_DEBUG` (deaktiviert) |
| `DATEN_MODUS` | Bestimmt, wie die mit den gesammelten Daten umgegangen wird. `DIREKT_UPLOAD`: Daten werden sofort bei Aufnahme hochgeladen und nicht zwischengespeichert. In diesem Modus wird keine SD-Karte benötigt. `TAEGLICHER_UPLOAD`: Aufnahmen werden auf der SD-Karte zwischengespeichert und am nächsten Morgen hochgeladen. `LOKAL`: Aufnahmen werden nur auf der SD-Karte gespeichert; die WLAN-Funktion wird hierbei dennoch eventuell für die Ermittlung der Uhrzeit verwendet, falls das RTC-Modul ausfällt. | `#define DATEN_MODUS DIREKT_UPLOAD` |
| `WARTE_VOR_FOTO` | Gibt eine Zeit (in Sekunden) an, die zwischen hochfahren und Foto gewartet werden soll. Dies dient dazu, dass sich der Kamera-Sensor an die Helligkeit der Umgebung anpassen kann, um unter-/überbelichteten Fotos vorzubeugen. | `#define WARTE_VOR_FOTO 2` |
| `AUDIOAUFNAHME_KURZ_DAUER` | Bestimmt die Länge der Audioaufnahme (in Sekunden) bei Aufweckung durch den Bewegungssensor. Maximalwert: 240 Sekunden. | `#define AUDIOAUFNAHME_KURZ_DAUER 5` |
| `AUDIOAUFNAHME_LANG_DAUER` | Bestimmt die Länge der Audioaufnahme (in Sekunden) bei Aufweckung durch das Real-Time-Clock-Modul. Dies dient dazu, den Vogelgesang am Morgen festzuhalten. Maximalwert: 240 Sekunden. | `#define AUDIOAUFNAHME_LANG_DAUER 60` |
| `WIFI_SSID` | Der Name des WLAN-Netzwerkes. | `#define WIFI_SSID "MeinWLAN"` |
| `WIFI_PASSWORD` | Das Passwort des WLAN-Netzwerkes. | `#define WIFI_PASSWORD "MeinPasswort"` |
| `UPLOAD_SERVER` | Die IP-Adresse des Webservers. | `#define UPLOAD_SERVER "192.168.178.50"` |
| `UPLOAD_PORT` | Der vom Webserver verwendete Port. Normalerweise ist dies bei HTTP der Port 80. | `#define UPLOAD_PORT 80` |
| `UPLOAD_PATH` | Der Pfad zum `upload.php`-Skript auf dem Webserver. Bei Ablage im Root-Verzeichnis lautet dieser `/upload.php` | `#define UPLOAD_PATH "/upload.php"` |

Vor dem Überspielen der Firmware muss unter "Werkzeuge" > "PSRAM" der Wert "OPI PSRAM" ausgewählt werden:

<p align="center">
<img src="Bilder/anleitung-arduino-ide-esp32s3.png"/>
</p>

### Gehäuse

### Server

Die Server-Seite verwendet HTML & PHP. Dafür muss eine entsprechende Server-Software auf einem geeigneten Gerät installiert werden. Das verwendete Gerät sollte soweit möglich durchgehend erreichbar sein, damit jederzeit Uploads stattfinden können. Es wird daher empfohlen, dafür zum Beispiel einen Raspberry Pi oder einen Mini-PC zu verwenden.

Um die Server-Seite zu hosten wird Apache & PHP verwendet. Die Installation dieser Komponenten hängt vom Betriebssystem ab:

* Unter Windows kann [XAMPP](https://www.apachefriends.org/de/index.html) verwendet werden.
* Für eine Anleitung zur Installation von Apache & PHP unter Linux sollte je nach Distribution nach einer Anleitung gesucht werden. Beispielsweise hier die [Installation unter Ubuntu](https://wiki.ubuntuusers.de/PHP/#Installation-fuer-serverseitige-Programmierung).

Nach der Installation können die Dateien aus dem "Server"-Verzeichnis dieses Repositories im entsprechenden Root-Verzeichnis des Webservers abgelegt werden: unter Windows der Ordner `Apache\htdocs`, unter Linux standardmäßig `/var/www/html`. Falls die Dateien in einem Unterverzeichnis abgelegt werden (zum Beispiel weil bereits andere Webseiten gehostet werden) muss der entsprechende Pfad in der `ESP.ino` eingetragen werden.

In dem Verzeichnis befinden sich auch zwei Log-Dateien `log.txt` und `errors.txt`. In diese Textdateien werden nach jedem erfolgten Upload rudimentäre Informationen festgehalten um potentielle Probleme zu diagnostizieren.

Damit die Webseite durch den ESP32S3 gefunden werden kann muss die IP-Adresse in der `ESP.ino` eingetragen werden (unter `#define UPLOAD_SERVER`).

* Anleitung für Windows [hier](https://support.microsoft.com/de-de/windows/suchen-sie-ihre-ip-adresse-in-windows-f21a9bbc-c582-55cd-35e0-73431160a1b9)
* Anleitung für Linux [hier](https://www.ionos.de/digitalguide/hosting/hosting-technik/linux-ip-adresse-anzeigen/)
