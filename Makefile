export LINUX_DIR			:= $(PWD)/kernel/linux-4.14.85
export KERNEL_DIR			:= $(PWD)/rt-kernel
export XENOMAI_DIR			:= $(PWD)/xenomai-3.0.8
export TOOLS_DIR			:= ${PWD}/xenomai-tools

export CORES				:= -j2

export ARCH					:= arm
export KERNEL				:= kernel
export CROSS_COMPILE		:= $(PWD)/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-


# .PHONY: clean kernel drivers tools tools_install configure xenomai_kernel

kernel: output patch_xenomai config
	make -C $(LINUX_DIR) ARCH=$(ARCH) O=build/linux $(CORES) zImage modules dtbs
	make -C $(LINUX_DIR) ARCH=$(ARCH) O=build/linux $(CORES) modules_install INSTALL_MOD_PATH=$(KERNEL_DIR) INSTALL_DTBS_PATH=$(KERNEL_DIR)


output:
	mkdir -p $(KERNEL_DIR)


patch_irq:
	cp -v kernel/irq-bcm283* $(LINUX_DIR)/drivers/irqchip/


patch_xenomai: patch_irq
	$(XENOMAI_DIR)/scripts/prepare-kernel.sh --linux=$(LINUX_DIR) --arch=$(ARCH) --ipipe=./kernel/ipipe-core-4.14.85-arm-6.patch --verbose


config: output patch_xenomai
	make -C $(LINUX_DIR) ARCH=$(ARCH) O=$(KERNEL_DIR) bcmrpi_defconfig
	cp kernel/kernel-config $(KERNEL_DIR)/.config


menuconfig: output patch_xenomai
	make -C $(LINUX_DIR) ARCH=$(ARCH) O=$(KERNEL_DIR) menuconfig


drivers: kernel
	make -C drivers/RTDM_gpio_driver
	make -C drivers/RTDM_gpio_sampling_driver
	make -C drivers/RTDM_gpio_wave_driver
	make -C drivers/RTDM_timer_driver


tools:
	cd $(XENOMAI_DIR); ./scripts/bootstrap --with-core=cobalt –enable-debug=partial
	cd $(XENOMAI_DIR); ./configure CFLAGS="-march=armv6zk -mfpu=vfp" LDFLAGS="-mtune=arm1176jzf-s" --build=i686-pc-linux-gnu --host=arm-linux-gnueabihf --with-core=cobalt --enable-smp CC=${CROSS_COMPILE}gcc LD=${CROSS_COMPILE}ld
	make -C $(XENOMAI_DIR) $(CORES)


tools_install:
	mkdir -p $(TOOLS_DIR)
	make -C $(XENOMAI_DIR) $(CORES) install DESTDIR=$(TOOLS_DIR)


clean_kernel:
	make -C $(KERNEL_DIR) clean distclean


clean_tools:
	make -C $(XENOMAI_DIR) clean
	rm -rf $(TOOLS_DIR)


clean_drivers:
	make -C drivers/RTDM_gpio_driver clean
	make -C drivers/RTDM_gpio_wave_driver clean
	make -C drivers/RTDM_timer_driver clean
	make -C drivers/RTDM_gpio_sampling_driver clean

