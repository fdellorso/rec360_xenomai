# Linux 4.14.85			branch rpi-4.14.y		commit 802d8776632344a4354d8ef5f142611a4c878570
# Linux 4.14.110		branch rpi-4.14.y		Not committed on https://github.com/raspberrypi/linux
# Linux 4.19.33			branch rpi-4.19.y		commit 4b3a3ab00fa7a951eb1d7568c71855e75fd5af85
# Linux 4.19.60			branch rpi-4.19.y		commit 2b3cf6c405f000c7b25953ab138d4dca0acaf74f
# Xenomai 3.1 latest	branch master			commit fbc3271096c63b21fe895c66ba20b1d10d72ff48
# tools latest			branch master			commit 5caa7046982f0539cf5380f94da04b31129ed521
# gcc 9.3.0										https://github.com/abhiTronix/raspberry-pi-cross-compilers

# scp xenomai-kernel.tgz pi@<ipaddress>:/tmp
# scp -r kernel-output/lib/modules/4.19.127+/kernel/drivers/stufa root@192.168.8.70:/lib/modules/4.19.127+/kernel/drivers

export LINUX_DIR			:= $(PWD)/kernel
export KBUILD_DIR			:= $(PWD)/kernel-build
export KERNEL_DIR			:= $(PWD)/kernel-output
export KPACKAGE_DIR			:= $(PWD)/kernel-package
export XENOMAI_DIR			:= $(PWD)/xenomai
export XBUILD_DIR			:= $(PWD)/xenomai-build
export XTOOLS_DIR			:= ${PWD}/xenomai-tools

export BCM2835LIB_DIR		:= /home/francesco/rpi-xenomai/bcm2835-1.58

# export NPROCS:=1
# export OS:=$(shell uname -s)

# ifeq($(OS),Linux)
#   NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)
# endif
# ifeq($(OS),Darwin) # Assume Mac OS X
#   NPROCS:=$(shell system_profiler | awk '/Number Of CPUs/{print $4}{next;}')
# endif

export CORES				:= -j$(shell nproc)

export ARCH					:= arm
export KERNEL				:= kernel
# export CROSS_COMPILE		:= $(PWD)/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-
# export CROSS_COMPILE		:= $(PWD)/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-7.5.0-2019.12-x86_64/bin/arm-linux-gnueabihf-
export CROSS_COMPILE		:= arm-linux-gnueabihf-

export CFLAGS				:= '-DSTUFA_DEBUG=1 -DSTUFA_SERIAL=0'

export BOOT_DIR				?= /media/francesco/boot
export ROOT_DIR				?= /media/francesco/rootfs

export COPY_EXCLUDE			:= '.clang-format'
export COPY_OPT				:= @rsync -ac --exclude=$(COPY_EXCLUDE) # cp or rsync -c

export CMDLINETXT			:= $(shell cat ${BOOT_DIR}/cmdline.txt)
export CONFIGTXT			:= $(shell cat ${BOOT_DIR}/config.txt)


.PHONY: kernel			kernel_package	\
		kernel_copy2sd					\
		config 			menuconfig		\
		patch_irq		patch_xenomai 	\
		prepare_drivers	overlays 		\
		xtools 			xtools_install	\
		clean_kernel 	clean_xtools	\
		reset_kernel 	reset_xtools


