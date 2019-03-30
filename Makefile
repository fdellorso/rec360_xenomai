export LINUX_DIR			:= $(PWD)/kernel/linux-4.14.85
export KERNEL_DIR			:= $(LINUX_DIR)/build/linux
export XENOMAI_DIR			:= $(PWD)/xenomai-3.0.8

export ARCH					:= arm
export CORES				:= -j2
export CROSS_COMPILE		:= $(PWD)/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-
export INSTALL_MOD_PATH		:= $(PWD)/
export INSTALL_DTBS_PATH	:= $(PWD)/

.PHONY: clean kernel drivers tools tools_install configure xenomai_kernel

kernel: xenomai_kernel $(KERNEL_DIR)/.config
	make  -C $(LINUX_DIR) ARCH=arm O=build/linux -j4 zImage modules dtbs
	make  -C $(LINUX_DIR) ARCH=arm O=build/linux modules_install INSTALL_MOD_PATH=MODULES


xenomai_kernel:
	$(XENOMAI_DIR)/scripts/prepare-kernel.sh --arch=arm --ipipe=./kernel/ipipe-core-4.14.85-arm-6.patch --linux=$(LINUX_DIR)


$(KERNEL_DIR)/.config:
	make -C $(LINUX_DIR) ARCH=arm O=build/linux bcmrpi_defconfig
	cp -f kernel/kernel-config kernel/linux-4.14.36/build/linux/.config


configure: xenomai_kernel $(KERNEL_DIR)/.config
	make  -C $(LINUX_DIR) ARCH=arm O=build/linux menuconfig


drivers: kernel
	make -C drivers/RTDM_gpio_driver
	make -C drivers/RTDM_gpio_sampling_driver
	make -C drivers/RTDM_gpio_wave_driver
	make -C drivers/RTDM_timer_driver


tools:
	cd $(XENOMAI_DIR); ./configure CFLAGS="-march=armv6zk -mfpu=vfp" LDFLAGS="-mtune=arm1176jzf-s" --build=i686-pc-linux-gnu --host=arm-linux-gnueabihf --with-core=cobalt --enable-smp CC=${CROSS_COMPILE}gcc LD=${CROSS_COMPILE}ld
	make -C $(XENOMAI_DIR) -j4


tools_install:
	make -C xenomai-3.0.7 install


clean:
	make -C $(KERNEL_DIR) clean distclean
	make -C drivers/RTDM_gpio_driver clean
	make -C drivers/RTDM_gpio_wave_driver clean
	make -C drivers/RTDM_timer_driver clean
	make -C drivers/RTDM_gpio_sampling_driver clean

