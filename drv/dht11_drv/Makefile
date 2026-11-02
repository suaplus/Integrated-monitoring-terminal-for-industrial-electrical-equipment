MODULENAME:=dht11
MODULE_DRV:=$(MODULENAME)_drv

# 驱动模块加入到内核编译 dht11_drv.o
obj-m += $(MODULE_DRV).o

# 指定内核源码路径
KERDIR:=/home/linux/imx6ull/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek

# 指定驱动模块路径(当前路径)
MODULEDIR:=$(shell pwd)

# cd /home/linux/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek -> make modules -> ./dht11_drv.ko
# cp dht11_drv.ko ~/nfs/rootfs
all:
	make -C $(KERDIR) M=$(MODULEDIR) modules
	cp $(MODULE_DRV).ko ~/nfs/rootfs

.PHONY:
clean:
	make -C $(KERDIR) M=$(MODULEDIR) clean
	rm ~/nfs/rootfs/$(MODULE_DRV).ko

