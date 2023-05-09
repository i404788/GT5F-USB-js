import cppyy
import os
import time


usb_lib_path = os.path.dirname(__file__) + '/usb_protoc.hpp'
cppyy.cppdef(f'#include "{usb_lib_path}"' +
             r'''
#include<stdio.h>
#include<string>

#pragma pack(push, 1)

typedef struct _templ{
    uint8_t blob[498];
} templ;

typedef _generic_packet<templ> TemplatePacket;
#pragma pack(pop)


int32_t find_scanner() {
    // Returns filedescriptor for scanner otherwise < 0
    printf("Finding scanners\n");
    CmdPacket result;
    for (int i = 0; i <= 9; i++) {
        std::string path = "/dev/sr" + std::to_string(i);
        printf("Trying %s\n", path.c_str());
        int fd = gethandle(path.c_str());
        if (fd < 0)
            continue;

        runCommand(fd, GT_CMD_USB_VALID, 0x00, result);
        if (result.data.param_data == 0x55 &&
            result.data.cmd_ack == GT_CMD_RESP_ACK){
            return fd;
        }
        close(fd);
    }
    return -1;
}

class Scanner {
    int32_t _fd;
    bool owner;
public:

    ~Scanner() {
        if (_fd > 0 && owner)
            close(_fd);
    }

    Scanner(int32_t fd, bool take_ownership = false) {
        _fd = fd;
        owner = take_ownership;
    }


    bool connected() {
        if (_fd <= 0)
            return false;

        CmdPacket result;
        runCommand(_fd, GT_CMD_USB_VALID, 0x00, result);
        if (result.data.param_data == 0x55 &&
            result.data.cmd_ack == GT_CMD_RESP_ACK){
            return true;
        }
        return false;
    }

    void set_led(bool value)
    {
        CmdPacket result;
        runCommand(_fd, GT_CMD_LED, value, result);
    }

    int32_t enrolled_count() {
        CmdPacket result;
        runCommand(_fd, GT_CMD_ENROLL_COUNT, 0x00, result);
        if (result.data.cmd_ack == GT_CMD_RESP_ACK) {
            return result.data.param_data;
        }
        return -1;
    }

    bool check_enrolled(uint32_t id) {
        CmdPacket result;
        runCommand(_fd, GT_CMD_CHECK_ENROLLED, (int32_t)id, result);
        if (result.data.cmd_ack == GT_CMD_RESP_ACK)
            return true;

        return false; // INVALID_POS or NOT_USED
    }

    bool is_finger_pressed() {
        usleep(50000);
        CmdPacket result;
        runCommand(_fd, GT_CMD_FINGER_PRESSED, 0x00, result);
        return !result.data.param_data;
    }

    int32_t enroll_generic(uint32_t id, uint8_t step)
    {
        CmdPacket result;
        runCommand(_fd, GT_CMD_ENROLL_START + step, step ? 0x00 : id, result);
        if (result.data.cmd_ack == GT_CMD_RESP_ACK) {
            step++;
            if (step > 3)
                return 0; // Done!
            return step;
        }
        else
        {
            // Scanner doesn't support continued capture
            // (will give 0x100b after retry stepN)
            // Need to restart from scratch unfortunately
            set_led(false);
            return -result.data.param_data;
        }
    }

    // Callback param: 0=done, <0=error, >0=steps-left
    // Does identify on existing db at the end
    // (if > -3000 , this enrollment is duplicate of that id)
    int32_t enroll(uint32_t id, void(*status_cb)(int32_t)) {
        int32_t step = 0;
        CmdPacket result;

        do {
            if (step > 0)
            {
                set_led(true);
                bool captured = false;
                do {
                    while(!is_finger_pressed()) {
                        status_cb(step);
                    }
                    captured = capture_finger();
                } while(!captured);
            }

            // Should go through 0x22-0x25
            printf("Enroll %x %x\n", GT_CMD_ENROLL_START + step, step);
            runCommand(_fd, GT_CMD_ENROLL_START + step,
                        step ? 0x00 : id, result);
            if (result.data.cmd_ack == GT_CMD_RESP_ACK) {
                step++;
                if (step > 1) {
                    while(is_finger_pressed()) {
                        // Lift finger status
                        status_cb(0xf0f0 | step); // 0xf0fX
                    }
                    set_led(false);
                    usleep(250000);
                }
            }
            else
            {
                // Scanner doesn't support continued capture
                // (will give 0x100b after retry stepN)
                // Need to restart from scratch unfortunately
                set_led(false);
                return -result.data.param_data;
            }
            status_cb(step);
        } while(step <= 3);
        set_led(false);
        return id;
    }

    bool capture_finger(bool fast = false)
    {
        CmdPacket result;
        runCommand(_fd, GT_CMD_CAPTURE_FINGER, !fast, result);
        return (result.data.cmd_ack == GT_CMD_RESP_ACK);
    }

    bool delete_finger(uint32_t id) {
        CmdPacket result;
        runCommand(_fd, GT_CMD_DELETE_FINGER, id, result);
        return (result.data.cmd_ack == GT_CMD_RESP_ACK);
    }

    bool delete_all() {
        CmdPacket result;
        runCommand(_fd, GT_CMD_DELETE_ALL, 0x00, result);
        return (result.data.cmd_ack == GT_CMD_RESP_ACK);
    }

    // Requires finger_pressed
    bool verify(uint32_t id){
        CmdPacket result;
        runCommand(_fd, GT_CMD_VERIFY_FINGER, id, result);
        return (result.data.cmd_ack == GT_CMD_RESP_ACK);
    }

    int32_t identify() {
        CmdPacket result;
        runCommand(_fd, GT_CMD_IDENTIFY_FINGER, 0x00, result);
        if (result.data.cmd_ack == GT_CMD_RESP_ACK) {
            return result.data.param_data;
        } else {
            return -result.data.param_data;
        }
    }

    int32_t get_template(uint32_t id, TemplatePacket &packet) {
        CmdPacket result;
        runCommand(_fd, GT_CMD_GET_TEMPLATE, id, result);
        if (result.data.cmd_ack != GT_CMD_RESP_ACK) {
            return -result.data.param_data;
        }

        recvPacket(_fd, packet);
        return 0;
    }

    std::string get_template_str(uint32_t id) {
        TemplatePacket packet;
        if (get_template(id, packet) < 0) {
            return "";
        }
        return std::string((char*)packet.data.blob, sizeof(packet.data.blob));
    }

    int32_t set_template(uint32_t id, TemplatePacket &packet) {
        CmdPacket result;
        runCommand(_fd, GT_CMD_SET_TEMPLATE, id, result);
        if (result.data.cmd_ack != GT_CMD_RESP_ACK) {
            return -result.data.param_data;
        }

        sendPacket(_fd, packet);
        recvPacket(_fd, result);
        if (result.data.cmd_ack != GT_CMD_RESP_ACK) {
            return -result.data.param_data;
        }
        return 0;
    }

    int32_t set_template_str(uint32_t id, std::string &tmpl) {
        TemplatePacket packet = {DataHeader, 0x1, {0}};
        if (tmpl.length() != 498) {
            return -1;
        }
        memcpy(packet.data.blob, tmpl.data(), tmpl.length());
        return set_template(id, packet);
    }

    int32_t verify_template(uint32_t id, TemplatePacket &packet) {
        CmdPacket result;
        runCommand(_fd, GT_CMD_VERIFY_TEMPLATE, id, result);
        if (result.data.cmd_ack != GT_CMD_RESP_ACK) {
            return -result.data.param_data;
        }
        sendPacket(_fd, packet);
        recvPacket(_fd, result);
        if (result.data.cmd_ack != GT_CMD_RESP_ACK) {
            return -result.data.param_data;
        }
        return 0;
    }

    int32_t identify_template(TemplatePacket &packet) {
        CmdPacket result;
        runCommand(_fd, GT_CMD_IDENTIFY_TEMPLATE, 0x00, result);
        if (result.data.cmd_ack != GT_CMD_RESP_ACK) {
            return -result.data.param_data;
        }
        sendPacket(_fd, packet);
        recvPacket(_fd, result);
        if (result.data.cmd_ack != GT_CMD_RESP_ACK) {
            return -result.data.param_data;
        }
        return result.data.param_data;
    }

    int32_t identify_template_str(std::string &tmpl) {
        TemplatePacket packet = {DataHeader, 0x1, {0}};
        if (tmpl.length() != 498) {
            return -1;
        }
        memcpy(packet.data.blob, tmpl.data(), tmpl.length());
        return identify_template(packet);
    }

    int32_t set_security_level(uint8_t level) {
        if (!(5 < level < 1)) {
            printf("Security level should be between 1-5 got %d, refusing request", level);
            return -2;
        }

        CmdPacket result;
        runCommand(_fd, GT_CMD_SET_SECURITY_LEVEL, level, result);
        if (result.data.cmd_ack != GT_CMD_RESP_ACK) {
            return -result.data.param_data;
        }
        return 0;
    }

    uint8_t get_security_level() {
        CmdPacket result;
        runCommand(_fd, GT_CMD_GET_SECURITY_LEVEL, 0x00, result);
        return result.data.param_data;
    }

    bool standby() {
        CmdPacket result;
        runCommand(_fd, GT_CMD_ENTER_STANDBY, 0x00, result);
        return result.data.cmd_ack == GT_CMD_RESP_ACK;
    }

    void wake() {
        // Only needed after standby
        CmdPacket result;
        runCommand(_fd, GT_CMD_WAKE, 0x00, result);
        usleep(20000); // Wait at least 20ms
    }
};
''')

