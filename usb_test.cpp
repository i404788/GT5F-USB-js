#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stddef.h>
#include "usb_protoc.hpp"

#pragma pack(push, 1)
typedef struct _devinfo
{
  uint32_t FirmwareVersion;
  uint32_t IsoAreaMaxSize;
  uint8_t DeviceSerialNumber[16];
} devinfo;

typedef _generic_packet<devinfo> DeviceInfoPacket;
#pragma pack(pop)

int main(int argc, char const *argv[])
{
  int fd = gethandle("/dev/sr1");
  /*
  // CMD: Init
  COMMAND_PACKET cmd = new_cmd();
  cmd.COMMAND = 0x01;
  cmd.PARAM = 0x00;
  SetCRC(&cmd);

  // SERIALIZE: Init
  unsigned char cIOBuffer[100];
  struct sg_io_hdr rawcmd;
  memset(&rawcmd, 0, sizeof(struct sg_io_hdr));
  configure_io(&rawcmd, (unsigned char*)&cmd, sizeof(COMMAND_PACKET), 0);
  rawcmd.mx_sb_len = sizeof(cIOBuffer);
  rawcmd.sbp = cIOBuffer;

  // SEND: Init
  if (ioctl(fd, SG_IO, &rawcmd) < 0)
  {
    printf("ioctl error: errno=%d (%s)\n", errno, strerror(errno));
  }
  printf("sense: %s ; dxfer: %s ; stat: %d\n", cIOBuffer, "<OUTGOING>", rawcmd.status);
  */
  CmdPacket result;
  int status = runCommand(fd, 0x01, 0x01, result);
  if (status < 0)
  {
    printf("Failed to send message 1: %d\n", status);
  }
  
  DeviceInfoPacket devInfo;
  status = recvPacket(fd, devInfo);
  if (status < 0)
  {
    printf("Failed to receive device info: %d; Maybe parameter = 0?\n", status);
  } else {
    printf("Firmware Version: 0x%02x, IsoMaxArea: %02x, Serial: %.16s\n", devInfo.data.FirmwareVersion, devInfo.data.IsoAreaMaxSize, devInfo.data.DeviceSerialNumber);
  }

  status = runCommand(fd, 0x12, 0x00, result);
  if (status < 0)
  {
    printf("Failed to send message 2: %d\n", status);
  }

  status = runCommand(fd, 0x02, 0x00, result);
  if (status < 0)
  {
    printf("Failed to close device: %d\n", status);
  }

  close(fd);
  return 0;
}
