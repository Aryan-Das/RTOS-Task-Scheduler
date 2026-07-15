CXX      = arm-none-eabi-gcc

TARGET  = rtos
ELF     = $(TARGET).elf

SRCS    = startup.s main.cpp

CFLAGS  = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -nostdlib -nostartfiles -ffreestanding -g -fno-exceptions -fno-rtti 

LDFLAGS = -T linker.ld 

all: $(ELF)

$(ELF): $(SRCS) linker.ld
	$(CXX) $(CFLAGS) $(SRCS) $(LDFLAGS) -Wl,-Map=rtos.map -o $(ELF)


qemu: $(ELF)
	qemu-system-arm -machine netduinoplus2 -nographic -kernel rtos.elf 


flash: rtos.elf
	arm-none-eabi-objcopy -O binary rtos.elf rtos.bin
	st-flash write rtos.bin 0x8000000

flashswo: rtos.elf
	arm-none-eabi-objcopy -DUSE_SWO -O  binary rtos.elf rtos.bin
	st-flash write rtos.bin 0x8000000

clean:
	rm -f $(ELF)

