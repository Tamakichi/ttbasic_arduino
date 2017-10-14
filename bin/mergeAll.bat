set bt=generic_boot20_pc13.bin
set btdir=.\PlusBootloader
mergesketch.exe %bt% ttbasic_NTSC.bin %btdir%\boot_ttbasic_NTSC.bin
mergesketch.exe %bt% ttbasic_OLED_SH1106_I2C.bin %btdir%\boot_ttbasic_OLED_SH1106_I2C.bin
mergesketch.exe %bt% ttbasic_OLED_SH1106_SPI.bin %btdir%\boot_ttbasic_OLED_SH1106_SPI.bin
mergesketch.exe %bt% ttbasic_OLED_SSD1306_I2C.bin %btdir%\boot_ttbasic_OLED_SSD1306_I2C.bin
mergesketch.exe %bt% ttbasic_OLED_SSD1306_SPI.bin %btdir%\boot_ttbasic_OLED_SSD1306_SPI.bin
mergesketch.exe %bt% ttbasic_Serial.bin %btdir%\boot_ttbasic_Serial.bin
mergesketch.exe %bt% ttbasic_TFT.bin %btdir%\boot_ttbasic_TFT.bin
