ARCH = arm
MODULE_DIR = $(shell pwd)
EXTRA_CFLAGS := -I${XENOMAI_DIR}/include

obj-m += rtdm-gpio-estop.o

.PHONY: all
all:: modules

.PHONY: modules
modules:
	${MAKE} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -C ${LINUX_DIR} SUBDIRS=${MODULE_DIR} modules

.PHONY: dtoverlay
dtoverlay:
	cp $(MODULE_DIR)/dtoverlay/* $(LINUX_DIR)/arch/arm/boot/dts/overlays/

.PHONY: config.txt
config.txt:
	@echo "dtoverlay=gpio-estop" >> $(BOOT_DIR)config.txt

.PHONY: clean
clean::
	rm -f  *.o  .*.o  .*.o.* *.ko  .*.ko  *.mod.* .*.mod.* .*.cmd *~
	rm -f Module.symvers Module.markers modules.order 
	rm -rf .tmp_versions
