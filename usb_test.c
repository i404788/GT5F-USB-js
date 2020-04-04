#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stddef.h>
#include "usb_protoc.h"

int main(int argc, char const *argv[])
{
  int fd = gethandle("/dev/sr0");
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
  int status = Run_Command(fd, 0x01, 0x00);
  if (status < 0){
    printf("Failed to send message 1: %d\n", status);
  }
  status = Run_Command(fd, 0x12, 0x00);
  if (status < 0){
    printf("Failed to send message 2: %d\n", status);
  }
  

  return 0;
}
