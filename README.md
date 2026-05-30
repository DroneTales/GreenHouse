# Greenhouse Contoller for Apple Home

In this repository you will find a firmware, a schematic, a Python data logger and a WEB-application that allows you to build your own Apple Home compatible greenhouse controller. Should you have any questions, please do not hesitate to contact me at gully.horror0w@icloud.com.  

**Required components**

- LilyGO T-A7608E-H or T-A7608SA-H board - 1 pcs.
- DS18B20 temperature sensor - 1-10 pcs.
- Transistor 2N3906 - 1 pcs.
- Transistor 2N3904 - 1 pcs.
- Resistor 100 Ohm - 1 pcs.
- Resistor 1K - 2pcs.
- Resistor 4.7K - 1 pcs.
- Resistor 47K - 1 pcs
- Capacitor 3000mF x 6.3V - 1 pcs.

**Required Arduino librarues**

- esp32 by Espressif Systems (board) 3.3.8
- esp32-ds18b20 2.0.3
- TinyGsmClient 0.12.0
- PubSubClient 2.8
 
**Arduino IDE settings**

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

## 1. Preparing home network

I assume that you already have a **HomeBridge** setup and running on your local network. You will also need an MQTT broker running locally. Follow [these](https://github.com/DroneTales/VideoDoorbell) instructions to set up the Mosquitto MQTT broker. I also assume that you are familiar with your home modem or router and are able to configure it.  

### 1.1. Checking Your IP Address Type

The very first thing we need to do is check what your external IP address type is. There are three common IP types: static white, dynamic white, and gray. The term **white** means that the IP address assigned to your modem or router is the same as the one seen from the Internet. The term **gray** means that the IP address assigned to your modem or router is different from the one seen from the Internet (because your device is behind a NAT). I believe that you know if you have a static white IP address, because to have one you need an agreement with your ISP. So you can simply skip the following parts regarding dynamic white and gray addresses.  

So now let's check your IP address type. Open your modem (or router; further on, I will refer to this network device simply as the router) web interface or connect to it via SSH and check what IP address has been assigned by your ISP. Usually this information can be found on a Status page or something similar. If you connect to your router via SSH, you can use one of the following commands:  

`cat /tmp/dhcp.leases`  
`ip addr show or ifconfig`  
`ip neigh`  
`arp -an`  

The correct command depends on your router's firmware. Once you have found the assigned IP, open [this](https://www.myip.com) page to see how your IP is seen from the Internet.  

If the address on the router is identical to the one shown on the MyIP page, then you have a white external IP. That is great. If the address shown on the MyIP page is different from the one that is assigned to your modem, then you have a gray IP address.  

Now, if you have a white external IP address and are not sure if it is static or dynamic, then do the following:  

- Remember the current IP address.
- Turn your router **off**.
- Wait few minutes.
- Turn the router **on**.
- Check the IP address again.

You may need to turn your router off and on a couple of times to force the address to change. If the IP address is still the same, then you have a static one. If it has changed, then you have a dynamic external IP address that can change from time to time.  

#### 1.1.1. Static White External IP

If you have a static white external IP address, then you do not need to do anything special at this stage.  

#### 1.1.2. Dynamic White External IP

In the case of a dynamic white external IP address, you will need to configure DynDNS on your router. Follow your router manufacturer's and chosen DynDNS service's instructions to configure DynDNS on your router.  

#### 1.1.3. Gray External IP

This is the worst case. You will need to configure a **tunnel** to be able to access your home network from the Internet. As we will use **Cloudflare** later as a DNS service, I recommend using the Cloudflare tunnel. The instructions on how to set it up can be found at [this](https://developers.cloudflare.com/tunnel/) link. However, before configuring the tunnel, read the following parts about the domain name.  

### 1.2. Obtaining a Domain Name

If you already own a domain name, you can skip this part. However, I recommend transferring your domain to Cloudflare because we will use it as a DNS proxy to protect our local home network from the external world.  

If you do not own a domain name and you have a white static or gray external IP address, you will need to purchase one. There are a lot of services that offer domain names. I personally use Namecheap and Cloudflare. For this project, it is better if you purchase the domain name from Cloudflare, so you will not need to transfer it to Cloudflare later.  

If you have a white dynamic external IP address, then a domain name can be provided by the DynDNS service and you do not need to purchase one. However, check your DynDNS service's rules.  

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

+---------+--------------------------------------+----------+------------------------------------------+  
|         |                 Local                |          |                   Remote                 |  
| Comment +--------------------------------------| Protocol +------------------------------------------+  
|         |  IP address  | Start port | End port |          | IP address   | Start  port |   End port  |  
+---------+--------------+------------+----------+----------+--------------+-------------+-------------+  
|  MQTT   | <homebridge> |   1883     |   1883   |   TCP    |              | <mqtt_port> | <mqtt_port> |  
+---------+--------------+------------+----------+----------+--------------+-------------+-------------+  
|  HTTP   | <homebridge> |    80      |    80    |   TCP    |              |      80     |      80     |  
+---------+--------------+------------+----------+----------+--------------+-------------+-------------+  
|  HTTPS  | <homebridge> |    443     |    443   |   TCP    |              |     443     |     443     |  
+---------+--------------+------------+----------+----------+--------------+-------------+-------------+  

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

The project includes two applications: the **Temperature Logger** service and the **Temperature Monitor** web application. The applications are written in Python and JavaScript. They require **Python 3.11** and the **nginx** web server. In this section, you will find detailed instructions on how to set it all up.  

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
  
**BTC**: bitcoin:1A1WM3CJzdyEB1P9SzTbkzx38duJD6kau  
**BCH**: bitcoincash:qre7s8cnkwx24xpzvvfmqzx6ex0ysmq5vuah42q6yz  
**ETH**: 0xf780b3B7DbE2FC74b5F156cBBE51F67eDeAd8F9a  



