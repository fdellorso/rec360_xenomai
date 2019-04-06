.PHONY: all
all: copy config cmdline

.PHONY: copy
copy:
	sudo cp *.dtb /boot/
	cd boot; sudo cp -rd * /boot/
	cd lib; sudo cp -dr * /lib/
	cd overlays; sudo cp -d * /boot/overlays/
	sudo cp -d bcm* /boot/

.PHONY: config
config:
	@echo "kernel=kernel.img" >> /boot/config.txt
	# @echo "device_tree=bcm2708-rpi-0-w.dtb" >> /boot/config.txt
	@echo "device_tree=bcm2708-rpi-b-plus.dtb" >> /boot/config.txt
	@echo "dtoverlay=gpio-estop" >> /boot/config.txt

.PHONY: cmdline
cmdline:
	@echo -n "dwc_otg.fiq_enable=0 dwc_otg.fiq_fsm_enable=0 dwc_otg.nak_holdoff=0" >> /boot/cmdline.txt