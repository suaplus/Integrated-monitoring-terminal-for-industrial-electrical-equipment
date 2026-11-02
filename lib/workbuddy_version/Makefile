#
# Makefile
#
CC = arm-linux-gnueabihf-gcc
LVGL_DIR_NAME ?= lvgl
LVGL_DIR ?= ${shell pwd}
CFLAGS ?= -O3 -g0 -I$(LVGL_DIR)/ -Wall -std=c99#-Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare
LDFLAGS ?= -lm -lpthread
BIN = demo

# 给主工程（src/）链接用的静态库：LVGL + lv_drivers + demo1 界面
# 注意：不含本目录的 main.c —— 主工程的 main() 在 src/main.c 里，否则重复定义
LIB = liblvglui.a


#Collect the files to compile
MAINSRC = ./main.c

include $(LVGL_DIR)/lvgl/lvgl.mk
include $(LVGL_DIR)/lv_drivers/lv_drivers.mk

CSRCS +=$(LVGL_DIR)/mouse_cursor_icon.c 

CSRCS += $(wildcard $(LVGL_DIR)/demo1/*.c)
CSRCS += $(wildcard $(LVGL_DIR)/demo1/components/*.c)
CSRCS += $(wildcard $(LVGL_DIR)/demo1/screens/*.c)
CSRCS += $(wildcard $(LVGL_DIR)/demo1/fonts/*.c)
CSRCS += $(wildcard $(LVGL_DIR)/demo1/images/*.c)

OBJEXT ?= .o

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))

MAINOBJ = $(MAINSRC:.c=$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS)

## MAINOBJ -> OBJFILES

all: default

%.o: %.c
	@$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"
    
default: $(AOBJS) $(COBJS) $(MAINOBJ)
	$(CC) -o $(BIN) $(MAINOBJ) $(AOBJS) $(COBJS) $(LDFLAGS)

# 只打包 LVGL/驱动/界面，不打包 MAINOBJ
lib: $(AOBJS) $(COBJS)
	$(AR) rcs $(LIB) $(AOBJS) $(COBJS)
	@echo "AR $(LIB)"

.PHONY: all default lib clean

clean: 
	rm -f $(BIN) $(LIB) $(AOBJS) $(COBJS) $(MAINOBJ)

