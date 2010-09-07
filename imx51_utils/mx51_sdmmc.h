struct sdmmc_dev {
	char mmc_ready;
	char f4BitMode;
	char bHighCapacity;
	char spare;
//	int startBlock = 0;
	unsigned char resp[20];
};

#define MAKE_RET_CODE(code, transfer_block_cnt) ((code << 16) | (transfer_block_cnt & 0xffff))
#define GET_CODE(ret) (ret >> 16)

int mmc_init(struct sdmmc_dev *pdev);
int sd_read_blocks(struct sdmmc_dev *pdev, uint block_num, uint block_cnt, void* buf);
int sd_write_blocks(struct sdmmc_dev *pdev, uint block_num, uint block_cnt, void* buf);
void stop_clock(void);
void repeat_error(char* s);

struct info {
	uint32_t partition_start;
	uint32_t partition_size;
	uint32_t partition_end;
	uint32_t cluster2StartSector;
	uint32_t sectorsPerCluster;
	uint32_t fat_sector;
	struct sdmmc_dev dev;
	struct common_info c;
	void * fat_buf;
	uint32_t fat_block;
	uint32_t file_cluster;
	uint32_t file_size;
	uint32_t *fname;
	uint32_t root_dir_blocks;
	uint8_t fat32;
};

struct chs {
	uint8_t head;		//0x00
	uint8_t sector;		//0x01	bit 0-5 sector, 6-7 high part of cylinder
	uint8_t cylinder;		//0x02	low part of cylinder
} __attribute__((__packed__));

//this structure starts on an odd halfword boundary
struct partition {
	uint8_t status;		//0x00	0x80 bootable (active)
	struct chs first;	//0x01	3 bytes
	uint8_t type;		//0x04	partition type
	struct chs last;	//0x05	3 bytes
	uint16_t lba_start[2];	//0x08 not uint32 because on an odd halfword boundary
	uint16_t size[2];	//0x0c not uint32 because on an odd halfword boundary
} __attribute__((__packed__));

struct mbr {
	unsigned char reserved[0x1be];
	struct partition part[4];	//0x1be - 16 bytes each
	uint16_t signature;		//0x1fe - 0xaa55, little endian
};
#define MBR_SIG			0xaa55

#define GET_ODD_WORD(p)	((p)[0] | ((p)[1] << 8))

struct boot {
	uint8_t	reserved[11];
	uint8_t	bytesPerSector_L;	//0x0b	usually 512
	uint8_t	bytesPerSector_H;	//0x0c
	uint8_t	sectorsPerCluster;	//0x0d	power of 2 from 1 to 128
	uint16_t reservedSectors;	//0x0e  usually 32 for fat 32
	uint8_t	numofFATs;		//0x10	(Almost always the value 2)
	uint8_t	max_root_dir_entries_L;	//0x11 0 for fat32
	uint8_t	max_root_dir_entries_H;	//0x12
	uint8_t	total_sectors_L;	//0x13 if 0 use 4 bytes at 0x20
	uint8_t	total_sectors_H;	//0x14
	uint8_t	media_descriptor;	//0x15
	uint16_t sectorsPerFAT12;	//0x16 Fat12/16
	uint8_t	reserved3[12];		//0x18
	/* Fat32 only */
	uint32_t sectorsPerFAT32;		//0x24
	uint32_t reserved4;		//0x28
	uint32_t rootCluster32;		//0x2c	root directory start
	uint16_t fs_infoSector;		//0x30	usually sector 1
	uint8_t	reserved5[0x2a];	//0x32
	uint8_t	bootCode_start[0x1a0];	//0x5c
	uint16_t reserved6;		//0x1fc
	uint16_t signature;		//0x1fe	- 0xaa55
};

#define STATUS_END_OF_CHAIN 1
#define STATUS_FILE_FOUND 2
#define STATUS_CLUSTER_NOT_VALID -1
#define STATUS_BLOCK_NOT_VALID -2

extern unsigned char uboot_name[];
int scan_block_for_file(struct info *pinfo);

typedef int (*work_func)(struct info *pinfo);
int scan_chain(unsigned cluster, unsigned chain, struct info *pinfo, work_func work);
