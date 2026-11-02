/*
 * spt660_read.c -- SPT660 三相电表读取：电压 / 电流 / 功率 / 功率因数 / 频率
 *
 * 交叉编译：
 *   arm-linux-gnueabihf-gcc -Wall -O2 spt660_read.c crc.c -o spt660_read
 *
 * 用法：
 *   ./spt660_read                              默认 /dev/ttymxc2 9600 地址1 CT=1 PT=1
 *   ./spt660_read /dev/ttymxc2 9600 1 40 10    指定串口/波特率/站点号/CT变比/PT变比
 *
 * 一次读寄存器 0x0014 起 27 个（0x14~0x2E），覆盖：
 *   相电压/线电压、三相电流、有功/无功/视在功率、功率因数、频率
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/types.h>
#include "crc.h"

#define REG_START   0x0014
#define REG_COUNT   27                  /* 0x14 ~ 0x2E */

/* ---------------- 串口 ---------------- */
static int tty_open(const char *dev, int baud)
{
    int fd = open(dev, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        printf("[ERR] open %s: %s\n", dev, strerror(errno));
        return -1;
    }

    struct termios tio;
    if (tcgetattr(fd, &tio) != 0) {
        printf("[ERR] tcgetattr: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    cfmakeraw(&tio);
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= ~CSIZE;
    tio.c_cflag |= CS8;
    tio.c_cflag &= ~PARENB;             /* n,8,1 */
    tio.c_cflag &= ~CSTOPB;
    tio.c_cflag &= ~CRTSCTS;
    tio.c_cc[VMIN]  = 0;
    tio.c_cc[VTIME] = 0;

    speed_t sp;
    switch (baud) {
        case 2400:   sp = B2400;   break;
        case 4800:   sp = B4800;   break;
        case 9600:   sp = B9600;   break;
        case 19200:  sp = B19200;  break;
        case 38400:  sp = B38400;  break;
        default:     sp = B9600;   break;
    }
    cfsetispeed(&tio, sp);
    cfsetospeed(&tio, sp);

    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd, TCSANOW, &tio) != 0) {
        printf("[ERR] tcsetattr: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    printf("[OK] %s opened, baud=%d\n", dev, baud);
    return fd;
}

/* 把一帧读完整：循环 read 直到收满 len 字节，总超时 ms 毫秒 */
static int read_exact(int fd, unsigned char *buf, int len, int ms)
{
    int got = 0;
    while (got < len) {
        fd_set rfds;
        struct timeval tv;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        tv.tv_sec  = ms / 1000;
        tv.tv_usec = (ms % 1000) * 1000;

        int r = select(fd + 1, &rfds, NULL, NULL, &tv);
        if (r == 0)  return got;        /* 超时，返回已收到的 */
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        int n = read(fd, buf + got, len - got);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;
            return -1;
        }
        got += n;
    }
    return got;
}

/* Modbus-RTU 读保持寄存器，成功返回 0 */
static int modbus_read_regs(int fd, unsigned char addr,
                            unsigned short start, unsigned short count,
                            unsigned short *regs)
{
    int retry = 0;
    int i = 0;
    
    unsigned char req[8], rsp[256];
    req[0] = addr;
    req[1] = 0x03;
    req[2] = start >> 8;
    req[3] = start & 0xFF;
    req[4] = count >> 8;
    req[5] = count & 0xFF;
    unsigned short c = CRC16_MODBUS(req, 6);
    req[6] = c & 0xFF;                  /* CRC 低字节在前 */
    req[7] = c >> 8;

    int expect = 5 + 2 * count;         /* addr+func+bytecnt + data + crc */

    for (retry = 0; retry < 3; retry++) {
        if (write(fd, req, 8) != 8) {
            printf("[ERR] write: %s\n", strerror(errno));
            return -1;
        }
        tcdrain(fd);

        int n = read_exact(fd, rsp, expect, 500);
        if (n == 0)  continue;          /* 超时重试 */
        if (n != expect) {
            printf("[WARN] 短帧 %d/%d, 可能波特率不对\n", n, expect);
            tcflush(fd, TCIFLUSH);
            continue;
        }
        if (rsp[0] != addr) { printf("[WARN] 地址不匹配 %02X\n", rsp[0]); continue; }
        if (rsp[1] & 0x80)  { printf("[WARN] 异常应答 %02X\n", rsp[2]); return -2; }

        c = CRC16_MODBUS(rsp, n - 2);
        if (rsp[n-2] != (c & 0xFF) || rsp[n-1] != (c >> 8)) {
            printf("[WARN] 应答 CRC 错\n");
            continue;
        }
        for (i = 0; i < count; i++)
            regs[i] = (rsp[3 + 2*i] << 8) | rsp[4 + 2*i];
        return 0;
    }
    return -1;
}

/* ---------------- 主循环 ---------------- */
int spt660_data_get(float *ua, float *ia, float *p)
{
    const char *dev = "/dev/ttymxc2";
    int baud = 9600, addr = 1;
    float ct = 1.0f, pt = 1.0f;
/*
    if (argc >= 2) dev  = argv[1];
    if (argc >= 3) baud = atoi(argv[2]);
    if (argc >= 4) addr = atoi(argv[3]);
    if (argc >= 5) ct   = atof(argv[4]);
    if (argc >= 6) pt   = atof(argv[5]);
*/
    int fd = tty_open(dev, baud);
    if (fd < 0) return 1;

    unsigned short regs[REG_COUNT];

    //while (1) {

        if (modbus_read_regs(fd, (unsigned char)addr, REG_START, REG_COUNT, regs) == 0) {
            /* 数据区索引：regs[0] 对应寄存器 0x0014 */
            /* 0x14~0x19: 相/线电压, 0x1A~0x1C: 三相电流, 0x1D: 方向位 */
            *ua = regs[0]  / 10.0f * pt;   /* 0x14 A相电压 */
            float ub = regs[1]  / 10.0f * pt;   /* 0x15 B相电压 */
            float uc = regs[2]  / 10.0f * pt;   /* 0x16 C相电压 */
            float uab= regs[3]  / 10.0f * pt;   /* 0x17 AB线电压 */
            float ubc= regs[4]  / 10.0f * pt;
            float uca= regs[5]  / 10.0f * pt;

            *ia = regs[6]  / 1000.0f * ct; /* 0x1A A相电流 */
            float ib = regs[7]  / 1000.0f * ct;
            float ic = regs[8]  / 1000.0f * ct;

            /* 0x1D 方向寄存器: bit0~3=Pa/Pb/Pc/P总, bit4~7=Q, bit8~11=PF, 1=负 */
            unsigned sign = regs[9];
            float pa = regs[10] / 10.0f * ct * pt;  /* 0x1E A相有功 W */
            float pb = regs[11] / 10.0f * ct * pt;
            float pc = regs[12] / 10.0f * ct * pt;
            *p  = regs[13] / 10.0f * ct * pt;  /* 0x21 总有功 */
            if (sign & (1<<0)) pa = -pa;
            if (sign & (1<<1)) pb = -pb;
            if (sign & (1<<2)) pc = -pc;
            if (sign & (1<<3)) *p  = -*p;

            float qa = regs[14] / 10.0f * ct * pt;  /* 0x22 A相无功 var */
            float qb = regs[15] / 10.0f * ct * pt;
            float qc = regs[16] / 10.0f * ct * pt;
            float q  = regs[17] / 10.0f * ct * pt;
            if (sign & (1<<4)) qa = -qa;
            if (sign & (1<<5)) qb = -qb;
            if (sign & (1<<6)) qc = -qc;
            if (sign & (1<<7)) q  = -q;

            float pfa = regs[22] / 1000.0f;     /* 0x2A A相功率因数 */
            float pfb = regs[23] / 1000.0f;     /* 0x2B */
            float pfc = regs[24] / 1000.0f;     /* 0x2C */
            float pf  = regs[25] / 1000.0f;     /* 0x2D 总功率因数 */
            if (sign & (1<<8))  pfa = -pfa;
            if (sign & (1<<9))  pfb = -pfb;
            if (sign & (1<<10)) pfc = -pfc;
            if (sign & (1<<11)) pf  = -pf;

            float f = regs[26] / 100.0f;        /* 0x2E 频率 */
            /*
            printf("U : Ua=%.1f  Ub=%.1f  Uc=%.1f | Uab=%.1f Ubc=%.1f Uca=%.1f (V)\n",
                   ua, ub, uc, uab, ubc, uca);
            printf("I : Ia=%.3f  Ib=%.3f  Ic=%.3f (A)\n", ia, ib, ic);
            printf("P : Pa=%.1f  Pb=%.1f  Pc=%.1f  P=%.1f (W)\n", pa, pb, pc, p);
            printf("Q : Qa=%.1f  Qb=%.1f  Qc=%.1f  Q=%.1f (var)\n", qa, qb, qc, q);
            printf("PF: %.3f / %.3f / %.3f  total=%.3f\n", pfa, pfb, pfc, pf);
            printf("F : %.2f Hz\n", f);
            printf("--------------------------------\n");
            */
        } else {
            printf("[TIMEOUT] 无应答 (检查接线/地址/波特率)\n");
        }
    //    sleep(1);
    //}

    close(fd);
    return 0;
}
