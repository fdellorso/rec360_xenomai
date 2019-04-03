export LINUX_DIR			:= $(PWD)/kernel
export KERNEL_DIR			:= $(PWD)/kernel-output
export XENOMAI_DIR			:= $(PWD)/xenomai
export TOOLS_DIR			:= ${PWD}/xenomai-tools

export CORES				:= -j2

export ARCH					:= arm
export KERNEL				:= kernel
export CROSS_COMPILE		:= $(PWD)/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-

export BOOT_DIR				?= /media/fra/boot/
export ROOT_DIR				?= /media/fra/rootfs/


.PHONY: clean_kernel clean_tools clean_drivers kernel drivers tools tools_install config menuconfig patch_irq patch_xenomai copy_tosd config.txt cmdline.txt


kernel:
	mkdir -p $(KERNEL_DIR)
	make -C $(LINUX_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) $(CORES) zImage modules dtbs
	make -C $(LINUX_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) $(CORES) modules_install dtbs_install INSTALL_MOD_PATH=$(KERNEL_DIR) INSTALL_DTBS_PATH=$(KERNEL_DIR)
	mkdir -p $(KERNEL_DIR)/boot
	cd $(LINUX_DIR); ./scripts/mkknlimg $(LINUX_DIR)/arch/arm/boot/zImage $(KERNEL_DIR)/boot/$(KERNEL).img


patch_irq:
	cp -v kernel-patch/irq-bcm283* $(LINUX_DIR)/drivers/irqchip/


patch_xenomai: patch_irq
	$(XENOMAI_DIR)/scripts/prepare-kernel.sh --linux=$(LINUX_DIR) --arch=$(ARCH) --ipipe=./xenomai-patch/ipipe-core-4.14.85-arm-6.patch --verbose


config: patch_xenomai
	make -C $(LINUX_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) $(CORES) bcmrpi_defconfig
	cp kernel-patch/kernel-config $(LINUX_DIR)/.config


menuconfig:
	make -C $(LINUX_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) $(CORES) menuconfig


copy_tosd:
	sudo cp $(KERNEL_DIR)/*.dtb $(BOOT_DIR)
	sudo cp -rd $(KERNEL_DIR)/boot/* $(BOOT_DIR)
	sudo cp -dr $(KERNEL_DIR)/lib/* $(ROOT_DIR)lib/
	sudo cp -d $(KERNEL_DIR)/overlays/* $(BOOT_DIR)overlays/
	sudo cp -d $(KERNEL_DIR)/bcm* $(BOOT_DIR)


config.txt:
	@echo "kernel=kernel.img" >> $(BOOT_DIR)config.txt
	@echo "device_tree=bcm2708-rpi-0-w.dtb" >> $(BOOT_DIR)config.txt


cmdline.txt:
	@echo -n " dwc_otg.fiq_enable=0 dwc_otg.fiq_fsm_enable=0 dwc_otg.nak_holdoff=0" >> $(BOOT_DIR)cmdline.txt


drivers:
	make -C drivers/RTDM_gpio_estop
	# make -C drivers/RTDM_gpio_driver
	# make -C drivers/RTDM_gpio_sampling_driver
	# make -C drivers/RTDM_gpio_wave_driver
	# make -C drivers/RTDM_timer_driver


tools:
	cd $(XENOMAI_DIR); ./scripts/bootstrap --with-core=cobalt –enable-debug=partial
	cd $(XENOMAI_DIR); ./configure CFLAGS="-march=armv6zk -mfpu=vfp" LDFLAGS="-mtune=arm1176jzf-s" --build=i686-pc-linux-gnu --host=arm-linux-gnueabihf --with-core=cobalt --enable-smp CC=${CROSS_COMPILE}gcc LD=${CROSS_COMPILE}ld
	make -C $(XENOMAI_DIR) $(CORES)


tools_install:
	mkdir -p $(TOOLS_DIR)
	make -C $(XENOMAI_DIR) $(CORES) install DESTDIR=$(TOOLS_DIR)


clean_kernel:
	cd $(LINUX_DIR); git reset --hard; git clean -d -f;
	rm -rf $(KERNEL_DIR)


clean_tools:
	cd $(XENOMAI_DIR); git reset --hard; git clean -d -f;
	rm -rf $(TOOLS_DIR)


clean_drivers:
	make -C drivers/RTDM_gpio_estop clean
	# make -C drivers/RTDM_gpio_driver clean
	# make -C drivers/RTDM_gpio_wave_driver clean
	# make -C drivers/RTDM_timer_driver clean
	# make -C drivers/RTDM_gpio_sampling_driver clean
