  #!/bin/sh
# Requer devkitARM (devkitPro). Gera game.gba
set -e
arm-none-eabi-gcc -specs=gba.specs -mthumb -mthumb-interwork -O2 -Wall main.c -o game.elf
arm-none-eabi-objcopy -O binary game.elf super-ze.gba
gbafix super-ze.gba
echo "Pronto: super-ze.gba"
