![Pasted Graphic 1](https://github.com/jetpax/RetroVMS-MINI/blob/main/images/retroVMS-logo.png?raw=true)

## Why RetroVMS?

RetroVMS aims to bring Connected Vehicle capabilities to the EV conversion community, enabling leading-OEM style Mobile App control and monitoring.

One of the biggest challenges of the EV conversion process lies in establishing seamless connectivity between the old and the new components, which is typically overcome with one or more Vehicle Control Units (VCU).  

RetroVMS is a Vehicle Management System (VMS), which provides not only these VCU functions, but also serves as a secure gateway providing mobile app integration over a 4G cellular data network.  


## Who is RetroVMS for?

![IMG_6830](https://github.com/jetpax/RetroVMS-MINI/blob/main/images/teslarr.png?raw=true)

RetroVMS was created primarily for EV conversion specialists and Restomod enthusiasts.  But it's also the ideal platform for owners of both OEM EVs and traditional ICE vehicles who seek features often overlooked by manufacturers. For example, RetroVMS offers the convenience of live vehicle location updates directly on your smartphone.


## What is RetroVMS?
![retrovmsV3](https://github.com/jetpax/RetroVMS-MINI/blob/main/images/retroVMS-MINI.png?raw=true)

The RetroVMS module has 5 main functions:

- Interfaces EV components such as heaters, chargers and inverters to legacy gauges and controls such as speedo or PNDR selector
- Monitors and Controls vehicle processes, such as charging, thermal management, parking brake, unlocking and more. 
- Reports vehicle metrics like state of charge,  location, temperatures,  tyre pressures via mobile app.
- Alerts on critical conditions and events, e.g. charge abort or completion,  battery cell failure or potential theft.
- Stores vehicle preferences, eg regen sensitivity,  charge settings,  ambient lighting etc

The firmware is compatible with OVMS,  which already has support for many existing EVs, and adding new vehicles is well documented and straightforward. 


## Example Application

![Application Example](https://github.com/jetpax/RetroVMS-MINI/blob/main/images/retroVMS-app-example.png?raw=true)

## Features

* Secure remote vehicle access over 4G networks
    * Control and monitor all vital vehicle functions remotely
    * Provide live vehicle charge status to route planning apps eg ABRP
    * Global Radio standards compatibility
    * Secure remote debug access and event logging
* Legacy vehicle and EV system integration including
    * Dashboard interfacing
    * Remote locking 
    * Charge monitoring and control
    * Smart LED Ambient Lighting
    * HVAC and thermal management
    * Tyre Pressure Monitoring
* Vehicle Dynamics Capture (GNSS & 3D Accelerometer)
    * Geolocation
    * Geofencing
    * Vehicle Movement (aka Flatbed) alerting
    * Traction control
* Flexible, Rugged Hardware
    * Run Time hardware I/O configuration (eg HS, LS, PWM)
    * Thermal/Overload/short/open circuit detection
    * Reverse Polarity and Load dump protection
    * Modular wireless and processor for supply-chain resilience and future proofing 
    * Switch whetting
    * Low Quiescent current operation
    * Sealed enclosure
* Vehicle Reverse Engineering support
* Bluetooth Driver Information Panel support


### Non-features!
- To provide BMS and Charge/Contactor control services
    - This safety critical function is inseparably tied to the HV battery pack and should be performed inside the pack envelope. 
- Drive High Current loads (>1A) directly
    - This should be done via relay or using a CAN based Power Distribution Unit ( cf VCFront)
- Perform ICE ECU functions


## RetroVMS Module firmware

RetroVMS module software, has 4 components using the ESP-IDF framework running on top of FreeRTOS.

- esp32m/core, providing system services, communications protocols and hardware system access
- OVMS compatible vehicle monitoring and control 
- ReactJS + MUI web configuration app (served by RetroVMS but running on web client)
  
![ebUI](https://github.com/jetpax/RetroVMS-MINI/blob/main/images/retroVMS-webUI.png?raw=true)


#### An OVMS compatible console is available over websockets
![console](https://github.com/jetpax/RetroVMS-MINI/blob/main/images/retroVMS-console.png?raw=true)
