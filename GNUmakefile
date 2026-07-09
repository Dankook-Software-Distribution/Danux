MAKEFLAGS += -rR
.SUFFIXES:

CC := gcc
LD := ld

KERNEL   := build/kernel
ISO      := os.iso
ISO_ROOT := build/iso_root
LIMINE   := limine-binary/limine

CPPFLAGS := -Iinclude -MMD -MP

# -mcmodel=kernel pairs with the 0xffffffff80000000 base in linker.ld.
# The -mno-* flags keep the compiler from emitting FPU/SSE instructions,
# which would fault until the kernel enables those units itself.
CFLAGS := -g -O2 -pipe -std=gnu11 -Wall -Wextra \
	-ffreestanding \
	-fno-stack-protector \
	-fno-stack-check \
	-fno-PIC \
	-fno-asynchronous-unwind-tables \
	-m64 -march=x86-64 \
	-mno-80387 -mno-mmx -mno-sse -mno-sse2 \
	-mno-red-zone \
	-mcmodel=kernel

LDFLAGS := -nostdlib -static -z max-page-size=0x1000 -T linker.ld

SRCS := $(shell find init -name '*.c')
OBJS := $(patsubst %.c,build/%.o,$(SRCS))
DEPS := $(OBJS:.o=.d)

QEMU      := qemu-system-x86_64
QEMUFLAGS := -M q35 -m 512M -cdrom $(ISO) -boot d

.PHONY: all
all: $(ISO)

.PHONY: kernel
kernel: $(KERNEL)

$(KERNEL): $(OBJS) linker.ld
	@mkdir -p $(dir $@)
	$(LD) $(OBJS) $(LDFLAGS) -o $@

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

-include $(DEPS)

$(LIMINE):
	$(MAKE) -C limine-binary

$(ISO): $(KERNEL) limine.conf $(LIMINE)
	rm -rf $(ISO_ROOT)
	mkdir -p $(ISO_ROOT)/boot/limine $(ISO_ROOT)/EFI/BOOT
	cp $(KERNEL) $(ISO_ROOT)/boot/kernel
	cp limine.conf $(ISO_ROOT)/boot/limine/
	cp limine-binary/limine-bios.sys \
	   limine-binary/limine-bios-cd.bin \
	   limine-binary/limine-uefi-cd.bin $(ISO_ROOT)/boot/limine/
	cp limine-binary/BOOTX64.EFI limine-binary/BOOTIA32.EFI $(ISO_ROOT)/EFI/BOOT/
	xorriso -as mkisofs -R -r -J \
		-b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(ISO_ROOT) -o $@
	./$(LIMINE) bios-install $@

.PHONY: run
run: $(ISO)
	$(QEMU) $(QEMUFLAGS)

# No local display (e.g. over ssh): connect a VNC client to :5901.
.PHONY: run-vnc
run-vnc: $(ISO)
	$(QEMU) $(QEMUFLAGS) -vnc :1

.PHONY: clean
clean:
	rm -rf build $(ISO)

.PHONY: distclean
distclean: clean
	$(MAKE) -C limine-binary clean
