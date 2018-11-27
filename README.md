# fh4ct
freeRTOS heap 4 caculate tool

fh4ct is a tool that help programmer debug freeRTOS's heap memory.

# How to use ?
You should dump your memory from mcu and provide orign elf file of your program,
fh4ct will search freeRTOS symbols, scan and parse the memory dump file to re-built heap 4 mem tree,
any little error in your memory found will be mark and help you check your memory and found each dynamic memory block.
