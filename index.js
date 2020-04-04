"use strict";

const native = require("./build/Release/gt_usb.node");

/**
 * Initializes usb device
 * @function
 * @param {string} [dev_path] Path to device (e.g. /dev/sr0)
 * @param {Function} [callback] Callback (handle: number) => any
 */
native.initdevice;

/**
 * Receives data from device
 * @function
 * @param {number} [len] Amout of bytes to read
 * @param {Function} [callback] Callback (status: number, data: Buffer) => any
 */
native.receiverawdata;

/**
 * Sends data to device
 * @function
 * @param {Buffer} [data] Data to send
 * @param {boolean} [isCMD] Determines if the data is formatted as a command
 * @param {Function} [callback] Callback (status: number) => any
 */
native.sendrawdata;

/**
 * Run command
 * @function
 * @param {number} [command] Command ID to use
 * @param {number} [parameter] Parameter to send with command
 * @param {Function} [callback] Callback (status: number) => any
 */
native.RunCommand;

const CMDCodes = {
  Open: 0x01,
  Close: 0x02,
  UsbInternalCheck: 0x03,
  ChangeBaudrate: 0x04,
  CmosLed: 0x12,
  GetEnrollCount: 0x20,
  CheckEnrolled: 0x21,
  EnrollStart: 0x22,
  Enroll1: 0x23,
  Enroll2: 0x24,
  Enroll3: 0x25,
  IsPressFinger: 0x26,
  DeleteID: 0x40,
  DeleteAll: 0x41,
  Verify: 0x50,
  Identify: 0x51,
  VerifyTemplate: 0x52,
  IdentifyTemplate: 0x53,
  CaptureFinger: 0x60,
  MakeTemplate: 0x61,
  GetImage: 0x62,
  GetRawImage: 0x63,
  GetTemplate: 0x70,
  SetTemplate: 0x71,
  GetDatabaseStart: 0x72,
  GetDatabaseEnd: 0x73,
  SetSecurityLevel: 0xf0,
  GetSecurityLevel: 0xf1,
  Identify_Template2: 0xf4,
  EnterStandbyMode: 0xf9,
  Ack: 0x30,
  Nack: 0x31
};

/**
 * Assumes callback is (arguments..., callback: (arguments...) => void)
 * @param {Function} func Function to promisify
 * @param {any} arguments args for func
 */
function promisify (func, ...args) {
  return new Promise((resolve, reject) => {
    try {
      let cb = (...cbargs) => resolve(cbargs); // Add resolve as callback
      func.apply(this, args.concat(cb));
    } catch (e) {
      reject(e);
    }
  });
}

const ReceiveData = (Bytes) => {
  return promisify(native.receiverawdata, Bytes);
}
const RunCommand = (CmdCode, Param) => {
  return promisify(native.RunCommand, CmdCode, Param)
}

module.exports = {
  native,
  CMDCodes,

  Initialize: async (DevicePath = '/dev/sr0') => {
    let status
    [status] = [-1]
    status = await promisify(native.initdevice, DevicePath)
    if (status < 0) throw `Couldn't open ${DevicePath}; code=${status}`
    status = await RunCommand(CMDCodes['Open'], 0x01)
    if (status < 0) throw `Couldn't initialize device; code=${status}`
    let data = await ReceiveData(24) // DWORD(4), DWORD(4), Buffer(16)
    if (data[0] < 0) throw `Couldn't retrieve data from device; code=${data[0]}`
    return `Firmware version: ${data[1].readUInt32BE(0)}, IsoAreaMaxSize: ${data[1].readUInt32BE(4)}, SerialNumber: ${data[1].slice(8).toString('hex')}`
  },
  RunCommand,
  ReceiveData
};
