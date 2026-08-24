# Контроллер теплицы для Apple Home

Здесь вы найдете прошивку и схему контроллера теплицы, совместимого с Apple Home. Так же, здесь представлены Python скрипты для запуска логгера температуры и WEB-интерфейса контрля температуры, которые вы можете запустить на своем домашнем сервере. По любым вопросам добро пожаловать в мой [телеграм канале](t.me/drone_tales).  

**Используемые компоненты**

- LilyGO T-A7608E-H or T-A7608SA-H board - 1 шт.
- Датчик температуры DS18B20 - 1-10 шт.
- Транзистор 2N3906 - 1 шт.
- Транзистор 2N3904 - 1 шт.
- Резистор 100 Ohm - 1 шт.
- Резистор 1K - 2 шт.
- Резистор 4.7K - 1 шт.
- Резистор 47K - 1 шт.
- Конденсатор 3000mF x 6.3V - 1 шт.

**Используемые библиотеки Arduino**

- esp32 by Espressif Systems (board) 3.3.8
- esp32-ds18b20 2.0.3
- TinyGsmClient 0.12.0
- PubSubClient 2.8
 
**Настройки Arduino IDE**

- Board: ESP32 WROVER Kit (all versions)
- CPU Frequency: 80MHz (WiFi/BT)
- Core Debug Level: None
- Erase All Flash Before Sketch Upload: Disabled
- Flash frequency: 80Mhz
- Flash Mode: QIO
- Flash Size: 4MB (32Mb)
- Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
- PSRAM: Enabled
- Upload Speed: 921600

## 1. Подготовка домашней сети

