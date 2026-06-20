CXX      = arm-none-eabi-gcc

TARGET  = rtos
ELF     = $(TARGET).elf

SRCS    = startup.s main.cpp

CFLAGS  = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -nostdlib -nostartfiles -ffreestanding -g

LDFLAGS = -T linker.ld

all: $(ELF)

$(ELF): $(SRCS) linker.ld
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $(ELF)

qemu: $(ELF)
	qemu-system-arm -machine netduinoplus2 -nographic -kernel $(ELF)


clean:
	rm -f $(ELF)

