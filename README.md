TOC
===
- [About](#about)
- [Build requirements](#build-requirements)
- [Build & Install](#build--install)
- [Playground](#playground)
  - [Crontab](#crontab)
  - [LCDProc configuration](#lcdproc-configuration)

## About

**backlight-toggle** is a tool to control backlight on HD44780 compatible LCDs,  
connected through the PCF8574 to the embedded system.  

Usually, it's enough just to do something like:  
```
# echo "1" > /sys/class/backlight/front_lcd/brightness
```

to turn it on, or  
```
# echo "0" > /sys/class/backlight/front_lcd/brightness
```

to turn it off.

**N.B.:** the path to the sysfs backlight control file might differ, but the concept  
remains the same practically for all LCD devices with some minor changes...  

In case of I2C-controlled device however it's not that easy, especially if  
the system doesn't have the path mentioned above.  

**backlight-toggle** allows to solve this little task by accessing the device  
over I2C bus the following way:  
```
# backlight-toggle -d /dev/i2c-1 -a 0x27 -s 0
```

to turn the backlight off, or  
```
# backlight-toggle -d /dev/i2c-1 -a 0x27 -s 1
```

to turn the backlight on, considering:  
- -d \<path\> :path to i2c control file in devfs
- -a \<address\> :device address on the bus
- -s \<state\> : 0 - off, 1 - on.

## Build requirements

The easiest way to build the project on Linux machine is to do the following:  
```
# apt install --no-install-recommends -y build-essential cmake libi2c0 libi2c-dev
```

Also, **i2c-dev** module must be present on the system and loaded at boot time:  
```
# modprobe i2c-dev
# echo "i2c-dev" >> /etc/modules
```

Active module, however, will be available as **i2c_dev**:  
```
# lsmod | grep i2c_dev
i2c_dev                20480  2
```

As a result, I2C device will be available in the devfs:  
```
# ls -1al /dev | grep i2c
crw-rw----  1 root i2c      89,   1 Jul 13 20:29 i2c-1
```

## Build & Install

Assuming the source is downloaded and one is in the source folder. Consider doing  
the following:  
```
user@vm:~/src/backlight_toggle$ mkdir build && cd build
user@vm:~/src/backlight_toggle$ cmake -DCMAKE_BUILD_TYPE=Release ..
user@vm:~/src/backlight_toggle$ make
user@vm:~/src/backlight_toggle$ sudo chown root:root backlight-toggle
user@vm:~/src/backlight_toggle$ sudo mv backlight-toggle /usr/bin 
```

## Playground

For this part one needs to have **lcdproc** installed. (honestly, *lcdproc* **is** the reason why it's started).  
Since I couldn't find the way how to turn the backlight on/off in *lcdproc* when I need it (I'd say - using  
some schedule), I configured **crontab** and created a service for my purposes.  

#### Crontab
If it's not created, let's do it (**cron** package should be installed beforehand).  
```
# crontab -e
# m h  dom mon dow   command
0    8 * * * /usr/bin/systemctl start lcdproc && /usr/bin/backlight-toggle -d /dev/i2c-1 -a 0x27 -s 1
0   22 * * * /usr/bin/systemctl stop lcdproc && /usr/bin/backlight-toggle -d /dev/i2c-1 -a 0x27 -s 0
```

#### LCDProc configuration

Now here is the fun. I'm using already built I2C bus expander board with pre-populated  
PCF8574 & HD44780 compatible 2x16 LCD, so **lcdproc** doesn't work with it with the default  
configuration out of the box, so I had to modify the configuration of the server first.  
```
# cat /etc/LCDd.conf
## This file was written by cme command.
## You can run 'cme edit lcdproc' to modify this file.
## You may also modify the content of this file with your favorite editor.


[server]
DriverPath = /usr/lib/aarch64-linux-gnu/lcdproc/
Driver = hd44780
WaitTime = 5
NextScreenKey = Right
PrevScreenKey = Left
ReportToSyslog = yes
ToggleRotateKey = Enter

[menu]
DownKey = Down
EnterKey = Enter
MenuKey = Escape
UpKey = Up

[hd44780]
ConnectionType = i2c
Device = /dev/i2c-1
Port = 0x27
Backlight = yes
BacklightInvert = yes
Size = 16x2
DelayBus = false
DelayMult = 1
Keypad = no
i2c_line_RS=0x01
i2c_line_RW=0x02
i2c_line_EN=0x04
i2c_line_BL=0x08
i2c_line_D4=0x10
i2c_line_D5=0x20
i2c_line_D6=0x40
i2c_line_D7=0x80
```

The configuration is the one which is recommended in the official **lcdproc** documentation,  
so nothing special here, just a note.

**N.B.:** don't try to turn the backlight on/off while **lcdproc** service is active - it  
simply won't allow you to do so (the backlight, however, might be turned off for a second,  
but the service will take the control over it and will turn the backlight on again).

There is no sence in giving lcdproc's client configuration, while having the service file for  
the client would be really nice... And here is the reason why.  
Considering the [crontab](#crontab) configuration, *LCDProc* service will be stopped at the  
time, mentioned in *crontab's* config file, which means that *lcdproc* client will automatically  
disconnect from the server. We need to link them together so the client **will** start as  
soon as the server will be activated and stop, when the server will be stopped.  
My suggestion to reach this goal will be as follows:  
```
# cat << EOF >> /lib/systemd/system/lcdproc-client.service
[Unit]
Description=LCDProc client
After=lcdproc.service
BindsTo=lcdproc.service

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/usr/bin/lcdproc

[Install]
WantedBy=multi-user.target
RequiredBy=lcdproc.service
Alias=lcdproc-client
EOF
# systemctl enable lcdproc-client
# systemctl start lcdproc-client
```

Assuming that */etc/lcdproc.conf* is pre-configured, all mentioned above will be enough to activate  
a connection to the *LCDProc* server and update the LCD screen accordingly.  
