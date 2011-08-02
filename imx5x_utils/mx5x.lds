/*
MEMORY
{
	iram_mem : ORIGIN = 0x1ffe0000, LENGTH = 128K
	sdram_mem : ORIGIN = 0x90000000, LENGTH = 512M
}

"ram_base" = ORIGIN(sdram_mem) + 0x4000;
*/
/* "ram_base" = ORIGIN(iram_mem) + 0x2000; */

SECTIONS {
	ENTRY(assem_start)
	.text load_addr + hdr_offset : {
		KEEP(*(.header_mem))
		KEEP(*(.dcd_mem))
		*(.text)
		*(.text.*)
		*(.gnu.linkonce.t.*)
/*
		*(.glue_7)
		*(.glue_7t)
*/
	}
	"prog_start" = ADDR(.text);
	"image_base" = ADDR(.text) - hdr_offset;
	. = ALIGN(0x20);

	.rodata : {
		*(.rodata.*)
	}
	.data : {
		*(.data.*)
		*(.gnu.linkonce.d.*)
		. = ALIGN(4);
	}
	"program_length" = . - image_base;
	"program_end" = .;

	.bss  (LOADADDR(.data) + SIZEOF ( .data) ) : {
		*(.bss) *(COMMON)
	}
	"bstart" = ADDR(.bss);
	"bend" = .;

	"end" = .;
	. = ALIGN(0x200);
	"free_space" = .;
	. = ALIGN(0x400);
	. += hdr_offset ? 0 : 0x400;
	"payload_offset" = . - image_base;
	.uboot_image : {
		*(.uboot_image)
	}
}
