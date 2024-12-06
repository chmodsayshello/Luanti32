# Luanti32

Luanti Clients... ON AN ESP32!


# DISCLAIMER
This project is in a VERY early stage of development! It has been quickly thrown together so far, and some, if not most portions of the code are extremly ugly and need a revamp (i.e. I made a mistake while parsing the SRP_S_B bytes and hardcoded size s to temporaily make this work).
**The server can easily cause buffer overflows, as I did not put assertions in place yet.**

Also note that as of right now most of the test main file is WiFi code directly copied from Espressif (licensed under public domain)

## Build Instructions
 - Comming soon

## Credits

 - Espressif for providing the public domain wifi code mentioned above
 - est31 and Tom Cocagne for [implementing  a minimal](https://github.com/est31/csrp-gmp) version of SRP, a tiny adjustment had been made to make this run on an ESP-32 (it tries to use /dev/urandom for filling a buffer if not on Windows)
 - chfast for [their minimal implementation of GMP](https://github.com/chfast/mini-gmp) (dependency of the SRP-libary)
