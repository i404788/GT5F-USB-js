# GT5F-js
Nodejs (+ Header Only C) library for the GT-521F52, GT-521F32 and GT-511C3(R) fingerprint scanners

It uses the [Linux SCSI Driver](http://sg.danny.cz/sg/index.html) so no OSX support.
It probably also doesn't work on Windows (MinGW), but it could be ported.

It can be installed with `yarn add gt-usb` or `npm install gt-usb`.

## Header Only Lib
The header only lib is `usb_protoc.h` it will handle the SCSI commands and CRC for you, you only need to provide a file desciptor and the commands/data.

See `usb_test.c` and `usb_module.cc` for examples.
