#include "libk.h"
#include "multiboot.h"
#include "interrupts.h"
#include "paging.h"


extern void load_idt(void);

void error(char *err, char errno) {
    kprintf("%4fs = %4fc\n", err, errno);
    for(;;);
}


void print_memory_area(struct memory_area *area) {
    kprintf("    start: %07x, length: %07x\n", area->base_addr, area->length);
}

void print_elf_section(struct elf_section *section) {
    kprintf("  addr: %07x, size: %07x, flags: %07x\n",
            section->addr, section->size, section->flags);
}


void enable_kernel_paging(struct multiboot_header *multiboot_info) {
    struct elf_section_tag *elf_sections = find_by_type(multiboot_info, ELF_TAG_TYPE);
    struct memory_map_tag *mmap_sections = find_by_type(multiboot_info, MMAP_TAG_TYPE);
    if (elf_sections == 0 || mmap_sections == 0) {
        error("could not find elf or mmap sections", 'S');
    }

    kprintf("memory areas:\n");
    itr_memory_mapped_locations(mmap_sections, print_memory_area);

    kprintf("kernel sections:\n");
    itr_elf_sections(elf_sections, print_elf_section);

    uint64_t kernel_start=0, kernel_end=0;
    kernel_start = ~kernel_start;

    void k_minmax(struct elf_section *area) {
        if (((uint64_t)(area->addr)) < kernel_start) {
            kernel_start = (uint64_t)(area->addr);
        }
        if (((uint64_t)(area->addr))+area->size > kernel_end) {
            kernel_end = (uint64_t)(area->addr);
            kernel_end += area->size;
        }
    }

    itr_elf_sections(elf_sections, k_minmax);
    kprintf("kernel start: %03x, kernel end: %03x\n", kernel_start, kernel_end);

    uint64_t multiboot_start=0, multiboot_end=0;
    multiboot_start = (uint64_t)multiboot_info;
    multiboot_end = multiboot_start + multiboot_info->total_size;
    kprintf("mboot start:  %03x, mboot end:  %03x\n", multiboot_start, multiboot_end);

    frame_allocator_t frame_allocator = 
        falloc_new(kernel_start, kernel_end, multiboot_start, multiboot_end, mmap_sections);
    
    remap_kernel(frame_allocator, multiboot_info);
}


int kmain(struct multiboot_header *multiboot_info) {
    kio_init();
    clear_screen();

    int ERROR = valid_multiboot(multiboot_info);
    if (ERROR) {
        error("multiboot information invalid", ERROR);
        return 1;
    }
    enable_kernel_paging(multiboot_info);

    for(;;);
}
