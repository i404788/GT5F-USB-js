# GT5F-js
Nodejs (+ Header Only C) library for the GT-521F52, GT-521F32 and GT-511C3(R) fingerprint scanners

It uses the [Linux SCSI Driver](http://sg.danny.cz/sg/index.html) so no OSX support.
It probably also doesn't work on Windows (MinGW), but it could be ported.

It can be installed with `yarn add gt-usb` or `npm install gt-usb`.

## Features
* Support for the [entire GT-521F52 API](https://cdn.sparkfun.com/assets/learn_tutorials/7/2/3/GT-521F52_Programming_guide_V10_20161001.pdf)
* SCSI-over-USB; so no adapter required.
* Node.js & C/C++ support
* Flexible interface so all SCSI GT-5* fingerprint readers should work.

## Usage/Example
```js
const { CMDCodes, RunCommand, Initdevice, ReceiveData, Initialize } = require('gt-usb')
(async () => {
  let deviceInfo = await Initialize('/dev/sr0')
  console.log(deviceInfo)
  // Turn on/off CMOS LED
  let status = await RunCommand(CMDCodes['CmosLed'], 0x00)
  console.log(status)
})()
```

## Header Only Lib
The header only lib is `usb_protoc.h` it will handle the SCSI commands and CRC for you, you only need to provide a file desciptor and the commands/data.

See `usb_test.c` and `usb_module.cc` for examples.
