"C:\Program Files\Nordic Semiconductor\nrf-command-line-tools\bin\nrfjprog.exe" --program "C:\Users\Chip\AppData\Local\Arduino15\packages\adafruit\hardware\nrf52\1.6.0\bootloader\feather_nrf52840_express\feather_nrf52840_express_bootloader-0.8.0_s140_6.1.1.hex" --family NRF52 --chiperase --reset && nrfjprog --memwr 0xFF000 --val 0x01 && nrfjprog --reset --program build\adafruit.nrf52.feather52840\nRF52840_firmware.ino.hex --sectorerase --family NRF52 --verify --log