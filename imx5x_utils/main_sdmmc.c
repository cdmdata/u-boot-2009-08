//#define DEBUG
#include <stdarg.h>
#include "mx5x_common.h"
#include "mx5x_sdmmc.h"
extern const int prog_start;
extern const int free_space;

int load_block_of_file(struct info *pinfo)
{
	return common_load_block_of_file(&pinfo->c, 512);
}

int main(void)
{
	struct info info;
	unsigned root;
	struct partition *part;
	struct mbr *pmbr = (struct mbr *)&prog_start;
	struct boot *pboot = (struct boot *)&free_space;
	unsigned root_dir_entries;
	int i;
	int ret;
//	my_printf( "wait 1 second\n");
//	delayMicro(1000000);
//	my_printf( "\ninfo on stack %x\n", (uint32_t)&info);

	do {
		my_memset((unsigned char *)&info, 0, sizeof(struct info));
//		debug_dump(pmbr, 0, 1);	//dump 1st 512 bytes of memory
		if (mmc_init(&info.dev)) {
			delayMicro(1000000);
			continue;
		}
		ret = sd_read_blocks(&info.dev, 0, 1, pmbr);
		if (!GET_CODE(ret)) {
			debug_dump(pmbr, 0, 1);	//dump 1st 512 bytes of memory
			if (pmbr->signature == 0xaa55)
				break;
		}
		my_printf("not bootable, ret=%x\n", ret);
	} while (1);

	part = &pmbr->part[0];
	i = 4;
	info.fat_buf = pboot + 1;
	info.fat_block = 0;
	info.c.buf = pboot + 2;
	info.partition_start = 0;
	info.partition_size = 0x0fffffff;
	info.partition_end = 0x0fffffff;
	if (part->first.head > part->last.head) {
		my_memcpy((unsigned char *)pboot, (unsigned char *)pmbr, 512);
		goto skip_partition;
	}
	while (i) {
		if (1) {
			debug_pr("part %x: status %x, type %x\n", 5-i, part->status, part->type);
			debug_pr("1st  head %x sector %x cylinder %x\n",
					part->first.head, part->first.sector & 0x3f, part->first.cylinder | (part->first.sector & 0xc0) << 2);
			debug_pr("last head %x sector %x cylinder %x\n",
					part->last.head, part->last.sector & 0x3f, part->last.cylinder | (part->last.sector & 0xc0) << 2);
			debug_pr("  lba_start=%x, size=%x\n",
				part->lba_start[0] |(part->lba_start[1]<<16),
				part->size[0] |(part->size[1]<<16));
		}
		if (part->status &0x80)
			break;		/* break if active partition */
		part++;
		i--;
	}
	if (!i) {
		//no active partition, use 1st partition
		part = &pmbr->part[0];
	}
	info.partition_start = part->lba_start[0] | (part->lba_start[1] << 16);
	info.partition_size = part->size[0] | (part->size[1] << 16);
	info.partition_end = info.partition_start + info.partition_size;
	debug_pr("partition_start=%x, partition_size=%x\n", info.partition_start, info.partition_size);
	if (!info.partition_start)
		repeat_error("no partitions\n");

	sd_read_blocks(&info.dev, info.partition_start, 1, pboot);
	debug_dump(pboot, info.partition_start, 1);
skip_partition:
	root_dir_entries = GET_ODD_WORD(&pboot->max_root_dir_entries_L);
	info.sectorsPerCluster = pboot->sectorsPerCluster;
	info.fat_sector = info.partition_start + pboot->reservedSectors;
	debug_pr("root_dir_entries=%i, fat_sector=0x%x sectorsPerCluster=0x%x\n",
			root_dir_entries, info.fat_sector, info.sectorsPerCluster);
	if (root_dir_entries == 0) {
		/*
	 	 For FAT32
		 */
		info.fat32 = 1;
		info.cluster2StartSector = info.fat_sector + (pboot->numofFATs * pboot->sectorsPerFAT32);
		root = pboot->rootCluster32;
	} else {
		info.fat32 = 0;
		root = info.fat_sector + (pboot->numofFATs * pboot->sectorsPerFAT12);
		info.root_dir_blocks = ((root_dir_entries + 0xf) >> 4);
		info.cluster2StartSector = root + info.root_dir_blocks;
	}
	debug_pr("fat32=%i, root=0x%x\n", info.fat32, root);
	info.fname = (uint32_t *)uboot_name;
	ret = scan_chain(root, info.fat32, &info, scan_block_for_file);
	if (ret == STATUS_FILE_FOUND) {
		info.c.search = info.c.initial_buf = info.c.buf = (void *)0x90100000;
		ret = scan_chain(info.file_cluster, 1, &info, load_block_of_file);
		if (ret != STATUS_END_OF_CHAIN) {
			print_hex(ret, 8);
			repeat_error(" file load failed\n");
			return 0;
		}
		stop_clock();
		exec_program(&info.c, info.file_cluster);
	} else {
		repeat_error("U-BOOT.BIN not found\n");
	}
	return 0;
}
