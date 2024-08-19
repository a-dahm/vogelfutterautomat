<p align="center">
    <img src="Bilder/gehaeuse_xray.png" width="500">
</p>

`⚠️ Dieses Projekt befindet sich zurzeit noch in der Prototyp-Phase. Der Prototyp ist funktionstüchtig, jedoch gibt es noch viel zu verbessern, damit das Gerät tatsächlich einen einsetzbaren Zustand erreicht. ⚠️`

# Vogelfutterautomat

Bei diesem Projekt handelt es sich um einen Vogelfutterautomat mit integrierter Kamera und Mikrofon zur automatischen Durchführung von Bild- und Audioaufnahmen von Vögeln. Das Gerät kann die gesammelten Daten per WLAN an ein externes System übertragen oder diese auf einer SD-Karte speichern. Als Stromversorgung dient ein Li-Po-Akku in Kombination mit einer Solarzelle.

## Repository-Übersicht

* Arduino: Firmware für den Arduino Pro Mini 3V
* ESP: Firmware für den Seeed Studio XIAO ESP32S3 Sense
* Server: Software für den PHP-Server
* Gehäuse: STL-Dateien für den 3D-Druck des Gehäuses

## Materialübersicht

Das gesamte für den Zusammenbau benötigte Material (mit beispielhaften Links):

* Arduino Pro Mini 3V [Berrybase](https://www.berrybase.de/arduino-pro-mini-328-3.3v/8mhz)
* Seeed Studio XIAO ESP32S3 Sense [Berrybase](https://www.berrybase.de/seeed-xiao-esp32s3-sense-esp32-s3r8-wlan-ble-5.0-ov2640-kamerasensor-8mb-psram-8mb-flash)
* DS3231 Real-Time-Clock-Modul [Eckstein-Shop](https://eckstein-shop.de/DS3231RTCModulI2CAT24C32forArduino2CwithLIR2032BatteryEN)
* DFRobot Solar Power Manager [Eckstein-Shop](https://eckstein-shop.de/DFRobotSolarPowerManager5VEN)
* Li-Po Akku
* Solarzelle
* OV2640 mit langem Ribbon-Cable und Weitwinkellinse [Amazon](https://amazon.de/dp/B0BXSL76L8)
* 6x M3-Schraube sowie Mutter (Gehäuse)
* 4x M2-Schraube sowie Mutter (Befestigung Platine)
* Lötmaterial (Platine, Verbindungskabel, Stiftleisten)