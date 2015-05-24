SOURCES += CMSIS/vectors_stm32f4xx.c
SOURCES += CMSIS/system_stm32f4xx.c
SOURCES += CMSIS/_exit.c
SOURCES += CMSIS/_initialize_hardware.c
SOURCES += CMSIS/_reset_hardware.c
SOURCES += CMSIS/_sbrk.c
SOURCES += CMSIS/_startup.c
SOURCES += CMSIS/_syscalls.c
SOURCES += CMSIS/assert.c
SOURCES += CMSIS/exception_handlers.c

SOURCES += HAL/stm32f4xx_hal.c
#SOURCES += HAL/stm32f4xx_hal_adc.c
#SOURCES += HAL/stm32f4xx_hal_adc_ex.c
SOURCES += HAL/stm32f4xx_hal_can.c
SOURCES += HAL/stm32f4xx_hal_cortex.c
#SOURCES += HAL/stm32f4xx_hal_crc.c
#SOURCES += HAL/stm32f4xx_hal_cryp.c
#SOURCES += HAL/stm32f4xx_hal_cryp_ex.c
#SOURCES += HAL/stm32f4xx_hal_dac.c
#SOURCES += HAL/stm32f4xx_hal_dac_ex.c
#SOURCES += HAL/stm32f4xx_hal_dcmi.c
#SOURCES += HAL/stm32f4xx_hal_dma.c
#SOURCES += HAL/stm32f4xx_hal_dma2d.c
#SOURCES += HAL/stm32f4xx_hal_dma_ex.c
#SOURCES += HAL/stm32f4xx_hal_eth.c
#SOURCES += HAL/stm32f4xx_hal_flash.c
#SOURCES += HAL/stm32f4xx_hal_flash_ex.c
#SOURCES += HAL/stm32f4xx_hal_flash_ramfunc.c
SOURCES += HAL/stm32f4xx_hal_gpio.c
#SOURCES += HAL/stm32f4xx_hal_hash.c
#SOURCES += HAL/stm32f4xx_hal_hash_ex.c
#SOURCES += HAL/stm32f4xx_hal_hcd.c
#SOURCES += HAL/stm32f4xx_hal_i2c.c
#SOURCES += HAL/stm32f4xx_hal_i2c_ex.c
#SOURCES += HAL/stm32f4xx_hal_i2s.c
#SOURCES += HAL/stm32f4xx_hal_i2s_ex.c
#SOURCES += HAL/stm32f4xx_hal_irda.c
#SOURCES += HAL/stm32f4xx_hal_iwdg.c
#SOURCES += HAL/stm32f4xx_hal_ltdc.c
#SOURCES += HAL/stm32f4xx_hal_nand.c
#SOURCES += HAL/stm32f4xx_hal_nor.c
#SOURCES += HAL/stm32f4xx_hal_pccard.c
SOURCES += HAL/stm32f4xx_hal_pcd.c
SOURCES += HAL/stm32f4xx_hal_pcd_ex.c
SOURCES += HAL/stm32f4xx_hal_pwr.c
SOURCES += HAL/stm32f4xx_hal_pwr_ex.c
SOURCES += HAL/stm32f4xx_hal_rcc.c
SOURCES += HAL/stm32f4xx_hal_rcc_ex.c
#SOURCES += HAL/stm32f4xx_hal_rng.c
#SOURCES += HAL/stm32f4xx_hal_rtc.c
#SOURCES += HAL/stm32f4xx_hal_rtc_ex.c
#SOURCES += HAL/stm32f4xx_hal_sai.c
#SOURCES += HAL/stm32f4xx_hal_sd.c
#SOURCES += HAL/stm32f4xx_hal_sdram.c
#SOURCES += HAL/stm32f4xx_hal_smartcard.c
#SOURCES += HAL/stm32f4xx_hal_spi.c
#SOURCES += HAL/stm32f4xx_hal_sram.c
#SOURCES += HAL/stm32f4xx_hal_tim.c
#SOURCES += HAL/stm32f4xx_hal_tim_ex.c
#SOURCES += HAL/stm32f4xx_hal_uart.c
#SOURCES += HAL/stm32f4xx_hal_usart.c
#SOURCES += HAL/stm32f4xx_hal_wwdg.c
#SOURCES += HAL/stm32f4xx_ll_fmc.c
#SOURCES += HAL/stm32f4xx_ll_fsmc.c
#SOURCES += HAL/stm32f4xx_ll_sdmmc.c
SOURCES += HAL/stm32f4xx_ll_usb.c

SOURCES += USB/usbd_conf.c
SOURCES += USB/usbd_core.c
SOURCES += USB/usbd_ctlreq.c
SOURCES += USB/usbd_desc.c
SOURCES += USB/usbd_hid.c
SOURCES += USB/usbd_ioreq.c

SOURCES += stm32f4xx_it.c
SOURCES += main.c

CC=arm-none-eabi-gcc
CFLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fsingle-precision-constant -O3 -g3 -nostartfiles -Xlinker --gc-sections -ICMSIS -IHAL -IUSB -DSTM32F429xx -DUSE_HAL_DRIVER -DUSE_USB_HS -DUSE_USB_HS_IN_FS -TCMSIS/mem.ld -TCMSIS/sections.ld
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=stm32test.elf
BINARY=stm32test.bin

all: $(SOURCES) $(EXECUTABLE)
    
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(CFLAGS) $^ -o $@ -lm

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJECTS)

run:
	arm-none-eabi-strip $(EXECUTABLE)
	arm-none-eabi-objcopy -O binary $(EXECUTABLE) $(BINARY)
	st-link_cli -c SWD -P $(BINARY) 0x08000000 -w32 -Run
