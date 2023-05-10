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

#define GT_TEMPLATE_SIZE 498

#define GT_CMD_WAKE 0x00
#define GT_CMD_INIT 0x01 // Params: device_info: true/false
#define GT_CMD_CLOSE 0x02
#define GT_CMD_USB_VALID 0x03
#define GT_CMD_LED 0x12
#define GT_CMD_ENROLL_COUNT 0x20
#define GT_CMD_CHECK_ENROLLED 0x21 // Param: fingerprint id
#define GT_CMD_ENROLL_START 0x22 // Param: fingerprint id, resp: NACK_DB_IS_FULL, NACK_INVALID_POS, NACK_IS_ALREADY_USED
#define GT_CMD_ENROLL_STEP1 0x23 // Resp: NACK_ENROLL_FAILED, NACK_BAD_FINGER
#define GT_CMD_ENROLL_STEP2 0x24 // ^
#define GT_CMD_ENROLL_STEP3 0x25 // Resp: ^ | fingerprint id; if id==-1 template data packet
#define GT_CMD_FINGER_PRESSED 0x26 // Resp: 0=true, elsie false
#define GT_CMD_DELETE_FINGER 0x40 // Param: fingerprint id, resp: NACK_INVALID_POS
#define GT_CMD_DELETE_ALL 0x41 // Resp: NACK_DB_IS_EMPTY
#define GT_CMD_VERIFY_FINGER 0x50 // Param: fingerprint id, Resp: NACK_INVALID_POS, KAC_IS_NOT_USED, NACK_VERIFY_FAILED
#define GT_CMD_IDENTIFY_FINGER 0x51 // Resp: fingerprint id, NACK_DB_IS_EMPTY, NACK_IDENTIFY_FAILED
#define GT_CMD_VERIFY_TEMPLATE 0x52 // Param: identify, resp: NACK_INVALID_POS,NACK_IS_NOT_USED, data: template, resp: NACK_COMM_ERR, NACK_VERIFY_FAILED
#define GT_CMD_IDENTIFY_TEMPLATE 0x53 // resp: NACK_DB_IS_EMPTY, data: template, resp: NACK_COMM_ERR, NACK_IDENTIFY_FAILED
#define GT_CMD_CAPTURE_FINGER 0x60 // Param: 0=fast, else best q, resp: NACK_FINGER_IS_NOT_PRESSED
#define GT_CMD_CREATE_TEMPLATE 0x61 // Resp: NACK_BAD_FINGER, data:template
#define GT_CMD_GET_IMAGE 0x62 // Resp: ACK, data: 258x202 (52116 bytes)
#define GT_CMD_GET_RAW_IMAGE 0x63 // Resp: ACK, data: 160x120 (19200 bytes)
#define GT_CMD_GET_TEMPLATE 0x70 // Param: fingerprint id, resp: NACK_INVALID_POS, NACK_IS_NOT_USED, data: template
#define GT_CMD_SET_TEMPLATE 0x71 // Param: fingerprint id, resp: NACK_INVALID_POS, >data: template, resp: NACK_COMM_ERR, NACK_DEV_ERR, fingerprint_id
#define GT_CMD_GET_DB_START 0x72 // Deprecated
#define GT_CMD_GET_DB_END 0x73 // Deprecated
#define GT_CMD_SET_SECURITY_LEVEL 0xF0 // Param 1-5, default:3, resp: ACK
#define GT_CMD_GET_SECURITY_LEVEL 0xF1 // Resp: security-level
#define GT_CMD_IDENTIFY_TEMPLATE2 0xF4 // param: "500", resp: NACK_INVALID_PARAM, data: template + 0x0101, resp: NACK_IDENTIFY_FAILED
#define GT_CMD_ENTER_STANDBY 0xF9 // resp: ACK, 0x00 to wake
#define GT_CMD_RESP_ACK 0x30
#define GT_CMD_RESP_NACK 0x31

#define GT_PARAM_NACK_TIMEOUT 0x1001
#define GT_PARAM_NACK_INVALID_BAUDRATE 0x1002
#define GT_PARAM_NACK_INVALID_POS 0x1003
#define GT_PARAM_NACK_IS_NOT_USED 0x1004
#define GT_PARAM_NACK_IS_ALREADY_USED 0x1005
#define GT_PARAM_NACK_COMM_ERR 0x1006
#define GT_PARAM_NACK_VERIFY_FAILED 0x1007
#define GT_PARAM_NACK_IDENTIFY_FAILED 0x1008
#define GT_PARAM_NACK_DB_IS_FULL 0x1009
#define GT_PARAM_NACK_DB_IS_EMPTY 0x100A
#define GT_PARAM_NACK_TURN_ERR 0x100B
#define GT_PARAM_NACK_BAD_FINGER 0x100C
#define GT_PARAM_NACK_ENROLL_FAILED 0x100D
#define GT_PARAM_NACK_IS_NOT_SUPPORTED 0x100E
#define GT_PARAM_NACK_DEV_ERR 0x100F
#define GT_PARAM_NACK_CAPTURE_CANCELED 0x1010
#define GT_PARAM_NACK_INVALID_PARAM 0x1011
#define GT_PARAM_NACK_FINGER_IS_NOT_PRESSED 0x1012
#define GT_PARAM_NACK_RAM_ERROR 0x1013
#define GT_PARAM_NACK_TEMPLATE_CAPACITY_FULL 0x1014
#define GT_PARAM_NACK_COMMAND_NO_SUPPORT 0x1015


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
  uint8_t packet[len + 6];
  *((uint16_t *)packet) = isCMD ? CmdHeader : DataHeader;
  packet[2] = 1;
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
  uint8_t packet[buf_size + 6];

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
