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

Below, I will assume that you have purchased a domain name and that it is controlled by Cloudflare. If you use DynDNS, then you can ignore all Cloudflare-related parts in the text below.  

### 1.3. Setting Up DNS Records

Unfortunately, if you have a dynamic external IP and use a DynDNS service, then in most cases you cannot have subdomains: your domain name will look like mygreenhouse.dyndns.com or something similar. In other cases (static white or gray external IP), you can create subdomains, and we will do so to separate traffic and protect our home network from attacks.  

Now we will create two subdomains: **chart** and **broker**. The first one will be used to access the temperature history chart, and the second one will be used to access your MQTT broker. Open your Cloudflare (or other) control panel and go to the DNS settings. Add two **A** records. Let's say your domain is greenhouse.home and your external IP is 20.22.24.26. Then the first **A** record should look like this:  

`A  chart.greenhouse.home  20.22.24.26`  

Some DNS servers use it in reverse (which is, actually, the correct form):  

`A  home.greenhouse.chart  20.22.24.26`  

Anyway, add the second one for the **broker** subdomain:  

`A  broker.greenhouse.home 20.22.24.26`  

or  

`A  home.greenhouse.broker  20.22.24.26`  

If you use Cloudflare with a static IP, then set the first one (chart) as **Proxied**. If you have a gray external IP address, then follow the Cloudflare tunnel setup instructions to find out how to add subdomains. **Do not forget to open the HTTP (80), HTTPS (443), and MQTT ports in the tunnel settings.** You can use the standard MQTT port (1883), but it is better to use a different one known only to you, for example, 23543 or something like that. Remember that port. You will need it later.  

### 1.4. Router Port Forwarding

If you use a Cloudflare tunnel (meaning you have a gray IP), skip this part. In other cases (DynDNS with a white dynamic or static IP), you need to set up port forwarding on your router so that applications (your web browser and the greenhouse controller) can connect to your HomeBridge device, which is located on your home local network and isolated from internet access by your router.  

OK, log in to your router's web interface and find the settings called "Port Forwarding" or "Port Mapping". Add three rules as shown in the table below:  

| Comment |  Local IP    | Start port | End port | Protocol | Remote IP    | Start  port |   End port  |  
|:-------:|:------------:|:----------:|:--------:|:--------:|:------------:|:-----------:|:-----------:|
|  MQTT   | <homebridge> |   1883     |   1883   |   TCP    |              | <mqtt_port> | <mqtt_port> |  
|  HTTP   | <homebridge> |    80      |    80    |   TCP    |              |      80     |      80     |  
|  HTTPS  | <homebridge> |    443     |    443   |   TCP    |              |     443     |     443     |  

Where *<homebridge>* is your HomeBridge device's IP address (let's say, *192.168.0.112*) and *<mqtt_port>* is the port you decided to open for external access to your MQTT broker. It can be the standard 1883 or any port you would like, for example, *23543*, as shown in the example in the previous chapter.  

**Once again, you will need to configure port forwarding only if you have a white static or white dynamic external IP address. If you have a gray external IP address, you need to do all the port configuration in the Cloudflare tunnel client and server; refer to the Cloudflare tunnel instructions.**  

### 1.5. Testing

Once you have completed all the configurations, it's time to test if your router is accessible from the Internet. This does not check if your *HomeBridge* device is accessible, only your *router*. This allows you to verify that your DNS settings are correct and working. (However, if your external IP is gray, it may help to check if your HomeBridge is also accessible, but it may not if Cloudflare blocks ping.) So, open the terminal (or Command Prompt on Windows) and execute the following command:  

`ping <your_domain_name>`  

where *<your_domain_name>* is the name of your subdomain, for example:  

`ping chart.greenhouse.home`  
`ping broker.greenhouse.home`  

You should see responses like the ones below (or similar):  

```
64 bytes from 20.22.24.26: icmp_seq=0 ttl=64 time=3.963 ms
64 bytes from 20.22.24.26: icmp_seq=1 ttl=64 time=4.059 ms
```

