import { startUi, CaptivePortal, Wifi, System, Mqtt, Sntp, NetworkInterfaces, Buzzer, DebugTasks, I2CScanner, DebugPins } from '@esp32m/ui';

startUi({ plugins: [DebugPins(), CaptivePortal, System, Wifi, Mqtt, Sntp, NetworkInterfaces, Buzzer(), DebugTasks, I2CScanner] });

