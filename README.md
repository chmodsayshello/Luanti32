# Luanti32

Luanti Clients... ON AN ESP32!


# DISCLAIMER
This project is in a VERY early stage of development! Some, if not most portions of the code are extremly ugly and need a revamp (i.e. I made a mistake while parsing the SRP_S_B bytes and hardcoded size s to temporaily make this work).

## Build Instructions

 1. Install ESP-IDF on your system. Refer to [Espressif's manual](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/#installation).
 2. Open a shell in which you can execute `idf.py` in whatever directory you cloned this repository into
 3. run `idf.py build`
 4. (to flash, run `idf.py flash` afterwards)


## Usage

 - Connect your esp32 to your network
 - Make an instance of the `LuantiClient` struct and overwrite it completly with 0.
 - Setting the fields you want/need:
 - Run LuantiClient_connect
 - Run LuantiClient_tick as often as possible while your ESP32 is connected to the Luanti-Server

I recommend you look at [the example](https://github.com/chmodsayshello/Luanti32/blob/main/main/example/main.c) provided.

## Credits

 - Espressif for providing the public domain wifi code
 - est31 and Tom Cocagne for [implementing  a minimal](https://github.com/est31/csrp-gmp) version of SRP, a tiny adjustment had been made to make this run on an ESP-32 (it tries to use /dev/urandom for filling a buffer if not on Windows)
 - chfast for [their minimal implementation of GMP](https://github.com/chfast/mini-gmp) (dependency of the SRP-libary)