Я предположу, что вы уже имеете настроенный и работающий у вас в сети **HomeBridge**. Вам также понадобится MQTT брокер, запущенный на вашем домашнем сервере. Следуйте [этим](https://github.com/DroneTales/VideoDoorbell) инструкциям для установки и настройки Mosquitto MQTT брокера. Я так же предполагаю, что вы знакомы с настройками вашего домашнего модема или роутера и способны произвести его конфигурацию.  

### 1.1. Проверка вашего IP адреса

В первую очередь мы должны узнать тип вашего внешнего IP адреса. Бывает три типа внешних IP адресов: статический белый, динамический белый и серый IP адрес. Термин **белый** означает, что IP адрес, назначенный вашему модему или роутеру, это тот же самый IP адрес, который виден из Internet. Термин **серый** означает, что IP адрес, назначенный вашему модему или роутеру, отличается от того, который виден из Internet (потому, что ваше устройство находится за NAT провайдеры). Я уверен, что если у вас белый внешний IP адрес, то вы точно об этом знаете, потому, что для получение такого адреса необходим договор с провайдером и ежемесячная оплата этой услуги. В этом случае вы можете смело пропустить части о серых адресах.  

Теперь нам нужно проверить тип вашего внешнего IP адреса. Для этого зайдите в WEB интерфейс вашего модема или роутера (далее я буду называть это устройство роутером) и посмотрите, какой адрес был назначен ему вашим Internet провайдером. Обычно, эта информация может быть найдена на странице "Статус". Если вы подключились к вашему роутеру по SSH, то выполните следующие команды, что бы посмотреть назначенный IP адрес.  

`cat /tmp/dhcp.leases`  
`ip addr show or ifconfig`  
`ip neigh`  
`arp -an`  

Точные команды зависят от прошивки вашего роутера. Но если вы работает с ним по SSH я могу смело предположить, что вы знаете как посмотреть IP адрес. Как только вы нашли назначенный IP адрес, откройте [эту](https://www.myip.com) страницу, что бы узнать, какой IP адрес видится извне.  

Если адреса совпадают (на странице вы видите тот же IP, что и на вашем роутере), то значит, что у вас белый внешний IP адрес, что очень хорошо. Если же IP адреса не совпадают, то у вас серый IP адрес и потребуются чуть больше работы для настройки внешнего подключения.  

Теперь, если у вас белый внешний IP и вы не уверены, статический он или динамический, то выполните следующие шаги, что бы узнать точный тип вашего внешнего IP адреса:  

- Запомните (или запишите) текущий IP адрес.
- **Выключите** ваш роутер.
- Подождите несколько минут.
- **Включите** ваш роутер.
- Проверьте назначенный IP адрес.

Возможно, потребуется выключить и включить можем несколько раз, для того, что бы провайдер сменил назначенный IP адрес. Если IP адрес не меняется, то у вас статический внешний IP адрес. Если меняется - то динамический.  

#### 1.1.1. Статический белый IP

Если у вас статический белый внешний IP, то никаких специальных действий делать не нужно.  

#### 1.1.2. Динамический белый IP

В случае, если у вас динамический белый внешний IP, вам понадобится настроить службу DynDNS на вашем роутере. Следуйте инструкции к вашему роутеру и выбранного DynDNS сервера для настройки.  

#### 1.1.3. Серый IP

Это худший вариант. Вам будет необходимо настроить **тунель** для того, что бы иметь возможность подключиться к вашей домашней сети из Internet. Так как мы будем в дальнейшем использовать **Cloudflare** в качестве DNS сервера, я рекомендую использоват **Cloudflare тунель**. Инструкции по настройки туннеля могут быть найдены [здесь](https://developers.cloudflare.com/tunnel/). Одно, прежде чем переходить к настройкам тунеля, прочитайте следующие части про доменное имя.  

### 1.2. Получение доменного имени

Если вы уже владеете каким-либо доменным именем, то вы можете пропустить эту часть. Однако, я рекомендую перенести ваш домен на Cloudflare потому, что мы будем использовать CloudFlare как DNS прокси для защиты вашей домашней сети от внешнего мира.  

Если вы не владеете доменным именем (для простоты - доменом) и у вас внешний белый статический или серый внешний IP адрес, то вам необходимо приобрести домен. Сервисов, которые предоставляют услуги регистрации доменных имен полно, но лично я использую два: Namecheap и Cloudflare. Для данного проекта будет лучше, если вы зарегистрируете доменное имя через Cloudflare, тогда вам не придется переносить его на Cloudflare позже.  

Если же у вас динамический белый внешний IP адрес, то доменное имя будет предоставлено сервисом DynDNS. Однако лучше обратиться к правилам оказания услуг вашего DynDNS сервиса для уточнения деталей.  

Далее я буду предполагать, что вы приобрели или перенесли ваш домен на Cloudflare. Если вы используете DynDNS, то можете пропустить все, что касается Cloudflare далее.  

### 1.3. Настройка записей DNS

К сожалению, если у вас динамический внешний IP и вы используете сервис DynDNS, то в большинстве случаев вы не сможете создать субдомены: имя вяшего домена будет выглядить как mygreenhouse.dyndns.com или что-то аналогичное. В других случаях (статический белый или серый IP), вы сможете создать субдомены и мы сделаем это для разделения графика и защиты домашней сети.  

Сайчса мы содадим два субдомена: **chart** и **broker**. Первый будет использоваться для доступа к WEB интерфейсу графика температур, а второй - для доступа к MQTT брокеру. Откройте панель управления доменом на Cloudflare (или другом регистраторе доменов, который вы используете). Добавьте две **A** записи. Допустим, что имя вашего домена **greenhouse.home** и ваш внешний IP **20.22.24.26**. Тогда первая **A** запись будет выглядеть как:  

`A  chart.greenhouse.home  20.22.24.26`  

Некоторые DNS сервера (точнее, их панели управления) используют обратную запись (что, в действительности, верно):  

`A  home.greenhouse.chart  20.22.24.26`  

В любом случе, добавьте вторую **A** запись для поддомена **broker**:  

`A  broker.greenhouse.home 20.22.24.26`  

или  

`A  home.greenhouse.broker  20.22.24.26`  

Если вы используете Cloudflare со статическим IP адресом, то сконфигурируйте первый субдомен (chart) как **Proxied**. Если у вас серый внешний IP, то следуйте инструкции по настройке тунеля Cloudflare. **Не забудьте открыть порты HTTP (80), HTTPS (443), и MQTT ports в настройках тунеля.** Вы можете использовать стандартный порт MQTT (1883), но будет лучше, если вы используете другой порт, например 23543.  

### 1.4. Настройка пробросал портов

Если вы используете CloudFlare тунель, то пропустите эту часть. В противном случае вам необходимо настроить проброс портов (Port Forwarding) на вашем роутере для того, что бы контроллер теплицы мог отправлять данные вашему MQTT брокеру и что бы вы имели доступ к WEB интерфейсу. Для этого зайдите в настройки вашего роутера и найдите раздел, который называется "Port Forwarding" или "Port Mapping". Добавьте следующие три правила, как показано в таблице ниже:  

| Comment |  Local IP    | Start port | End port | Protocol | Remote IP    | Start  port |   End port  |  
|:-------:|:------------:|:----------:|:--------:|:--------:|:------------:|:-----------:|:-----------:|
|  MQTT   | <homebridge> |   1883     |   1883   |   TCP    |              | <mqtt_port> | <mqtt_port> |  
|  HTTP   | <homebridge> |    80      |    80    |   TCP    |              |      80     |      80     |  
|  HTTPS  | <homebridge> |    443     |    443   |   TCP    |              |     443     |     443     |  

Где *<homebridge>* - IP адрес вашего HomeBridge сервера (скажем, *192.168.0.112*), а *<mqtt_port>* - порт MQTT брокера, который вы решили "выставить" наружу. Это может быть как стандартный MQTT порт - 1883, так и любой другой, который вам нравится. Например: *23543*.  

**Повторю: настраивать проброс портов нужно только в том случае, если у вас белый статический либо белый динамический внешний IP адрес. Если у вас серый внешний IP адрес, то все настройки должны быть произведены в клиенте и сервере Cloudflare туннеля. Обратитесь к инструкции по настройке туннеля Cloudflare за подробностями.**  

### 1.5. Проверка

Пригло время проверить ваши настройки и возможность доступа к вашему серверу из вне. Эта проверка не гарантирует, что ваш *HomeBridge* сервер доступен, она лишь проверяет ваш *роуетр*. Это позволит убедиться, что DNS настроены корректно. И так, откройте терминал (или командную строку на Windows) и выполните следующую команду:  

`ping <your_domain_name>`  

где *<your_domain_name>* - имя вашего субдомена, например:  

`ping chart.greenhouse.home`  
`ping broker.greenhouse.home`  

Вы должны увидеть что-то вроде этого:  

```
64 bytes from 20.22.24.26: icmp_seq=0 ttl=64 time=3.963 ms
64 bytes from 20.22.24.26: icmp_seq=1 ttl=64 time=4.059 ms
```

(Конечно, вместо 20.22.24.26, вы должны увидеть ваш IP адрес.). Если же вы видите что-то вроде:  

```
Request timeout for icmp_seq 0
Request timeout for icmp_seq 1
Request timeout for icmp_seq 2
```

значит что-то не так с настройками вашего DNS сервера или IP (может он **серый** или **за NAT**?).  

## 2. Определение адресов датчиков температуры

Теперь нам необходимо узнать адреса датчиков температуры, что бы мы могли их идетнифицировать в HomeBridge. Это не является необходимым, если у вас только один датчик, но если их несколько, то это очень важный шаг. Например у меня 4 датчика: 3 внутри теплицы и 1 снаружи. Два из тех, что внутри, расположены около дверей. И один - по центру теплицы. Для того, что бы знать, какой из них где, мне необходимо знать адрес каждого. Затем я помечу их цветными метками (например, термоусадкой). Прошивка поддерживает **вплоть дл 10** датчиков температуры.  

### 2.1. Сборка устройства

Прежде чем продожить мы должны собрать устройство. Используйте схему и вид платы из папки *wiring*. После сборки подлючите GSM антенну и не отключайте ее. Включение устройства без подключенной GSM антенны может вывести GSM модем из строя.  

При желании вы можете использовать любую плату на основе ESP32 для того, что бы определить адреса датчиков температуры, но я предположу, что вы будете использовать плату контроллера.  

### 2.2. Использование скетча FindSensors

Теперь откройте скетч **FindSensors** в Arduino IDE и прошейте вашу плату. После этого подключите первый сенсор и подключите плату. Откройте Serial Monitor в Arduino IDE. Если все собрано верно, то вы увидите адрес подключенного сенсора. Запишите этот адрес и пометьте сенсор каким-либо способом.  

Как это делал я: когда адрес сенсора найдет, я записывал его и помечал сенсор цветной термоусадкой. В итоге я получил таблицу вроде такой:  

9666714504207294760 - Черный  
18437195502557880616 - Белый  
290801794589212200 - Красный  
16791709353807948072 - Зеленый  

## 3. Настройка HomeBridge

Мы выполнили все необходимые приготовления и все выглядит работающим как надо. Так же у нас теперь есть список датчиков температуры. Теперь нам надо подготовить HomeBridge для получения данных с контроллера теплицы. Для этого откройте WEB интерфейс вашего HomeBridge, переключитесь на страницу *Plugins*, найдите и установите плагин **Homebridge MQTTThing**. Не беспокойтесь, он полностью совместим с последними версиями HomeBridge.  

После установки плагина перейдите на страницу JSON Config. Найдите раздел *"accessories"* в файле конфигурации. Если такой секйии там нет, то добавьте ее в самый конец конфигурационного файла (не забудьте добавить запятую после предыдущей секции):  

```
"accessories": [
]
```  

Если такая секция уже есть, то добавьте следующую конигурацию сразу за последним аксессуаром в этой секции (не забудьте про запятую!). Если вы только что создали этот раздел, то добавьте конфигурации между фигурными скобками (Я предполагаю, что у вас 4 датчика. Измените конфигурацию под ваши нужды при необходимости).  

```
{
    "accessory": "mqttthing",
    "type": "temperatureSensor",
    "name": "Outside",
    "manufacturer": "DroneTales",
    "model": "DS18B20",
    "firmwareRevision": "2.0",
    "serialNumber": "0_Black_966671",
    "minTemperature": -50,
    "maxTemperature": 60,
    "username": "mqtt_user_name",
    "password": "mqtt_password",
    "integerValue": true,
    "logMqtt": true,
    "topics": {
        "getBatteryLevel": "greenhouse/battery",
        "getStatusLowBattery": {
            "topic": "greenhouse/battery",
            "apply": "return message <= 20 ? true : false;"
        },
        "getCurrentTemperature": {
            "topic": "greenhouse/sensors/0",
            "apply": "if (!isNaN(message)) return message;"
        },
        "getStatusActive": {
            "topic": "greenhouse/sensors/0",
            "apply": "return isNaN(message) ? 0 : 1;"
        }
    }
},
{
    "accessory": "mqttthing",
    "type": "temperatureSensor",
    "name": "Zone 1",
    "manufacturer": "DroneTales",
    "model": "DS18B20",
    "firmwareRevision": "2.0",
    "serialNumber": "1_White_184371",
    "minTemperature": -50,
    "maxTemperature": 60,
    "username": "mqtt_user_name",
    "password": "mqtt_password",
    "integerValue": true,
    "logMqtt": true,
    "topics": {
        "getBatteryLevel": "greenhouse/battery",
        "getStatusLowBattery": {
            "topic": "greenhouse/battery",
            "apply": "return message <= 20 ? true : false;"
        },
        "getCurrentTemperature": {
            "topic": "greenhouse/sensors/1",
            "apply": "if (!isNaN(message)) return message;"
        },
        "getStatusActive": {
            "topic": "greenhouse/sensors/1",
            "apply": "return isNaN(message) ? 0 : 1;"
        }
    }
},
{
    "accessory": "mqttthing",
    "type": "temperatureSensor",
    "name": "Zone 2",
    "manufacturer": "DroneTales",
    "model": "DS18B20",
    "firmwareRevision": "2.0",
    "serialNumber": "2_Red_290801",
    "minTemperature": -50,
    "maxTemperature": 60,
    "username": "mqtt_user_name",
    "password": "mqtt_password",
    "integerValue": true,
    "logMqtt": true,
    "topics": {
        "getBatteryLevel": "greenhouse/battery",
        "getStatusLowBattery": {
            "topic": "greenhouse/battery",
            "apply": "return message <= 20 ? true : false;"
        },
        "getCurrentTemperature": {
            "topic": "greenhouse/sensors/2",
            "apply": "if (!isNaN(message)) return message;"
        },
        "getStatusActive": {
            "topic": "greenhouse/sensors/2",
            "apply": "return isNaN(message) ? 0 : 1;"
        }
    }
},
{
    "accessory": "mqttthing",
    "type": "temperatureSensor",
    "name": "Zone 3",
    "manufacturer": "DroneTales",
    "model": "DS18B20",
    "firmwareRevision": "2.0",
    "serialNumber": "3_Green_167917",
    "minTemperature": -50,
    "maxTemperature": 60,
    "username": "dronetales",
    "password": "dronetales",
    "integerValue": true,
    "logMqtt": true,
    "topics": {
        "getBatteryLevel": "greenhouse/battery",
        "getStatusLowBattery": {
            "topic": "greenhouse/battery",
            "apply": "return message <= 20 ? true : false;"
        },
        "getCurrentTemperature": {
            "topic": "greenhouse/sensors/3",
            "apply": "if (!isNaN(message)) return message;"
        },
        "getStatusActive": {
            "topic": "greenhouse/sensors/3",
            "apply": "return isNaN(message) ? 0 : 1;"
        }
    }
}
```

Я привел такую длинную конфигурация для того, что бы вы могли понять, как это устроено. Замените *mqtt_user_name* на имя пользователя вашего MQTT брокера, а *mqtt_password* - на пароль. Параметр *"name"* - имя сенсора по-умолчанию, которое появится в приложении Home после добавления сенсоров. Вы можете использовать любое, но я предпочитаю использовать нейтральные имена и потом переименовать сенсоры в приложении Home. Обратите внимание на параметр *"serialNumber"*. Я формировал его по следующему алгоритму *ID_сенсора_цвет_первые_6_цифр_адреса_сенсора*. Это позволяет определить сенсор в приложении Home и дать ему осмысленное имя в зависимости от расположения сенсора. Параметр *"topic"* должен оканчиваться порядковым номером сенсораp (0, 1, 2, 3, etc.). Надеюсь, это понятно.  

Сохраните конфигурацию и перезапустите HomeBridge. Если все сделано правильно, то после перезапуска HomeBridge вы увидите датчики температуры в приложении Home. Вам не нужно добавлять их вручную, они появятся автоматически.  

## 4. Прошивка контроллера теплицы

Откройте скетч *GreenHouse.ino* и прошейте плату. После этого подключите все сенсоры. **Не забудьте про GSM антенну!** Подключите аккумулятор 18650. Подключите внешнее питание (если требуется) к порту USB-C. **Не включайте плату!**  

### 4.1. Подготовка SD-Card

Now you need a micro SD card. The card is used to store the GSM, MQTT, and sensor configuration. Later, I will probably add logging to the card and other things I have in my plans. But currently, it is used to store the configuration file. So format the SD card as **FAT32** and copy the *config.txt* file to it. The file must be in the root of the card's file system.  

### 4.2. Updating the Configuration File

Now you should update the configuration file. Open it in any text editor (make sure that the selected text editor saves the file as ASCII and in raw text format). You will see the content of the file as shown below:  

```
[Board]
ShortSleep=120000000
LongSleep=900000000

[GPRS]
APN=mts
UserName=mts
Password=mts
PIN=1234

[MQTT]
Server=mqtt.server.name
Port=1883
UserName=mqtt_user_name
Password=mqtt_password
ClientID=GreenHouse

[Sensors]
Count=4
Address_0=9666714504207294760
Address_1=18437195502557880616
Address_2=290801794589212200
Address_3=16791709353807948072
```  

You can find few sections there.  

#### 4.2.1. The Board Section

This section contains two parameters: **ShortSleep** and **LongSleep**. The **ShortSleep** parameter is the sleep time in micro-seconds used when the board was not able to send temperature data. If the data sending fails, the board goes to sleep for this **ShortSleep** interval. The default value is 2 minutes. The **LongSleep** parameter is the sleep interval in micro-seconds used when the data was sent successfully. The default value is 15 minutes.  

#### 4.2.2. The GPRS Section

The *GPRS* section contains the mobile network parameters. You should provide the **APN**, the **username**, the **password**, and the SIM **PIN**. You can leave some of them empty if they are not required by your mobile network operator.  

#### 4.2.3. The MQTT Section

Here you should provide your MQTT broker settings. The **Server** parameter must be set to the MQTT server domain name (in our example above, we used *broker.greenhouse.home* as the MQTT server name, so you should set the **Server** parameter to this value). The **Port** parameter is the MQTT broker port number (in our example above, it is *5555*). The **UserName** and **Password** parameters are the MQTT broker *username* and *password*. The **ClientID** is the **unique** MQTT broker client ID. You can leave it as it is.  

#### 4.2.4. The Sensors Section

In this section, you should provide your temperature sensor addresses. The **Count** parameter must be set to the total number of sensors used in your configuration. Then each **Address_n** must be set to the corresponding *sensor address*. The **N** must correspond to the MQTT message number (*greenhouse/sensors/0*, *greenhouse/sensors/1*, etc.) in the MQTT Things HomeBridge plugin configuration.  

### 4.3. Testing the Hardware

Everything is ready for the first test. Connect the sensors to the board if you have not done so yet, insert the SD card with the configuration file, insert a SIM card, and power up the board.  

If everything is assembled correctly and the configuration file is valid, then in a few seconds you will see the red LEDs near the board's modem start flashing, indicating an attempt to send data. Once the LEDs turn off, wait a bit. If they start flashing again after about 2 minutes, then the data was not sent and there is something wrong with your mobile network or with your MQTT server configuration (domain, port forwarding, etc.). Check all these things. Try using MQTT Explorer (Windows) or something similar to connect to your MQTT broker by its domain name. Check the tunnel settings, if any, and check your DynDNS configuration.  

If the LEDs start flashing a second time after about 15 minutes, then everything is working as expected and you should see the received MQTT messages from the temperature sensors in the HomeBridge log window. You should also see the temperature readings for those sensors in your Home app.  

If the LEDs do not start flashing for a long time after the board is powered up, then there is something wrong with the configuration file or the SD card. Re-check the configuration file. Make sure the SD card was formatted as FAT32 and is readable.  

## 5. Temperature Logger

The project includes two applications: the **Temperature Logger** service and the **Temperature Monitor** web application. The applications are written in Python and JavaScript. They require **Python 3.11** and the **nginx** web server. In this section, you will find detailed instructions on how to set it all up. By [this](https://greenhouse.dronetales.net) link you can see how it works in real life.  

<img width="1680" height="1050" alt="Снимок экрана — 2026-05-30 в 21 35 41" src="https://github.com/user-attachments/assets/57321409-9ff9-43b6-a319-9f182bb773a2" />

### 5.1. Install Pre-Requirements

To be able to run the applications, you need to install Python 3.11 and nginx. If you already have either of them installed, simply skip this step.  

#### 5.1.1. Install Python

First, let's install Python 3.11. To do so, log in to your HomeBridge device via SSH and execute the following command:  

```
sudo apt update
sudo apt install python3.11 python3.11-venv
```  

#### 5.1.2. Install nginx

Now we must install the nginx web server and certbot to be able to use an SSL certificate from Let's Encrypt. To install nginx, execute the following command:  

`sudo apt install nginx certbot python3-certbot-nginx`  

### 5.2. Prepare the Directory Structure

We will store all the application files in the /opt/greenhouse/ directory. The directory structure looks like this:  

```
/opt/greenhouse/  
├── static/  
│   ├── css/  
│   │   └── style.css         # Styles definitions for web pages  
│   │  
│   └── js/  
│       ├── app.js            # Dashboard frontend  
│       ├── date-adapter.js   # Date-time adapter  
│       └── sensors.js        # Sensors page frontend  
│  
├── templates/  
│   ├── index.html            # Dashboard page  
│   └── sensors.html          # Sensors configuration page  
│  
├── .venv/                    # Python virtual environment  
├── const.py                  # Constants used by both applications  
├── db.py                     # Database settings  
├── logger.py                 # Temperature logger application  
├── sensors.py                # Sensors configuration  
├── server.py                 # Flask web application  
└── settings.py               # Application settings  
```

All the directories must be owned by the default user. In my case, it is the *homebridge* user. So in the commands below, replace *homebridge* with your username.  

```
sudo mkdir /opt/greenhouse
sudo chown homebridge:homebridge /opt/greenhouse
cd /opt/greenhouse
mkdir static
mkdir static/css
mkdir static/js
mkdir templates
```  

#### 5.2.1. Set Up the Python Virtual Environment

```
python3.11 -m venv .venv
source .venv/bin/activate
pip install Flask waitress
source deactivate
```

#### 5.2.2. Download Chart.js Library

```
cd /opt/greenhouse/static/js
wget -O chart.js https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js
```  

#### 5.2.3. Install Applications Files

Now copy all the files from the **Logger** directory to the **/opt/greenhouse** directory on your **HomeBridge** device. If you face any problems with copying the files, you can use a simple trick: use the nano text editor on the HomeBridge device, open the file locally, and copy-paste all the content into the nano editor. Not too fast, but it's a 100% working solution.  

Do not forget to modify the **settings.py** file by updating the following lines:  

```
MQTT_SERVER     = "mqtt_server_ip"
MQTT_USER_NAME  = "mqtt_user_name"
MQTT_PASSWORD   = "mqtt_password"
```

Replace *mqtt_server_ip* with the correct MQTT broker IP address; *mqtt_user_name* with the correct user name; and *mqtt_password* with the correct MQTT broker password.  

### 5.3. Setting Up System Services

Now we need to set up the applications to run as system services.  

#### 5.3.1. Run the Web App As a Systemd Service

Execute the following command:  

`sudo nano /etc/systemd/system/greenhouse-web.service`  

The copy and paste the following line into the create file:  

```
[Unit]
Description=Greenhouse Web Dashboard
After=network.target

[Service]
Type=simple
User=homebridge
Group=homebridge
WorkingDirectory=/opt/greenhouse
ExecStart=/opt/greenhouse/.venv/bin/python /opt/greenhouse/server.py
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

Enable and start the service.  

```
sudo systemctl daemon-reload
sudo systemctl enable greenhouse-web
sudo systemctl start greenhouse-web
```  

#### 5.3.2. Run the Logger App As a Systemd Service

Execute the following command:  

`sudo nano /etc/systemd/system/greenhouse-logger.service`  

The copy and paste the following line into the create file:  

```
[Unit]
Description=Greenhouse MQTT Logger
After=network.target

[Service]
Type=simple
User=homebridge
Group=homebridge
WorkingDirectory=/opt/greenhouse
ExecStart=/opt/greenhouse/.venv/bin/python /opt/greenhouse/logger.py
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
```  

Enable and start the **Logger** service.

```
sudo systemctl daemon-reload
sudo systemctl enable greenhouse-logger
sudo systemctl start greenhouse-logger
```

### 5.4. Create an Initial nginx Configuration

Execute the following command:  

`sudo nano /etc/nginx/sites-available/greenhouse`

The copy and paste the following lines to the just created file.  

```
server {
    listen 80;
    server_name chart.greenhouse.home;

    location / {
        proxy_pass http://127.0.0.1:5000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

Replace the *chart.greenhouse.home* with your domain name.  

Enable the site.  

```
sudo ln -s /etc/nginx/sites-available/greenhouse /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```  

### 5.5. Obtain SSL Certificate.

`sudo certbot --nginx -d chart.greenhouse.home`  

Replace the *chart.greenhouse.home* with your domain name.  

Follow the prompts. Certbot will automatically modify your Nginx config to listen on port 443 with SSL and set up HTTP→HTTPS redirection.  

#### 5.5.1. SSL Certificate Auto‑Renewal Setup

Certbot installs a systemd timer by default. Verify it’s active:  

`sudo systemctl status certbot.timer`  

It will renew certificates automatically twice a day. You can also test the renewal process with:  

`sudo certbot renew --dry-run`  

Re-load the nginx:  

`sudo nginx -t && sudo systemctl reload nginx`

## 6. Final test

Congratulations! Everything has been set up and should be running. Wait for some time (remember that the temperature data is sent every 15 minutes, so you will need to wait to collect some data in the database). Next, enter your domain address in your web browser. It should show the temperature graph!  

**I hope I did not forget anything important.**  

Should you have any questions, please do not hesitate to contact me at gully.horror0w@icloud.com.  

## 7. Support the author

If you like what I am doing, you can support me using one of the links below:  

**BuyMeACoffee**: https://buymeacoffee.com/dronetales  
**Boosty**: https://boosty.to/drone_tales/donate  
**PayPal**: mike@btframework.com  



