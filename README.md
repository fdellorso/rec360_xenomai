# REC360 SW - Xenomai on Raspberry Pi 1B+ / Zero (W)

<h4>Versions:</h4>

- Raspberry Linux Kernel <b>4.19.127</b>

- Xenomai <b>3.2.1</b>

- Xenomai I-Pipe Patch <b>4.19.128-arm-9</b>

- Compiler ARM 32bit <b>8.3.0</b>

- Linux Distro <b>Ubuntu Desktop 20.04 LTS</b>


<h4>Requirements:</h4>

- Raspberry Pi GCC Cross-Compiler Toolchains, see https://github.com/abhiTronix/raspberry-pi-cross-compilers

- Version <b>8.3.0</b>, Target OS <b>Buster</b>, Target Platform <b>Pi 1/Zero</b> 

<h5>Fresh Install:</h5>

- SD card with a DietPi after First_boot and Second_boot procedures -> Will be patched with Xenomai, connect to PC before executing the next commands

<h5>Updating rec_360 Xenomai kernel:</h5>

- Connect SD card where rec_360 system is installed, and apply patch with the same commands(?)

## Command to compile

- `$ make patch_irq`
Patch Kernel BCM2835 IRQ Driver for Xernomai  

- `$ make patch_xenomai`
Patch Kernel with Xenomai I-Pipe

- `$ make config`
Load Kernel BCM2835 default settings

- `$ make menuconfig`
Open nCurses Kernel config dialog Box

- `$ make prepare_drivers`
Copy Drivers on Linux Build folder

- `$ make prepare_cmdlinetxt`
Prepare cmdline.txt file

- `$ make prepare_configtxt`
Prepare config.txt file

- `$ make kernel`
Compile Kernel

- `$ make kernel_copy2sd`
Copy Kernel files on SD

- `$ make clean_kernel`
Clear Kernel build folders