(Of course, instead of 20.22.24.26, you will see your own IP address.). If you see something like this:  

```
Request timeout for icmp_seq 0
Request timeout for icmp_seq 1
Request timeout for icmp_seq 2
```

then there is something wrong with your DNS settings or your IP (maybe it is **gray** or **behind a NAT**?).  

## 2. Finding Temperature Sensor Addresses

We need to know the temperature sensor addresses now because we will use them in HomeBridge to identify the sensors. It is not necessary if you have just one, but if you have several, this is a very important step. For example, I have 4 sensors: 3 inside the greenhouse and 1 outside. Two of the three inside are located near the doors, and the third one is in the middle of the greenhouse. To be able to identify them, I need to know the address of each. Then I labeled them with colour labels. The firmware supports **up to 10** temperature sensors.  

### 2.1. Assembly the Device

Before we can continue, you should assemble the device. Use the schematic and the board view located in the *wiring* folder. After assembling, attach the GSM antenna and do not detach it. Turning on the device without the GSM antenna may damage the modem.  

Actually, you can use any ESP32 to find the sensor addresses, but I assume you will use the same board as for the Greenhouse Controller. For details, refer to the comments in the **FindSensors** sketch.  

### 2.2. Using the FindSensors Sketch

Now open the **FindSensors** sketch in the Arduino IDE and flash it to the board. Once done, connect the first sensor and power up the board. Open the Arduino IDE Serial Monitor window. There you will see the address of your sensor. Write it down and mark the sensor somehow so that later you can tell which sensor has which address.  

How I did that: When a sensor address was found, I wrote it down and marked the sensor with a colour label. Finally, I got a list like the one below:  

9666714504207294760 - Black  
18437195502557880616 - White  
290801794589212200 - Red  
16791709353807948072 - Green  

## 3. Configuring HomeBridge

We have all the external things set up and they look like they are working well. We also have a list of our temperature sensor addresses. Now we must prepare HomeBridge to receive temperature data from our Greenhouse Controller. To do that, open your HomeBridge web interface, switch to the *Plugins* page, and find and install the **Homebridge MQTTThing** plugin. Do not worry, it is fully compatible with the latest version of HomeBridge.  

When the plugin is installed, go to the JSON Config page. Find the *"accessories"* section in the JSON configuration file. If there is no such section, add one at the very end of the configuration file, as shown below (do not forget to add a comma after the previous section's closing bracket):  

```
"accessories": [
]
```  

If the section already exists, add the following configuration right after the last accessory in the section (do not forget to add a comma after the closing bracket of the previous accessory). If you just created the section, add the configuration between the square brackets (I assume you have four temperature sensors and labeled them as in my example above. Change the configuration to suit your needs by removing some sections if you have fewer sensors or by adding new sections if you have more sensors).  

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

I have provided such a long configuration example so that you can figure out how it works. Replace *mqtt_user_name* with your MQTT broker username and *mqtt_password* with your MQTT broker password. The *"name"* parameter is the default sensor name that appears in your Home app when you add the sensors. You can use any name you like, but I prefer to use neutral names and rename the sensors in the Home app later. Also, take a look at the *"serialNumber"* parameter. I build it as *sensorID_colour_first_six_digits_from_address*. This allows me to identify the sensor in the Home app and give it the correct name depending on the sensor's location. And the *"topic"* parameter must end with the sensor's sequential number (0, 1, 2, 3, etc.). I hope you got it.  

Now save the configuration and restart HomeBridge. If you did everything correctly, then after HomeBridge restarts, you should see the sensors in your Home app. You do not need to add all of them manually, because they are connected to the HomeBridge bridge, so they will appear automatically.  

## 4. Flashing the Controller Firmware

Open the *GreenHouse.ino* sketch and flash it to the board. Once completed, connect all the sensors. **Do not forget about the GSM antenna! Running the board without a connected antenna may damage the modem.** Connect the 18650 battery. Connect external power (if needed) to the USB-C port. Do not turn the board on yet.  

### 4.1. Preparing SD-Card

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



