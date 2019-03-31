export LINUX_DIR			:= $(PWD)/kernel
export KERNEL_DIR			:= $(PWD)/kernel-output
export XENOMAI_DIR			:= $(PWD)/xenomai
export TOOLS_DIR			:= ${PWD}/xenomai-tools

export CORES				:= -j2

export ARCH					:= arm
export KERNEL				:= kernel
export CROSS_COMPILE		:= $(PWD)/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-


.PHONY: clean_kernel clean_tools clean_rtdm kernel rtdm tools tools_install menuconfig patch_xenomai

kernel: config
	make -C $(LINUX_DIR) ARCH=$(ARCH) O=$(KERNEL_DIR) $(CORES) zImage modules dtbs
	make -C $(LINUX_DIR) ARCH=$(ARCH) O=$(KERNEL_DIR) $(CORES) modules_install dtbs_install INSTALL_MOD_PATH=$(KERNEL_DIR) INSTALL_DTBS_PATH=$(KERNEL_DIR)


output:
	mkdir -p $(KERNEL_DIR)


patch_irq:
	cp -v kernel-patch/irq-bcm283* $(LINUX_DIR)/drivers/irqchip/


patch_xenomai: patch_irq
	$(XENOMAI_DIR)/scripts/prepare-kernel.sh --linux=$(LINUX_DIR) --arch=$(ARCH) --ipipe=./xenomai-patch/ipipe-core-4.14.85-arm-6.patch --verbose


config: output patch_xenomai
	make -C $(LINUX_DIR) ARCH=$(ARCH) O=$(KERNEL_DIR) bcmrpi_defconfig
	cp kernel-patch/kernel-config $(KERNEL_DIR)/.config


menuconfig: output patch_xenomai
	make -C $(LINUX_DIR) ARCH=$(ARCH) O=$(KERNEL_DIR) menuconfig


rtdm: kernel tools
	make -C drivers/RTDM_gpio_driver
	make -C drivers/RTDM_gpio_sampling_driver
	make -C drivers/RTDM_gpio_wave_driver
	make -C drivers/RTDM_timer_driver


tools:
	cd $(XENOMAI_DIR); ./scripts/bootstrap --with-core=cobalt –enable-debug=partial
	cd $(XENOMAI_DIR); ./configure CFLAGS="-march=armv6zk -mfpu=vfp" LDFLAGS="-mtune=arm1176jzf-s" --build=i686-pc-linux-gnu --host=arm-linux-gnueabihf --with-core=cobalt --enable-smp CC=${CROSS_COMPILE}gcc LD=${CROSS_COMPILE}ld
	make -C $(XENOMAI_DIR) $(CORES)


tools_install: tools
	mkdir -p $(TOOLS_DIR)
	make -C $(XENOMAI_DIR) $(CORES) install DESTDIR=$(TOOLS_DIR)


clean_kernel:
	make -C $(KERNEL_DIR) clean distclean


clean_tools:
	make -C $(XENOMAI_DIR) clean
	rm -rf $(TOOLS_DIR)


clean_rtdm:
	make -C drivers/RTDM_gpio_driver clean
	make -C drivers/RTDM_gpio_wave_driver clean
	make -C drivers/RTDM_timer_driver clean
	make -C drivers/RTDM_gpio_sampling_driver clean

