export LINUX_DIR			:= $(PWD)/kernel
export KBUILD_DIR			:= $(PWD)/kernel-build
export KERNEL_DIR			:= $(PWD)/kernel-output
export KPACKAGE_DIR			:= $(PWD)/kernel-package
export XENOMAI_DIR			:= $(PWD)/xenomai
export XBUILD_DIR			:= $(PWD)/xenomai-build
export TOOLS_DIR			:= ${PWD}/xenomai-tools

export CORES				:= -j8

export ARCH					:= arm
export KERNEL				:= kernel
export CROSS_COMPILE		:= $(PWD)/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-

export BOOT_DIR				?= /media/francesco/boot/
export ROOT_DIR				?= /media/francesco/rootfs/


.PHONY: kernel			kernel_package	\
		config 			menuconfig		\
		patch_irq		patch_xenomai 	\
		prepare_drivers	drivers			\
		overlays 						\
		tools 			tools_install	\
		clean_kernel 	clean_tools		\
		reset_kernel 	reset_tools


kernel:
	mkdir -p $(KERNEL_DIR)
	make -C $(LINUX_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(KBUILD_DIR) $(CORES) zImage modules dtbs
	make -C $(LINUX_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(KBUILD_DIR) $(CORES) modules_install dtbs_install INSTALL_MOD_PATH=$(KERNEL_DIR) INSTALL_DTBS_PATH=$(KERNEL_DIR)
	mkdir -p $(KERNEL_DIR)/boot
	cd $(LINUX_DIR); ./scripts/mkknlimg $(KBUILD_DIR)/arch/arm/boot/zImage $(KERNEL_DIR)/boot/$(KERNEL).img
	cp kernel-patch/Makefile $(KERNEL_DIR)/


kernel_package:
	mkdir -p $(KPACKAGE_DIR)
	cd $(KERNEL_DIR); tar czf $(KPACKAGE_DIR)/xenomai-kernel.tgz *


patch_irq:
	cp kernel-patch/irq-bcm283* $(LINUX_DIR)/drivers/irqchip/


patch_xenomai: patch_irq
	$(XENOMAI_DIR)/scripts/prepare-kernel.sh --linux=$(LINUX_DIR) --arch=$(ARCH) --ipipe=./xenomai-patch/ipipe-core-4.14.85-arm-6.patch --verbose


config: patch_xenomai
	mkdir -p $(KBUILD_DIR)
	make -C $(LINUX_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(KBUILD_DIR) $(CORES) bcmrpi_defconfig
	patch $(KBUILD_DIR)/.config kernel-patch/defconfig.patch


menuconfig:
	make -C $(LINUX_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) O=$(KBUILD_DIR) $(CORES) menuconfig


prepare_drivers:
	cp -r drivers/stufa $(LINUX_DIR)/drivers/
	@echo -n "obj-y += stufa/" >> $(LINUX_DIR)/drivers/Makefile
	patch $(LINUX_DIR)/drivers/Kconfig drivers/Kconfig.patch


drivers:
	cp drivers/RTDM_gpio_estop/rtdm-gpio-estop.c $(LINUX_DIR)/drivers/stufa/estop/


overlays:
	cp drivers/overlays/* $(LINUX_DIR)/arch/arm/boot/dts/overlays/


tools:
	cd $(XENOMAI_DIR); ./scripts/bootstrap --with-core=cobalt –enable-debug=partial
	mkdir -p $(XBUILD_DIR)
	cd $(XBUILD_DIR); $(XENOMAI_DIR)/configure CFLAGS="-march=armv6zk -mfpu=vfp" LDFLAGS="-mtune=arm1176jzf-s" --build=i686-pc-linux-gnu --host=arm-linux-gnueabihf --with-core=cobalt --enable-smp CC=${CROSS_COMPILE}gcc LD=${CROSS_COMPILE}ld
	make -C $(XBUILD_DIR) $(CORES)


tools_install:
	mkdir -p $(TOOLS_DIR)
	make -C $(XENOMAI_DIR) $(CORES) install DESTDIR=$(TOOLS_DIR)


clean_kernel:
	rm -rf $(KERNEL_DIR)
	rm -rf $(KBUILD_DIR)


clean_tools:
	rm -rf $(TOOLS_DIR)
	rm -rf $(XBUILD_DIR)


reset_kernel:
	cd $(LINUX_DIR); git reset --hard; git clean -fxd :/;


reset_tools:
	cd $(XENOMAI_DIR); git reset --hard; git clean -fxd :/;