# PowerGeneration-Arduino
I am in the process of creating some boards to allow me to be a bit more off-grid-ish while camping or otherwise outdoors.  This repository contains the Arduino code required to run the various boards.  It is intended to be checked out at a top-level Arduino workspace.

# Repository layout

* **/docs** - Documentation
* **/libraries** - Arduino libraries used by the various sketches (mostly git submodules)
* **/batterycharger** - Arduino sketch for the [battery charger](https://easyeda.com/Beirdo/TEG_Controller-a188ea3f20ad454082dd60f157da3a8e) board
* **/mppt** - Arduino sketch for the [MPPT](https://easyeda.com/Beirdo/MPPT_Board-0f943a50d17e42ab88a2a93e983fdc09) ([Maximum Power Point Tracking](https://en.wikipedia.org/wiki/Maximum_power_point_tracking)) board
* **/nano-temperatures** - Arduino sketch for the [Nano Temperature Shield](https://easyeda.com/Beirdo/Nano_Temperature_Shield-ad89e13a45764eea99eeb719f00b5a79) board
* **/nano-gprs** - Arduino sketch for the [Nano GPRS Shield](https://easyeda.com/Beirdo/Nano_GPRS_Shield-026ff65dd22e48b7b38a45dc8aefa5b8) board
* **/consoletest** - Arduino sketch for testing the serial CLI code

# Licensing

All board designs and Arduino sketches are licensed under [Creative Commons Attribution-ShareAlike 3.0 License](https://creativecommons.org/licenses/by-sa/3.0/deed.en) also known as CC-BY-SA 3.0.

Arduino libraries linked here are under various open-source licenses.
