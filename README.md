# HIDman fork
## USB HID to and Serial HID to PS/2

HIDman is an open source device to allow the use of modern USB keyboards and mice on legacy PCs. Its an amazing project and Kudos to all the forks before me. This has been modified for my personal needs but I thought I'd share incase it was useful.

Peripherals that support legacy PCs are becoming hard to find, especially for the very first PCs. This project aims to provide a simple cheap solution that will cover everything from the original IBM 5150 PC all the way up to modern computers that have PS/2 ports.

<img src=/images/hidmanfront.jpg width=50%/>
<img src=/images/hidmanrear.jpg width=50%/>


# Features

 I have disabled the button keyboard select functionality as I only want and need PS/2
 
* There is no mouse support. I have changed the DB9 mouse serial in to work as a "virtual serial USB device". It is expecting USBHID packets to be sent over serial and they will be treated the same as a USBHID keyboard.
I am using nodered on a Pi to send buffered payloads as per USBHID standard. This enables a device, in my case a CM4 Pi HMI and a programable USBHID keypad, to output keystrokes to a PS/2 enabled device. All the parsing functions are as before, the HIDman just recieves data from a serial input buffer just as it would from USB. If you need the other keyboards and or mouse then use the original files

Build, manufacturing credit and money should go to rasteri. I used PCB way and it was great, tarrifs not so much. If I was making 100 I would update the circuit to have an additonal button to load firmware the first time, tweezers work but you must short the connection the first time.

* Driverless configuration menu - change settings just by opening a text editor

# How to get one

## Purchase : 

* He has some for sale on his tindie page - https://www.tindie.com/products/rasteri/hidman/
* If you're in the US, retro hack shack also has stock - https://retrohackshack.com/product/hidman-usb-keyboard-converter/

## DIY : 

* Grab an assembled PCB from PCBway : https://www.pcbway.com/project/shareproject/HIDman_USB_to_Retro_PC_Adapter_3b4fb5d5.html
* Also grab the front plate : https://www.pcbway.com/project/shareproject/Front_enclosure_plate_for_HIDman_0aec2ae9.html
* And the rear plate : https://www.pcbway.com/project/shareproject/Rear_Enclosure_for_HIDman_c0ff0b8a.html
* And the enclosure : https://lincolnbinns.com/e-case-a-black-40mm.html
* Don't forget 8x screws : https://lincolnbinns.com/e-case-screws.html (I suggest the thread former ones)
* After assembling the unit, you will need to install firmware. See the "Firmware Update" section below.

# Quickstart Guide

## USB Connection

You can connect many different combinations of USB devices to HIDman.

The most obvious being to connect a keyboard to one USB port, and a mouse to the other one :

<img src=/images/usbsetup1.svg width=300/>

Or, you could perhaps connect a wireless keyboard+mouse dongle to one port, and a game controller to the other :

<img src=/images/usbsetup2.svg width=500/>

Hub support can be hit-and-miss. This is (mostly) not HIDman's fault - many modern hubs don't support low-speed USB devices properly.


## Newer PCs (PS/2) ### its all relative

* Connect male-to-male Mini-DIN cables from HIDman’s keyboard to your PC’s PS/2 ports.
* Turn on your PC.


<img src=/images/newerpcs.svg width=400/>


# Configuration is still there but I have not updated or checked it.

Configuration is provided through a built-in menu system. No software install is required - all interaction is through the keyboard.

Simply open a text editor on your target PC (eg notepad, edit, vi), then hold HIDman's power button for a few seconds. The menu will be typed out into your text editor.

<img src=/images/menu1.png width=394/>

"Adv.Keyboard" and "Adv.Mouse" switch the keyboard and mouse respectively into "Native Mode". This may be required to access certain functions such as scroll wheels and extra buttons on mice.

**Note : Some USB devices with both keyboard and mouse interfaces (e.g. mice with configurable buttons) require both Adv.Keyboard and Adv.Mouse to be enabled otherwise they behave unpredictably.**

"Intellimouse" enables scroll wheel and 5-button support for PS/2 mice.

"Debug" enables the debug menu. The most useful debugging option is "Dump HID Data". Enabling this dumps information about all USB devices connected after that point. 

<img src=/images/menu2.png width=450/>

If you are having trouble getting a USB device to work, enable HID Dumping then connect the troublesome USB device. Wait for it to finish dumping, then press ESC and save the resulting data to a text file. Then send it to me for analysis.


# Firmware Update


If you're on windows, first install WCH's ISP tool - https://www.wch.cn/downloads/WCHISPTool_Setup_exe.html
If on Mac or Linux, install ch55xtool : https://github.com/MarsTechHAN/ch552tool

The next step is to put the HIDman in firmware update mode.

1. Disconnect everything from HIDman, including all USB devices and PCs. (failure to do this may result in damage to HIDman, your PC, or both).
2. For first time installation follow the instructions in the "Debricking" section below. else
3. Hold down HIDman's ⏻ power button.
4. Use a USB A-to-A cable to connect HIDman's LOWER USB port to a USB port on your modern PC. Available online or I made one from 2 old cables
5. I cannot get hex files to work but bin files work just fine. Compiled in codespace.

If HIDman doesn't appear in your computers' device list, follow the instructions in the "Debricking" section below.

After that, update instructions will depend on your operating system.

## Windows

Follow the instructions in this diagram :
Set the Chip first or it will not find the device

<img src=/images/firmware2.svg width=800/>


## Linux or Mac

Pass the -f parameter to ch55xtool specify the firmware file to load. For example :

```
python3 ch55xtool.py -f hidman_axp_v1.1.bin
```

## Debricking

If this is your first time programming HIDman, or you accidentally program a corrupted firmware, you can force it into firmware update mode another way. It can be a bit fiddly though.

<img src=/images/C5.png width=500/>

1. Disassemble HIDman.
2. Connect the two pads of component C5 (shown above) together using a pair of tweezers or similar.
3. While keeping the pads connected, use a USB A-to-A cable to connect HIDman's LOWER USB port to a USB port on your modern PC.

# Advanced Setups

## Combined PS/2 port
HIDman actually outputs keyboard *and* mouse signals on **BOTH** PS/2 connectors. If you have a PC that can take advantage of this (typically Mini PCs or Laptops), you can keep the cabling neater by using only one cable. **NOTE : You will need to enable "PS2 AUX Output" in the advanced menu.**

<img src=/images/combined1.svg width=500/>

Note that there are two different types of Combined PS/2 ports, and HIDman provides for both types. Generally, laptops will need plugged into the Mouse port on HIDman, and Mini PCs want the Keyboard port. If it doesn't work with one port, try the other one.

## Technical description

The HIDman is based around the CH559 from WCH, a remarkably flexible chip with **two** USB HOST ports. This makes it ideal for our purposes.

The code is forked from atc1441's excellent repository - https://github.com/atc1441/CH559sdccUSBHost

PCB and enclosure was designed in KiCad - source files are in the hardware directory.
