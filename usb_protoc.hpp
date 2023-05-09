#pragma pack(push, 1)
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <scsi/sg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

// TODO: reverse bytes for BE archs
#define CmdHeader 0xAA55  // 0x55AA
#define DataHeader 0xA55A // 0x5AA5

struct cmd_data {
  uint32_t param_data; // param | resp data
  uint16_t cmd_ack;    // cmd code  | ACK/NACK
};

template <typename data_type> struct _generic_packet {
  uint16_t header;
  uint16_t device_id;
  data_type data;
  uint16_t crc;
};

typedef _generic_packet<cmd_data> CmdPacket;

template <uint32_t bufSize>
struct _generic_packet<uint8_t[bufSize]> data_packet;

#pragma pack(pop)

uint16_t calc_crc(uint8_t *packet, size_t len) {
  uint16_t CRC = 0;
  for (size_t i = 0; i < len; i++)
    CRC += packet[i];
  return CRC;
}

void free_io_hdr(struct sg_io_hdr *p_hdr) {
  if (p_hdr != nullptr) {
    if (p_hdr->cmdp != nullptr) {
      free(p_hdr->cmdp);
    }
    free(p_hdr);
  }
}

void configure_io(struct sg_io_hdr *hdr, uint8_t *packet, size_t packet_len,
                  bool bRead) {
  memset(hdr, 0, sizeof(struct sg_io_hdr));
  hdr->interface_id = 'S';
  hdr->dxfer_direction = bRead ? SG_DXFER_FROM_DEV : SG_DXFER_TO_DEV;
  hdr->dxfer_len = packet_len;
  hdr->dxferp = packet;
  hdr->cmd_len = 10;   // GENERIC_CMD_LENGTH
  hdr->timeout = 5000; // 5s
  uint8_t *cmd = (uint8_t *)calloc(1, hdr->cmd_len);
  cmd[0] = 0xEF;
  cmd[1] = bRead ? 0xFF : 0xFE;
  hdr->cmdp = cmd;
}

int32_t gethandle(const char *device) {
  int32_t fd = open(device, O_RDONLY);
  if (fd < 0) {
    printf("%s: Open error: %s, errno=%d (%s)\n", __func__, device, errno,
           strerror(errno));
    exit(1);
  }
  return fd;
}

int32_t sendRawData(int32_t fd, const uint8_t *data, size_t len, bool isCMD) {
  // TODO: Use data_packet as base?
  uint8_t packet[len + 6] = {0, 0, 1, 0};
  *((uint16_t *)packet) = isCMD ? CmdHeader : DataHeader;
  memcpy(&packet[4], data, len);
  uint16_t crc = calc_crc(packet, len + 4);
  *((uint16_t *)&packet[len + 4]) = crc;

  struct sg_io_hdr io_hdr;
  configure_io(&io_hdr, packet, len + 6, false);
  int32_t stat;
  if ((stat = ioctl(fd, SG_IO, &io_hdr)) != 0) {
    printf("%s: ioctl error: errno=%d (%s)\n", __func__, errno,
           strerror(errno));
  }
  free(io_hdr.cmdp);
  return stat;
}

int32_t receiveRawData(int32_t fd, uint8_t *buf, size_t buf_size) {
  uint8_t packet[buf_size + 6] = {0};

  struct sg_io_hdr io_hdr;
  configure_io(&io_hdr, packet, buf_size + 6, true);
  int32_t stat;
  if ((stat = ioctl(fd, SG_IO, &io_hdr)) != 0) {
    printf("%s: ioctl error: errno=%d (%s)\n", __func__, errno,
           strerror(errno));
  }

  uint16_t packet_crc = *((uint16_t *)&packet[buf_size + 4]);
  if (packet_crc != calc_crc(packet, buf_size + 4)) {
    free(io_hdr.cmdp);
    printf("CRC not matched while receiving packet");
    return -1;
  }
  free(io_hdr.cmdp);
  return stat;
}

template <typename packet_type>
int32_t sendPacket(int32_t fd, packet_type &packet) {
  constexpr uint32_t crc_window = sizeof(packet_type) - sizeof(uint16_t);
  packet.crc = calc_crc((uint8_t *)&packet, crc_window);

  struct sg_io_hdr io_hdr;
  configure_io(&io_hdr, (uint8_t *)&packet, sizeof(packet_type), false);
  int32_t stat;
  if ((stat = ioctl(fd, SG_IO, &io_hdr)) != 0) {
    printf("%s: ioctl error: errno=%d (%s)\n", __func__, errno,
           strerror(errno));
  }
  free(io_hdr.cmdp);
  return stat;
}

template <typename packet_type>
int32_t recvPacket(int32_t fd, packet_type &packet) {
  struct sg_io_hdr io_hdr;
  configure_io(&io_hdr, (uint8_t *)&packet, sizeof(packet_type), true);
  int32_t stat;
  if ((stat = ioctl(fd, SG_IO, &io_hdr)) != 0) {
    printf("%s: ioctl error: errno=%d (%s)\n", __func__, errno,
           strerror(errno));
  }

  constexpr uint32_t crc_window = sizeof(packet_type) - sizeof(uint16_t);
  if (packet.crc != calc_crc((uint8_t *)&packet, crc_window)) {
    free(io_hdr.cmdp);
    printf("CRC not matched while receiving packet");
    return -1;
  }
  free(io_hdr.cmdp);
  return stat;
}

int32_t runCommand(int32_t fd, uint16_t cmd, uint32_t param,
                   CmdPacket &result) {
  CmdPacket cmdpacket = {CmdHeader, 0x1, {param, cmd}, 0};
  int32_t stat = sendPacket(fd, cmdpacket);
  if (stat != 0)
    return stat;

  return recvPacket(fd, result);
}
