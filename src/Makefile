OBJ = app
SRC = main.c ui_app.c qlink.c mylink.c mailbox.c mysqlite3.c mymqtt.c mq2.c crc.c spt660_read.c
CC = arm-linux-gnueabihf-gcc

INC = ../include
INC += -I../lib/sqlite3/include
LIBS = ../lib/sqlite3/lib

LIBS+= -L../lib/build/output
LIBS+= -L../lib/openssl_install/lib
INC+=-I../include/src
INC+= -I../lib/openssl_install/src

# ---- LVGL（lib/workbuddy_version）：源码先编成静态库 liblvglui.a，主工程再链接 ----
LVGL_ROOT = ../lib/workbuddy_version
INC      += -I$(LVGL_ROOT)
LIBLVGL   = $(LVGL_ROOT)/liblvglui.a

# paho 的动态库可能只有带版本号的文件、没有 .so 软链接，这里按三级降级
PAHO_SO := $(firstword $(wildcard ../lib/build/output/libpaho-mqtt3c.so \
                                ../lib/build/output/libpaho-mqtt3c.so.1.0))
ifneq ($(PAHO_SO),)
PAHO_LINK = $(PAHO_SO)
else
PAHO_LINK = -lpaho-mqtt3c
endif

# -ldl：libsqlite3.a 里的 unixDlOpen 用到 dlopen/dlsym，老 glibc 下 libdl 是独立库
FLAGS = -g -lpthread -lsqlite3 -lrt -lssl -lcrypto -lm -ldl

# liblvglui.a 要放在源文件之后、系统库之前（否则 -lm/-lpthread 可能被 --as-needed 丢掉）
$(OBJ) : $(SRC) $(LIBLVGL)
	$(CC) $(SRC) -o $(OBJ) -I$(INC) -L$(LIBS) $(LIBLVGL) $(PAHO_LINK) $(FLAGS)
	make -C ../drv/adxl345_drv
	make -C ../drv/dht11_drv
	cp $(OBJ) ~/nfs/rootfs

$(LIBLVGL) : FORCE
	$(MAKE) -C $(LVGL_ROOT) lib

FORCE:

.PHONY: lvgl clean

lvgl:
	$(MAKE) -C $(LVGL_ROOT) lib

clean:
	rm -f $(OBJ)
	rm ~/nfs/rootfs/$(OBJ)
	make -C ../drv/adxl345_drv clean
	make -C ../drv/dht11_drv clean
