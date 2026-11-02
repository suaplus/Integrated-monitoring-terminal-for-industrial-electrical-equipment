modulename := adxl345_drv

obj-m += $(modulename).o 

kerdir := /home/linux/imx6ull/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek

pwd := $(shell pwd)

all:
	make -C $(kerdir) modules M=$(pwd) 
	cp $(modulename).ko ~/nfs/rootfs 

.PHONY:
distclean:
	make -C $(kerdir) modules M=$(pwd) clean
	rm ~/nfs/rootfs/$(modulename).ko
clean:
	make -C $(kerdir) modules M=$(pwd) clean