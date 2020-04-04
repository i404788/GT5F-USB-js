declare module 'gt-usb' {
  let native: {
    /**
     * Initializes usb device
     * @function
     * @param {string} [dev_path] Path to device (e.g. /dev/sr0)
     * @param {Function} [callback] Callback (handle: number) => any
     */
    initdevice: (dev_path: string, callback: (handle: number) => void) => void,

    /**
     * Receives data from device
     * @function
     * @param {number} [len] Amout of bytes to read
     * @param {Function} [callback] Callback (status: number, data: Buffer) => any
     */
    receiverawdata: (len: number, callback: (status: number, data: Buffer) => void) => void,

    /**
     * Sends data to device
     * @function
     * @param {Buffer} [data] Data to send
     * @param {boolean} [isCMD] Determines if the data is formatted as a command
     * @param {Function} [callback] Callback (status: number) => any
     */
    sendrawdata: (data: Buffer, isCMD: boolean, callback: (status: number) => void) => void,

    /**
     * Run command
     * @function
     * @param {number} [command] Command ID to use
     * @param {number} [parameter] Parameter to send with command
     * @param {Function} [callback] Callback (status: number) => any
     */
    RunCommand: (command: number, parameter: number, callback: (status: number) => void) => void
  }

  let RunCommand: (command: number, parameter: number) => Promise<[number]>
  let Initdevice: (DevicePath: string) => Promise<[number]>
  let ReceiveData: (Bytes: number) => Promise<[number, Buffer]>
  /**
   * Initializes device and communication
   * @function
   * @param {string} [DevicePath] /dev device to open (default /dev/sr0)
   */
  let Initialize: (DevicePath: string) => Promise<string>

  const CMDCodes: {[code: string]: number}
}
