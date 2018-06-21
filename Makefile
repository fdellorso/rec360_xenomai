export LINUX_DIR := $(PWD)/kernel/linux-4.9.51
export KERNEL_DIR := $(LINUX_DIR)/build/linux
export XENOMAI_DIR := $(PWD)/xenomai-3.0.6

.PHONY: clean kernel drivers tools tools_install configure xenomai_kernel

kernel: xenomai_kernel $(KERNEL_DIR)/.config
	make  -C $(LINUX_DIR) ARCH=arm O=build/linux -j4 bzImage modules dtbs
	make  -C $(LINUX_DIR) ARCH=arm O=build/linux modules_install INSTALL_MOD_PATH=MODULES


xenomai_kernel:
	$(XENOMAI_DIR)/scripts/prepare-kernel.sh --arch=arm --ipipe=./kernel/ipipe-core-4.9.51-arm-4.patch --linux=$(LINUX_DIR)


$(KERNEL_DIR)/.config:
	make -C $(LINUX_DIR) ARCH=arm O=build/linux multi_v7_defconfig
	cp -f kernel/kernel-config kernel/linux-4.9.51/build/linux/.config


configure: xenomai_kernel $(KERNEL_DIR)/.config
	make  -C $(LINUX_DIR) ARCH=arm O=build/linux menuconfig


drivers: kernel
	make -C drivers/RTDM_gpio_driver
	make -C drivers/RTDM_gpio_wave_driver
	make -C drivers/RTDM_timer_driver


tools:
	cd $(XENOMAI_DIR); ./configure --enable-smp --with-core=cobalt
	make -C $(XENOMAI_DIR) -j4


tools_install:
	make -C xenomai-3.0.6 install


clean:
	make -C $(KERNEL_DIR) clean distclean
	make -C drivers/RTDM_gpio_driver clean
	make -C drivers/RTDM_gpio_wave_driver clean
	make -C drivers/RTDM_timer_driver clean

