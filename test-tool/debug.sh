sudo sh -c "echo ttyAMA0 > /sys/module/kgdboc/parameters/kgdboc"
sudo sh -c "echo g > /proc/sysrq-trigger"