gt_nack_map = {
    0x0001: "REQUEST_FAILED",
    0x0002: "INVALID_PARAM",

    0x1001: "TIMEOUT",
    0x1002: "INVALID_BAUDRATE",
    0x1003: "INVALID_POS",
    0x1004: "IS_NOT_USED",
    0x1005: "IS_ALREADY_USED",
    0x1006: "COMM_ERR",
    0x1007: "VERIFY_FAILED",
    0x1008: "IDENTIFY_FAILED",
    0x1009: "DB_IS_FULL",
    0x100A: "DB_IS_EMPTY",
    0x100B: "TURN_ERR",
    0x100C: "BAD_FINGER",
    0x100D: "ENROLL_FAILED",
    0x100E: "IS_NOT_SUPPORTED",
    0x100F: "DEV_ERR",
    0x1010: "CAPTURE_CANCELED",
    0x1011: "INVALID_PARAM",
    0x1012: "FINGER_IS_NOT_PRESSED",
    0x1013: "RAM_ERROR",
    0x1014: "TEMPLATE_CAPACITY_FULL",
    0x1015: "COMMAND_NO_SUPPORT",
}


def gt_errno(v):
    if v is None:
        return v
    if v < 0:
        return gt_nack_map.get(-v, f'0x{v:x}')
    return v


if True:  # Prevent import re-ordering
    from cppyy.gbl import find_scanner, Scanner, TemplatePacket

if __name__ == "__main__":
    fd = find_scanner()
    print('fd: ', fd)
    scanner = Scanner(fd)
    enrolled = gt_errno(scanner.enrolled_count())
    print('Enrolled:', enrolled)
    if enrolled > 3:
        print(f'Removing fingerprints {gt_errno(scanner.delete_all())}')
        enrolled = gt_errno(scanner.enrolled_count())
        print('Enrolled:', enrolled)

    def print_status(status: int):
        print(f"Status: {status:x}")
        time.sleep(0.1)

    finger_id = 2
    print(f'enroll_ret {gt_errno(scanner.enroll(finger_id, print_status))}')
    template = scanner.get_template_str(finger_id)
    print(f'template res: {template}')
    assert scanner.delete_all(), "Expecting scanner to delete all"
    res = scanner.set_template_str(finger_id, template)
    breakpoint()
    #print('delete', scanner.delete_finger(1))

    del scanner
