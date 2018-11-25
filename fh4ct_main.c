#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "t_datafile.h"
#include "t_readfile.h"

#define DUMP_MEM_SIZE           384
#define FREE_RTOS_HEAP_4_FLAG   0x80000000

uint32_t XX_BASE;
uint32_t X_xStart;
uint32_t X_pxEnd;
uint32_t X_xFreeBytesRemaining;
uint32_t X_xMinimumEverFreeBytesRemaining;
static uint8_t dump_mem[DUMP_MEM_SIZE * 1024];
uint32_t mem_total_size;

typedef struct
{
    uint32_t addr;
    uint32_t next;
    uint32_t size;
}mem_t;

void heap_load(void* pd, uint32_t len);
uint32_t heap_addr_to_u8(uint32_t addr);
uint32_t heap_addr_to_u16(uint32_t addr);
uint32_t heap_addr_to_u32(uint32_t addr);
uint32_t heap_get_FreeBytesRemaining(void);
uint32_t heap_get_MinimumEverFreeBytesRemaining(void);
uint32_t heap_get_pxEnd(void);
int heap_scan(void);

int elf_get_symbol_addr(const char* elf_file_name, const char* symbol_short_name, uint32_t* result_addr)
{
    FILE *stream;
    char buf[2048];
    char cmd_str[256];
    int count;
    int ret;
    printf("parse : %s ...", symbol_short_name);
    if((elf_file_name == 0) || (symbol_short_name == NULL))
    {
        return -1;
    }
    sprintf(cmd_str, "readelf -a %s | grep %s | cut -d ':' -f 2 | cut -d ' ' -f 2", elf_file_name, symbol_short_name);
    memset(buf, 0, sizeof(buf));
    stream = popen(cmd_str, "r");
    if(stream == NULL)
    {
        return -2;
    }
    count = fread(buf, sizeof(char), sizeof(buf), stream);
    if(count <= 0)
    {
        ret = -3;
        goto out;
    }
    sscanf(buf, "%x", result_addr);
    printf(" is 0x%08X\n", *result_addr);
    ret = 0;
    goto out;
out:
    pclose(stream);
    return ret;
}

int elf_parser(const char* elf_file_name)
{
    int ret;
    uint32_t ucHeap;
    ret = elf_get_symbol_addr(elf_file_name, "xStart", &X_xStart);
    if(ret < 0) return ret;
    ret = elf_get_symbol_addr(elf_file_name, "pxEnd", &X_pxEnd);
    if(ret < 0) return ret;
    ret = elf_get_symbol_addr(elf_file_name, "xFreeBytesRema", &X_xFreeBytesRemaining);
    if(ret < 0) return ret;
    ret = elf_get_symbol_addr(elf_file_name, "xMinimumEverFr", &X_xMinimumEverFreeBytesRemaining);
    if(ret < 0) return ret;
    ret = elf_get_symbol_addr(elf_file_name, "ucHeap", &ucHeap);
    if(ret < 0) return ret;
//"_start_data" is the symbol of .data section address
    ret = elf_get_symbol_addr(elf_file_name, "_start_data", &XX_BASE);
    if(ret < 0) return ret;
    return 0;
}

int main(int argc, char* argv[])
{
    int ret;
    DataFile_ypeDef mem_bin;
    const char* elf_name;
    if(argc != 3)
    {
        printf("use: %s [elf] [mem.bin]\n", argv[0]);
        return -1;
    }
    mem_bin.FileName = argv[2];
    ret = t_readfile(&mem_bin);
    if(ret < 0)
    {
        printf("open file: \"%s\" error!\n", argv[1]);
        return ret;
    }
    heap_load(mem_bin.pFileData, mem_bin.FileSize);
    printf("==========[ freeROS heap4 check tool ]==========\n");
    printf("=====[ size = 0x%08X = %09d bytes ]=====\n", mem_bin.FileSize, mem_bin.FileSize);
    elf_name = argv[1];
    ret = elf_parser(elf_name);
    if(ret < 0)
    {
        printf("elf parser fail!\n");
        return ret;
    }
    printf("    FreeBytesRemaining            = %d Bytes\n", heap_get_FreeBytesRemaining());
    printf("    MinimumEverFreeBytesRemaining = %d Bytes\n", heap_get_MinimumEverFreeBytesRemaining());
    heap_scan();
    return 0;
}

void heap_load(void* pd, uint32_t len)
{
    memcpy(dump_mem, pd, len);
}

