#pragma pack(1)
#include <stdio.h>
#include <stdlib.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <cstring>
#include <errno.h>

// Header Of Cmd and Ack Packets
#define STX1 0x55 //Header1
#define STX2 0xAA //Header2

// Header Of Data Packet
#define STX3 0x5A //Header1
#define STX4 0xA5 //Header2

#define HEADLEN 2
#define DEVIDLEN 2
#define CRCLEN 2

typedef struct _COMMAND_PACKET
{
  unsigned int PARAM;
  unsigned short COMMAND;
} COMMAND_PACKET;

unsigned short LCMDAck;
int LParamAck;

unsigned short GetCRCRaw(unsigned char *packet, size_t len)
{
  unsigned short CRC = 0;

  for (size_t i = 0; i < len; i++)
  {
    CRC += packet[i];
  }
  return CRC;
}

void destroy_io_hdr(struct sg_io_hdr *p_hdr)
{
  if (p_hdr)
  {
    free(p_hdr);
  }
}

void configure_io(struct sg_io_hdr *hdr, unsigned char *packet, size_t packet_len, int bRead)
{
  memset(hdr, 0, sizeof(struct sg_io_hdr));
  hdr->interface_id = 'S';
  hdr->dxfer_direction = bRead ? SG_DXFER_FROM_DEV : SG_DXFER_TO_DEV;
  hdr->dxfer_len = packet_len;
  hdr->dxferp = packet;
  hdr->cmd_len = 10;   // GENERIC_CMD_LENGTH
  hdr->timeout = 5000; // 5s
  unsigned char *cmd = (unsigned char *)malloc(10);
  cmd[0] = 0xEF;
  cmd[1] = bRead ? 0xFF : 0xFE;
  hdr->cmdp = cmd;
}

int gethandle(const char *device)
{
  int fd = open(device, O_RDONLY);
  if (fd < 0)
  {
    printf("%s: Open error: %s, errno=%d (%s)\n", __func__, device, errno, strerror(errno));
    exit(1);
  }
  return fd;
}

int Receive_Data(int fd, unsigned char *packet, int nSize)
{
  if (packet == NULL)
    return -4;

  unsigned char packetdata[nSize + 6];
  memset(packetdata, 0, nSize + 6);
  struct sg_io_hdr rawret;
  configure_io(&rawret, packetdata, nSize + 6, 1);
  if (ioctl(fd, SG_IO, &rawret) < 0)
  {
    printf("%s: ioctl error: errno=%d (%s)\n", __func__, errno, strerror(errno));
  }

  unsigned char *data = (unsigned char *)rawret.dxferp;

  if (!((data[0] == STX1 && data[1] == STX2) || (data[0] == STX3 && data[1] == STX4)))
  {
    printf("%s: return data head invalid %02x %02x\n", __func__, data[0], data[1]);
    return -1;
  }

  unsigned short *DevID = (unsigned short *)&data[2];
  if (*DevID != 1)
  {
    printf("%s: Device ID got corrupted %02x %02x\n", __func__, data[0], data[1]);
    return -3;
  }

  unsigned short *CRC = (unsigned short *)&data[nSize + 4];
  if (*CRC != GetCRCRaw(data, nSize + 4))
  {
    printf("%s: CRC doesn't match %02x %02x\n", __func__, *CRC, GetCRCRaw(data, nSize + 4));
    return -2;
  }
  memcpy(packet, &data[4], nSize);
  return 0;
}

int Send_Data(int fd, unsigned char *data, size_t len, int isCMD)
{
  unsigned char packet[len + 6];
  packet[0] = isCMD ? STX1 : STX3;       // HEAD 1
  packet[1] = isCMD ? STX2 : STX4;       // HEAD 2
  *((unsigned short *)(&packet[2])) = 1; // Dev ID
  memcpy(&packet[HEADLEN + DEVIDLEN], data, len);
  *((unsigned short *)(&packet[len + HEADLEN + DEVIDLEN])) = GetCRCRaw(packet, len + HEADLEN + DEVIDLEN);

  struct sg_io_hdr rawret;
  configure_io(&rawret, packet, len + 6, 0);
  if (ioctl(fd, SG_IO, &rawret) < 0)
  {
    printf("%s: ioctl error: errno=%d (%s)\n", __func__, errno, strerror(errno));
    return -1;
  }
  return 0;
}

int Receive_Command(int fd, unsigned short *LastCMDAck, int *LastParamAck)
{
  unsigned char dxferBuffer[100];
  int stat = Receive_Data(fd, dxferBuffer, sizeof(COMMAND_PACKET));
  if (stat < 0)
  {
    printf("%s: Failed to receive command\n", __func__);
  }
  COMMAND_PACKET *ret = (COMMAND_PACKET *)&dxferBuffer;

  *LastCMDAck = ret->COMMAND;
  *LastParamAck = ret->PARAM;
  return stat;
}

int Send_Command(int fd, COMMAND_PACKET packet)
{
  int stat = Send_Data(fd, (unsigned char *)&packet, sizeof(COMMAND_PACKET), 1);
  if (stat < 0)
  {
    printf("%s: ioctl error: errno=%d (%s)\n", __func__, errno, strerror(errno));
  }
  return stat;
}

int Run_Command(int fd, unsigned short COMMAND, int param)
{
  COMMAND_PACKET cmd;
  cmd.COMMAND = COMMAND;
  cmd.PARAM = param;
  int stat = Send_Command(fd, cmd);
  if (stat != 0)
    return stat;

  stat = Receive_Command(fd, &LCMDAck, &LParamAck);
  if (stat != 0)
    return stat;

  return 0;
}