kernel:
	mkdir -p $(KERNEL_DIR)
	make -C $(LINUX_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(KBUILD_DIR) $(CORES) CFLAGS_MODULE=$(CFLAGS) zImage modules dtbs
	make -C $(LINUX_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(KBUILD_DIR) $(CORES) CFLAGS_MODULE=$(CFLAGS) modules_install dtbs_install INSTALL_MOD_PATH=$(KERNEL_DIR) INSTALL_DTBS_PATH=$(KERNEL_DIR)
	mkdir -p $(KERNEL_DIR)/boot
	cd $(LINUX_DIR); ./scripts/mkknlimg $(KBUILD_DIR)/arch/arm/boot/zImage $(KERNEL_DIR)/boot/$(KERNEL).img
	$(COPY_OPT) kernel-patch/Makefile $(KERNEL_DIR)/


kernel_package:
	mkdir -p $(KPACKAGE_DIR)
	cd $(KERNEL_DIR); tar czf $(KPACKAGE_DIR)/xenomai-kernel.tgz *


kernel_copy2sd:
	sudo cp 	$(KERNEL_DIR)/*.dtb			$(BOOT_DIR)
	sudo cp -rd $(KERNEL_DIR)/boot/*		$(BOOT_DIR)
	sudo cp -dr $(KERNEL_DIR)/lib/*			$(ROOT_DIR)/lib/
	sudo cp -d	$(KERNEL_DIR)/overlays/*	$(BOOT_DIR)/overlays/
	sudo cp -d	$(KERNEL_DIR)/bcm*			$(BOOT_DIR)
	sudo umount	$(BOOT_DIR)
	sudo umount $(ROOT_DIR)


# Kernel 4.14.85
# patch_irq:
# 	cp kernel-patch/irq-bcm283* $(LINUX_DIR)/drivers/irqchip/

# patch_xenomai: patch_irq
# 	$(XENOMAI_DIR)/scripts/prepare-kernel.sh --linux=$(LINUX_DIR) --arch=$(ARCH) --ipipe=./xenomai-patch/ipipe-core-4.14.85-arm-6.patch --verbose


# Kernel 4.19.60
# patch_irq:
# 	cd $(LINUX_DIR); git checkout 4b3a3ab00fa7a951eb1d7568c71855e75fd5af85 drivers/irqchip/irq-bcm2835.c drivers/irqchip/irq-bcm2836.c kernel/trace/ftrace.c;


# patch_xenomai: patch_irq
# 	$(XENOMAI_DIR)/scripts/prepare-kernel.sh --linux=$(LINUX_DIR) --arch=$(ARCH) --ipipe=./xenomai-patch/ipipe-core-4.19.33-arm-2.patch --verbose


# Kernel 4.19.127
patch_irq:
	cp kernel-patch/irq-bcm283* $(LINUX_DIR)/drivers/irqchip/


patch_xenomai:
	$(XENOMAI_DIR)/scripts/prepare-kernel.sh --linux=$(LINUX_DIR) --arch=$(ARCH) --ipipe=./xenomai-patch/ipipe-core-4.19.128-arm-9.patch --verbose


config:
	mkdir -p $(KBUILD_DIR)
	make -C $(LINUX_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(KBUILD_DIR) $(CORES) bcmrpi_defconfig
	# if ! patch -R -p0 -s -f --dry-run patch $(KBUILD_DIR)/.config kernel-patch/defconfig.patch; then \
	# 	patch $(KBUILD_DIR)/.config kernel-patch/defconfig.patch; \
	# fi


menuconfig:
	make -C $(LINUX_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(KBUILD_DIR) $(CORES) menuconfig


prepare_drivers:
	# PWM Serialiser modified driver
	# $(COPY_OPT) drivers/include/linux/pwm_dev.h $(LINUX_DIR)/include/linux/pwm.h
	# @sed 's+#include <linux/pwm_dev.h>+#include <linux/pwm.h>+g' \
	# 	  drivers/drivers/pwm/core.c > $(LINUX_DIR)/drivers/pwm/core.c
	# @sed 's+#include <linux/pwm_dev.h>+#include <linux/pwm.h>+g' \
	# 	  drivers/drivers/pwm/pwm-bcm2835.c > $(LINUX_DIR)/drivers/pwm/pwm-bcm2835.c
	$(COPY_OPT) drivers/include/linux/pwm.h $(LINUX_DIR)/include/linux/
	$(COPY_OPT) drivers/drivers/pwm/* $(LINUX_DIR)/drivers/pwm/
	# DMA modified driver
	$(COPY_OPT) drivers/drivers/dma/bcm2835-dma.c $(LINUX_DIR)/drivers/dma/
	# if ! patch -R -p0 -s -f --dry-run $(LINUX_DIR)/drivers/dma/bcm2835-dma.c drivers/drivers/dma/bcm2835-dma.K4.19.127.patch; then \
	# 	patch $(LINUX_DIR)/drivers/dma/bcm2835-dma.c drivers/drivers/dma/bcm2835-dma.K4.19.127.patch; \
	# fi
	# StuFA Defines
	$(COPY_OPT) -r drivers/include/stufa $(LINUX_DIR)/include/
	# StuFA Drivers & Task Module
	$(COPY_OPT) -r drivers/drivers/stufa $(LINUX_DIR)/drivers/
	# Enable StuFA Drivers to compile
	if ! grep -q stufa '$(LINUX_DIR)/drivers/Makefile'; then \
		echo -n "obj-y += stufa/" >> $(LINUX_DIR)/drivers/Makefile; \
	fi
	# Enable Xenomai Library to DMA Folder
	if ! grep -q xenomai '$(LINUX_DIR)/drivers/dma/Makefile'; then \
		echo -n "ccflags-y += -I../xenomai/include" >> $(LINUX_DIR)/drivers/dma/Makefile; \
	fi
	# Add StuFA Driver Folder
	if ! patch -R -p0 -s -f --dry-run $(LINUX_DIR)/drivers/Kconfig drivers/drivers/Kconfig.patch; then \
		patch $(LINUX_DIR)/drivers/Kconfig drivers/drivers/Kconfig.patch; \
	fi


overlays:
	$(COPY_OPT) drivers/overlays/*.dts $(LINUX_DIR)/arch/arm/boot/dts/overlays/
	if ! patch -R -p0 -s -f --dry-run $(LINUX_DIR)/arch/arm/boot/dts/overlays/Makefile drivers/overlays/Makefile.K4.19.127.patch; then \
		patch $(LINUX_DIR)/arch/arm/boot/dts/overlays/Makefile drivers/overlays/Makefile.K4.19.127.patch; \
	fi


prepare_cmdlinetxt:
	# TODO to check
	if ! grep -q dwc_otg '$(BOOT_DIR)/cmdline.txt'; then \
		echo -n '$(CMDLINETXT) dwc_otg.fiq_enable=0 dwc_otg.fiq_fsm_enable=0 dwc_otg.nak_holdoff=0' > $(BOOT_DIR)/cmdline.txt; \
	fi
	# console=ttyS0,115200 console=tty1 root=PARTUUID=e8af6eb2-02 rootfstype=ext4 elevator=deadline fsck.repair=yes rootwait quiet net.ifnames=0


prepare_configtxt:
	if ! grep -q kernel '$(BOOT_DIR)/config.txt'; then \
		echo "\n\nkernel=kernel.img" >> $(BOOT_DIR)/config.txt; \
		echo "\ndevice_tree=bcm2708-rpi-zero-w.dtb" >> $(BOOT_DIR)/config.txt; \
		echo "\n# Set GPIO6 (Fan) to be an output set to 0" >> $(BOOT_DIR)/config.txt; \
		echo "gpio=6=op,dl" >> $(BOOT_DIR)/config.txt; \
		echo "\n# Set GPIO7,8 (Hall_Int & Gyro_Int) to be an input and pull-up" >> $(BOOT_DIR)/config.txt; \
		echo "gpio=7,8=ip,pu" >> $(BOOT_DIR)/config.txt; \
		echo "\n# Set GPIO16 (Laser) to be an output set to 0" >> $(BOOT_DIR)/config.txt; \
		echo "gpio=16=op,dl" >> $(BOOT_DIR)/config.txt; \
		echo "\n# Set GPIO17,19,26,27 (Light) to be an output set to 0" >> $(BOOT_DIR)/config.txt; \
		echo "gpio=17,19,26,27=op,dl" >> $(BOOT_DIR)/config.txt; \
		echo "\n# Set GPIO18 (Stepper) to Alternative 5" >> $(BOOT_DIR)/config.txt; \
		echo "gpio=18=a5" >> $(BOOT_DIR)/config.txt; \
		echo "\n# Set GPIO20,21 (Emergency) to be an input and pull-up" >> $(BOOT_DIR)/config.txt; \
		echo "gpio=20,21=ip,pu" >> $(BOOT_DIR)/config.txt; \
		echo "\n# Set GPIO23 (Turntable) to be an output set to 1" >> $(BOOT_DIR)/config.txt; \
		echo "gpio=23=op,dh" >> $(BOOT_DIR)/config.txt; \
		echo "\n# Set GPIO24,25 (Button) to be respectively an output set to 0, and an input and pull-up" >> $(BOOT_DIR)/config.txt; \
		echo "gpio=24=op,dl" >> $(BOOT_DIR)/config.txt; \
		echo "gpio=25=ip,pu" >> $(BOOT_DIR)/config.txt; \
		echo "\ndtoverlay=stufa-pwm" >> $(BOOT_DIR)/config.txt; \
		echo "#dtoverlay=gpio-poweroff,gpiopin=24,active_low=0" >> $(BOOT_DIR)/config.txt; \
		echo "dtoverlay=i2c-rtc,ds3231" >> $(BOOT_DIR)/config.txt; \
	fi


xtools:
	cd $(XENOMAI_DIR); ./scripts/bootstrap
	# --with-core=cobalt --enable-debug=partial
	mkdir -p $(XBUILD_DIR)
	cd $(XBUILD_DIR); $(XENOMAI_DIR)/configure CFLAGS="-march=armv6zk -mfpu=vfp" LDFLAGS="-mtune=arm1176jzf-s" \
											   --build=i686-pc-linux-gnu --host=arm-linux-gnueabihf \
											   --with-core=cobalt \
											   CC=${CROSS_COMPILE}gcc LD=${CROSS_COMPILE}ld
											   # --enable-smp (Symmetric multiprocessing)
	make -C $(XBUILD_DIR) $(CORES)


xtools_install:
	mkdir -p $(XTOOLS_DIR)
	make -C $(XBUILD_DIR) $(CORES) install DESTDIR=$(XTOOLS_DIR)


clean_kernel:
	rm -rf $(KPACKAGE_DIR)
	rm -rf $(KERNEL_DIR)
	rm -rf $(KBUILD_DIR)


clean_xtools:
	rm -rf $(XTOOLS_DIR)
	rm -rf $(XBUILD_DIR)


reset_kernel:
	cd $(LINUX_DIR); git reset --hard; git clean -fxd :/;


reset_xtools:
	cd $(XENOMAI_DIR); git reset --hard; git clean -fxd :/;
