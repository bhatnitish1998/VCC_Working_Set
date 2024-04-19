CFLAGS = -Wall -Wextra -Werror -O2 -Werror=array-bounds=0

all: guest.bin simple_kvm

simple_kvm: simple_kvm.c simple_kvm.h
	gcc -o $@ $<

guest.bin: guest.o
	ld -m elf_i386 --oformat binary -e main -Ttext 0 -o $@ $^
guest.o: guest.c
	$(CC) $(CFLAGS) -m32 -ffreestanding -fno-pic -c -o $@ $^

clean:
	rm simple_kvm guest.bin guest.o