uint32_t heap_get_FreeBytesRemaining(void)
{
    return *(uint32_t*)(dump_mem + X_xFreeBytesRemaining - XX_BASE);
}

uint32_t heap_get_MinimumEverFreeBytesRemaining(void)
{
    return *(uint32_t*)(dump_mem + X_xMinimumEverFreeBytesRemaining - XX_BASE);
}

uint32_t heap_get_pxEnd(void)
{
    return *(uint32_t*)(dump_mem + X_pxEnd - XX_BASE);
}

uint32_t heap_addr_to_u8(uint32_t addr)
{
    return *(uint8_t*)(dump_mem + addr - XX_BASE);
}

uint32_t heap_addr_to_u16(uint32_t addr)
{
    return *(uint16_t*)(dump_mem + addr - XX_BASE);
}

uint32_t heap_addr_to_u32(uint32_t addr)
{
    return *(uint32_t*)(dump_mem + addr - XX_BASE);
}
mem_t heap_get_item(uint32_t addr)
{
    mem_t ret;
    ret.addr = addr;
    ret.next = *(uint32_t*)(dump_mem + addr - XX_BASE);
    ret.size = *(uint32_t*)(dump_mem + addr + 4 - XX_BASE);
    return ret;
}

static mem_t mem_table[1024];
static uint32_t mem_table_count;
int heap_scan(void)
{
    mem_t cur;
    int freelist_count = 0;
    int usedlist_count = 0;
    int i;
    cur = heap_get_item(X_xStart);
    mem_total_size = heap_get_pxEnd() - cur.addr;
    printf("mem_total_size = %d Bytes\n", mem_total_size);
    do
    {
        cur = heap_get_item(cur.next);
        mem_table[mem_table_count++] = cur;
        if(cur.next == 0)
        {
            if(cur.addr == heap_get_pxEnd())
            {
                printf("pxEnd is match!\n");
            }
            else
            {
                printf("\033[31mpxEnd is not match! It should be 0x%08X but is 0x%08X ! maybe link is down!\033[0m\n", heap_get_pxEnd(), cur.addr);
                printf("cur: addr=%08X next=%08X size=%08X\n", cur.addr, cur.next, cur.size);
            }
            printf("scan ok!\n");
            break;
        }
        freelist_count++;
        printf("\033[32m[free %03d : addr = %08X size = %08X] [%02.3f\%]\033[0m",freelist_count, cur.addr, cur.size, cur.size * 100.0f / mem_total_size);
        if(cur.size == 0)
        {
            printf("size is zero !");
        }
        printf("\n");
        if(cur.next != heap_get_pxEnd())
        {
            uint32_t blank;
            mem_t use;
            blank = cur.next - cur.addr;
            use = heap_get_item(cur.addr + cur.size);
            mem_table[mem_table_count++] = use;
            do
            {
                usedlist_count++;
                if(use.size & FREE_RTOS_HEAP_4_FLAG)
                {
                    use.size -= FREE_RTOS_HEAP_4_FLAG;
                    printf("[used %03d : addr = %08X size = %08X] [%02.3f\%]", usedlist_count, use.addr, use.size, use.size * 100.0f / mem_total_size);
                }
                else
                {
                    printf("[used %03d : addr = %08X size = %08X] [%02.3f\%] [size alarm]", usedlist_count, use.addr, use.size, use.size * 100.0f / mem_total_size);
                }
                if(use.next != 0)
                {
                    printf("use.next is not null! ");
                }
                blank -= use.size;
                if(blank == 0)
                {
                {
                    printf("\n");
                    break;
                }
                else if(blank < 0)
                {
                    printf("blank alarm!\n");
                    break;
                }
                use = heap_get_item(use.addr + use.size);
                if(use.addr == cur.next)
                {
                    printf("\n");
                    break;
                }
                else if(use.addr > cur.next)
                {
                    printf("link alarm!\n");
                    break;
                }
                mem_table[mem_table_count++] = use;
                printf("\n");
            }while(blank > 0);
        }
        if(freelist_count >= 200)
        {
            printf("200 full!\n");
            break;
        }
    }while(1);
    uint32_t calc_total_mem = 0;
    mem_table_count--;
    printf("block count sum = %d\n", mem_table_count);
    for(i=0; i< mem_table_count; i++)
    {
        calc_total_mem += mem_table[i].size;
    }
    printf("block total size = %d\n", calc_total_mem);
    return 0;
}
