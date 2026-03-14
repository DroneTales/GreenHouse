# Датчик температуры для теплицы

Этот репозиторий содердит прошивку для устройства Apple HomeKit, о котором рассказано в [этом видео](https://youtu.be/).  

**Симулятор схем**

https://falstad.com/circuit/circuitjs.html

**Используемые компоненты**

- Плата LilyGO T-A7608E-H
- Датчик температуры DS18B20
- Транзисторы 2N3906 и 2N3904
- Резисторы 1K, 47K, 100, 4.7K
- Электролитический конденсатор 3000мкФ x 6.3V

**Используемые библиотеки Arduino**

- esp32 by Espressif Systems (board) 3.3.7
- esp32-ds18b20 2.0.3
- TinyGsmClient 0.12.0
- PubSubClient 2.8
 
**Настройки Arduino IDE**

- Board: ESP32 WROVER Kit (all versions)
- CPU Frequency: 80MHz (WiFi)
- Core Debug Level: None
- Erase All Flash Before Sketch Upload: Disabled
- Flash frequency: 80Mhz
- Flash Mode: QIO
- Flash Size: 4MB (32Mb)
- Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
- PSRAM: Enabled
- Upload Speed: 921600

**Поддержать автора**

Если вам нравится то, что я делаю и интересны новые проекты, вы можете поддержать меня удобным для вас способом, используя следующую информацию:  

**BuyMeACoffee**: https://buymeacoffee.com/dronetales  
**Boosty**: https://boosty.to/drone_tales/donate  
 
**BTC**: bitcoin:1A1WM3CJzdyEB1P9SzTbkzx38duJD6kau  
**BCH**: bitcoincash:qre7s8cnkwx24xpzvvfmqzx6ex0ysmq5vuah42q6yz  
**ETH**: 0xf780b3B7DbE2FC74b5F156cBBE51F67eDeAd8F9a  
