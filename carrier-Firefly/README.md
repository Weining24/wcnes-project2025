# Pico-Backscatter: carrier-Firefly
Guide to configure Firefly to generate a unmodulated carrier in the range of 2394MHz to 2507MHz.

You will control the board via serial port.

### Install a compatible driver (if needed: e.g. Windows)
The device is compatible with the following driver: [CP210x USB to UART Bridge VCP Drivers](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=overview)

## Access Serial Port
<br> Install the python pyserial library:
```
 pip install pyserial
```

<br> Therea are conflicting names for the serial libary. You should have a python3 virtual environment:
```
python3 -m venv .venv
source ./venv/bin/activate
```


<br> Find the connected device port under /dev. 

Firefly should be connected to a usb port. The port name that appears
in your OS is OS- and machine-specific, but
most likely starts with "tty.usb". One way is to 
disconnect and reconnect Firefly, and see the new name that appears.
```
ls /dev/
```
<br> Access the device via serial port:
```
python3 -m serial.tools.miniterm -e [your-port] 115200
```

## Configuration Option
After connecting with pyserial, it is possible that what you type in the 
console are invisible. They however should work after you press enter.

If the board does not respond to commands below, press reset button on Firefly
(you should see "hello world"). 

Configure the frequency
```
 f {frequency}
```
Start the carrier generation
```
 a
```
Reset the Firefly with reset button or the following command
```
 r
```

Example: set the carrier to 2450MHz
```
 f 2450
 a
```

## Reference
[Zolertia Firefly Datasheet](https://github.com/Zolertia/Resources/blob/master/Firefly/Hardware/Revision%20A/Datasheets/ZOL-BO001-A2%20-%20Firefly%20revision%20A%20Datasheet%20v.1.0.0.pdf)


python3 -m serial.tools.miniterm -e /dev/tty.usbserial-2110 115200