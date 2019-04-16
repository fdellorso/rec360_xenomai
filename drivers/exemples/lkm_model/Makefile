ARCH = arm
MODULE_DIR = $(shell pwd)
EXTRA_CFLAGS := -I${XENOMAI_DIR}/include

obj-m += xxx.o

.PHONY: all
all:: modules

.PHONY: modules
modules:
	${MAKE} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -C ${KBUILD_DIR} SUBDIRS=${MODULE_DIR} modules

.PHONY: clean
clean::
	rm -f  *.o  .*.o  .*.o.* *.ko  .*.ko  *.mod.* .*.mod.* .*.cmd *~
	rm -f Module.symvers Module.markers modules.order 
	rm -rf .tmp_versions
