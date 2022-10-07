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

- SD card with DietPi fresh installed -> Will be patched with Xenomai, connect to PC before executing the next commands

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
