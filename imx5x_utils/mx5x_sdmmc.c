//#define DEBUG
//#include <stdio.h>
//#include <stdint.h>
//#include <stdlib.h>
//#include <stddef.h>
//#include <string.h>
//#include <legacy.h>
//#include "shell.h"
//#include <io.h>
//#include <mmio.h>
//#include <delay.h>
//#include <mem_map.h>
#include <stdarg.h>
#include "mx5x_common.h"
#include "mx5x_sdmmc.h"

#define MMC_BLOCK_SHIFT		9
#define MMC_BLOCK_SIZE		(1<<MMC_BLOCK_SHIFT)
#define MMC_DEFAULT_RCA		1

#define MMC_CMDAT_NORESP	(0 << 16)
#define MMC_CMDAT_R1		(2 << 16)
#define MMC_CMDAT_R3		(2 << 16)
#define MMC_CMDAT_R4		(2 << 16)
#define MMC_CMDAT_R5		(2 << 16)
#define MMC_CMDAT_R6		(2 << 16)
#define MMC_CMDAT_R7		(2 << 16)

#define MMC_CMDAT_R1B		(3 << 16)
#define MMC_CMDAT_R5B		(3 << 16)

#define MMC_CMDAT_R2		(1 << 16)

#define MMC_CMD_CCCE		(1 << 19)
#define MMC_CMD_CICE		(1 << 20)
#define MMC_CMD_DPS		(1 << 21)
#define MMC_XFR_READ	(1 << 4)
#define MMC_XFR_WRITE	0

#define MMC_CMD_RESET			((0 << 24) | MMC_CMDAT_NORESP)	// | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_CMD_SEND_OP_COND		((1 << 24) | MMC_CMDAT_R1B | MMC_CMD_CCCE | MMC_CMD_CICE)	//response?
#define MMC_CMD_ALL_SEND_CID		((2 << 24) | MMC_CMDAT_R2 | MMC_CMD_CCCE)
#define MMC_CMD_SET_RCA			((3 << 24) | MMC_CMDAT_R6 | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_CMD_SELECT_CARD		((7 << 24) | MMC_CMDAT_R1B | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_CMD_SEND_IF_COND		((8 << 24) | MMC_CMDAT_R7 | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_CMD_SEND_CSD		((9 << 24) | MMC_CMDAT_R2 | MMC_CMD_CCCE)
//#define MMC_CMD_SEND_CID		((10 << 24) | MMC_CMDAT_R2 | MMC_CMD_CCCE)
//#define MMC_CMD_STOP			((12 << 24) | MMC_CMDAT_R1B | MMC_CMD_CCCE | MMC_CMD_CICE)
//#define MMC_CMD_SEND_STATUS		((13 << 24) | MMC_CMDAT_R1 | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_CMD_SET_BLOCKLEN		((16 << 24) | MMC_CMDAT_R1 | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_CMD_READ_BLOCK		((17 << 24) | MMC_CMDAT_R1 | MMC_CMD_CCCE | MMC_CMD_CICE | MMC_CMD_DPS | MMC_XFR_READ)
//#define MMC_CMD_RD_BLK_MULTI		((18 << 24) | MMC_CMDAT_R1 | MMC_CMD_CCCE | MMC_CMD_CICE)
#define MMC_CMD_WRITE_BLOCK		((24 << 24) | MMC_CMDAT_R1 | MMC_CMD_CCCE | MMC_CMD_CICE | MMC_CMD_DPS | MMC_XFR_WRITE)
//#define MMC_CMD_APP_CMD		((55 << 24) | MMC_CMDAT_R1 | MMC_CMD_CCCE | MMC_CMD_CICE)

#define SD_APP_CMD_SET_BUSWIDTH		((6 << 24) | MMC_CMDAT_R1 | MMC_CMD_CCCE | MMC_CMD_CICE)
#define SD_STATUS			((13 << 24) | MMC_CMDAT_R1 | MMC_CMD_CCCE | MMC_CMD_CICE | MMC_CMD_DPS | MMC_XFR_READ)

#define SD_APP_CMD41			((41 << 24) | MMC_CMDAT_R3)	/* 0x29 */
#define SD_APP_CMD55			((55 << 24) | MMC_CMDAT_R1 | MMC_CMD_CCCE | MMC_CMD_CICE)	/* 0x37 */



#define SD_DSADDR	0x00	//4 bytes, DMA system address
#define SD_BLKATTR	0x04	//4 bytes, block attributes
#define SD_CMDARG	0x08	//4 bytes, command argument
#define SD_XFRTYPE	0x0c	//4 bytes, transfer type
#define SD_CMDRSP	0x10	//16 bytes, command response
#define SD_DATAPORT	0x20	//4 bytes, data buffer access port
#define SD_PRSSTATE	0x24	//4 bytes, present state
#define PRSSTATE_SDSTB	(1 << 3)	//clock stable

#define SD_PROCTL	0x28	//4 bytes, protocol control
#define SD_SYSCTL	0x2c	//4 bytes, system control
#define SYSCTL_CLKEN			(1 << 3)
#define SYSCTL_HCKEN			(1 << 1)
#define SYSCTL_SLOW_CLOCK		(0x8000)	/* slow clock = (54M) divide by (256 * 1) = 214K (elvis 270)*/
#define SYSCTL_CLOCK_MASK		(0xfff0)
#define SYSCTL_CLOCK(a,b)		((a<<8)|(b<<4))
#define SYSCTL_DATA_TIMEOUT		(0x0e << 16)
#define SYSCTL_DATA_TIMEOUT_MASK	(0x0f << 16)
#define SYSCTL_RSTA			(1 << 24)
#define SYSCTL_RSTC			(1 << 25)
#define SYSCTL_RSTD			(1 << 26)
#define SYSCTL_INITA			(1 << 27)

#define SD_IRQSTAT	0x30	//4 bytes, irq status
#define SD_IRQSTATEN	0x34	//4 bytes, irq status enable
#define SD_IRQSIGEN	0x38	//4 bytes, irq signal enable
#define SD_AUTOC12ERR	0x3c	//4 bytes, Auto CMD12 Error status
#define SD_HOSTCAP	0x40	//4 bytes, host controller capabilities
#define SD_WML		0x44	//4 bytes, watermark level
#define SD_FEVT		0x50	//4 bytes, force event
#define SD_ADMAES	0x54	//4 bytes, ADMA error status
#define SD_ADSADDR	0x58	//4 bytes, ADMA system address
#define SD_VENDOR	0xc0	//4 bytes, vendor specific register
#define SD_HOSTVER	0xfc	//4 bytes, host controller version




void stop_clock(struct sdmmc_dev *pdev)
{
	IO_MOD(pdev->base, SD_SYSCTL, SYSCTL_CLKEN, 0);	/* stop clock to SDCard*/
	delayMicro(10);
}
static void clock_off(struct sdmmc_dev *pdev)
{
	IO_MOD(pdev->base, SD_SYSCTL, 0xf, 0);	/* disable internal clock too */
	delayMicro(10);
}

static void start_clock(struct sdmmc_dev *pdev)
{
	uint clkctl;
	do {
		clkctl = IO_READ(pdev->base, SD_PRSSTATE);
	} while (!(clkctl & PRSSTATE_SDSTB));
	IO_MOD(pdev->base, SD_SYSCTL, 0, SYSCTL_CLKEN);	/* enable clock */
}

static uint8_t *mmc_cmd(struct sdmmc_dev *pdev, uint cmd, uint arg)
{
	uint base = pdev->base;
	uint *prsp = (uint *)pdev->resp;
	uint status;

//	stop_clock(pdev);
	IO_WRITE(base, SD_CMDARG, arg);
	IO_WRITE(base, SD_IRQSTAT, 0xffffffff);
	IO_WRITE(base, SD_IRQSTATEN, (cmd & MMC_CMD_DPS) ? 0xff0033 : 0xff0001);
	start_clock(pdev);
//	debug_pr("PRSSTATE=%x\n", IO_READ(base, SD_PRSSTATE));
	IO_WRITE(base, SD_XFRTYPE, cmd);
	status = 0;
	do {
		uint stat = IO_READ(base, SD_IRQSTAT);
		if (stat != status) {
			status = stat;
//			debug_pr("IRQSTAT=%x cmd=%x, arg=%x\n", status, cmd, arg);
		}
	} while (!(status & 0xff0001));

	if (status & 0xff0000) {
		debug_pr("IRQSTAT %x: cmd: 0x%x, arg: 0x%08x, PRSSTATE:%x\n",
				status, cmd, arg, IO_READ(base, SD_PRSSTATE));
		if (!(status & 1)) {
			IO_WRITE(base, SD_IRQSTAT, status);
			IO_WRITE(base, SD_SYSCTL, SYSCTL_RSTC | SYSCTL_RSTD);	//reset cmd
			return 0;
		}
		if (status & 0x10000) {
			if (cmd & MMC_CMD_DPS)
				IO_WRITE(base, SD_SYSCTL, SYSCTL_RSTD);	//reset data
			return 0;	/* timeout */
		}
	}

	switch ((cmd >> 16) & 0x3) {
		case 2:	//(R3,R4) no CRC, (R1,R5,R6,R7) CRC
		case 3:	//R1B, R5B
			//39:8 -> resp[31:0]
			*prsp = IO_READ(base, SD_CMDRSP);
//			debug_pr("MMC resp = %08x\n", prsp[0]);
			break;

		case 1:	//R2
			//133:8 -> resp[125:0]
			prsp[0] = IO_READ(base, SD_CMDRSP + 0);
			prsp[1] = IO_READ(base, SD_CMDRSP + 4);
			prsp[2] = IO_READ(base, SD_CMDRSP + 8);
			prsp[3] = IO_READ(base, SD_CMDRSP + 0x0c);
//			debug_pr("MMC resp = %08x %08x %08x %08x\n", prsp[0], prsp[1], prsp[2], prsp[3]);
			break;
		//case 0 - no response to command is default
		default:
			*prsp = 0;
	}
	return pdev->resp;
}

static void mmc_setblklen(struct sdmmc_dev *pdev, uint blklen)
{
	static uint prevLen = ~0;
	if (blklen != prevLen) {
		/* set block len */
		mmc_cmd(pdev, MMC_CMD_SET_BLOCKLEN, blklen);
		prevLen = blklen ;
	}
}

static int mmc_ReadFifo(struct sdmmc_dev *pdev, uint * pDst, uint len)
{
	uint status = 0;
	uint base = pdev->base;
	do {
		status = IO_READ(base, SD_IRQSTAT);
		if (status & 0xff0022)
			break;
		delayMicro(10);
	} while (1);

	do {
		*pDst++ = IO_READ(base, SD_DATAPORT);
		if (len <= 4)
			break;
		len -= 4;
	} while (1);

	while (!(status & 0xff0002)) {
		//wait for transfer complete
		status = IO_READ(base, SD_IRQSTAT);
	}
	IO_WRITE(base, SD_IRQSTAT, 0xff);
	if (status & 0xff0000) {
		IO_WRITE(base, SD_SYSCTL, SYSCTL_RSTD);	//reset data
		IO_WRITE(base, SD_IRQSTAT, 0xff0000);
		my_printf( "IRQSTAT %x\n", status);
		return -1;
	}
	return 0;
}

static int mmc_WriteFifo(struct sdmmc_dev *pdev, uint * pDst, uint len)
{
	uint status = 0;
	uint base = pdev->base;
	do {
		status = IO_READ(base, SD_IRQSTAT);
		if (status & 0xff0012)
			break;
		delayMicro(10);
	} while (1);

	do {
		IO_WRITE(base, SD_DATAPORT, *pDst++);
		if (len <= 4)
			break;
		len -= 4;
	} while (1);

	while (!(status & 0xff0002)) {
		//wait for transfer complete
		status = IO_READ(base, SD_IRQSTAT);
	}
	IO_WRITE(base, SD_IRQSTAT, 0xff);
	if (status & 0xff0000) {
		IO_WRITE(base, SD_SYSCTL, SYSCTL_RSTD);	//reset data
		IO_WRITE(base, SD_IRQSTAT, 0xff0000);
		debug_pr( "IRQSTAT %x\n", status);
		return -1;
	}
	return 0;
}

static int mmc_block_read(struct sdmmc_dev *pdev, unsigned char *dst, uint blockNum, uint len)
{
	int ret;
	uint base = pdev->base;
	unsigned char *resp;
	if (len == 0)
		return 0;

//	debug_pr("%s dst=%08x blk=%08x len=%08x\n", __func__, (uint)dst, blockNum, len);
	mmc_setblklen(pdev, len);

	/* send read command */
//	stop_clock(pdev);
	IO_MOD(base, SD_SYSCTL, (0x0f << 16), (0x0e << 16));
	IO_WRITE(base, SD_BLKATTR, (1 << 16) | len);
//	blockNum += startBlock;
	if (!pdev->bHighCapacity) blockNum <<= MMC_BLOCK_SHIFT;
	resp = mmc_cmd(pdev, MMC_CMD_READ_BLOCK, blockNum);
	if (resp)
		ret = mmc_ReadFifo(pdev, (unsigned int*)dst,len);
	else
		ret = -1;
//	stop_clock(pdev);
	return ret;
}

static int mmc_block_write(struct sdmmc_dev *pdev, uint blockNum, unsigned char *src, int len)
{
	int ret;
	uint base = pdev->base;
	unsigned char *resp;
	if (len == 0)
		return 0;

//	debug_pr("%s blockNum=%08x src=%08x len=%08x\n", __func__, blockNum, (uint)src, len);
	mmc_setblklen(pdev, len);

	/* send write command */
//	stop_clock(pdev);
	IO_MOD(base, SD_SYSCTL, SYSCTL_DATA_TIMEOUT_MASK, SYSCTL_DATA_TIMEOUT);
	IO_WRITE(base, SD_BLKATTR, (1 << 16) | len);
//	blockNum += startBlock;
	if (!pdev->bHighCapacity) blockNum <<= MMC_BLOCK_SHIFT;
	resp = mmc_cmd(pdev, MMC_CMD_WRITE_BLOCK, blockNum);
	if (resp)
		ret = mmc_WriteFifo(pdev, (unsigned int*)src, len);
	else
		ret = -1;
//	stop_clock(pdev);
	return ret;
}

static unsigned char* mmc_reset(struct sdmmc_dev *pdev)
{
	uint base = pdev->base;
	unsigned char *resp;
	uint sysctl;
	int i=0;
	do {
		clock_off(pdev);
		IO_WRITE(base, SD_SYSCTL, SYSCTL_SLOW_CLOCK | SYSCTL_RSTA | SYSCTL_RSTC | SYSCTL_RSTD);	//reset cmd
		do {
			sysctl = IO_READ(base, SD_SYSCTL);
		} while (sysctl & SYSCTL_RSTA);
		IO_WRITE(base, SD_SYSCTL, SYSCTL_INITA | SYSCTL_DATA_TIMEOUT | SYSCTL_SLOW_CLOCK);
		IO_WRITE(base, SD_PROCTL, (2<<4));	//little endian, 1 bit mode
		IO_WRITE(base, SD_WML, (16<<24)|(128<<16)|(16<<8)|(128<<0));	//128 word watermark
		start_clock(pdev);
		delayMicro(100);
		resp = mmc_cmd(pdev, MMC_CMD_RESET, 0);	//reset
		if (resp) break;
//		debug_pr( "%s:error\n", __func__);
		i++;
		if (i>=10) break;
	} while (1);
	delayMicro(50);
	pdev->f4BitMode = 0;
	pdev->bHighCapacity = 0;
	return resp;
}

#define ENODEV 0x30
static int test_for_SDCard(struct sdmmc_dev *pdev)
{
	unsigned char *resp ;
	int highCapacityAllowed = 0;
	int bRetry=0;
	resp = mmc_reset(pdev);
	resp = mmc_cmd(pdev, MMC_CMD_SEND_IF_COND, (1<<8)|0xaa);	//request 2.7-3.6 volts
	if (resp) {
		highCapacityAllowed = 1<<30;
//		debug_pr("cmd8 response: %2x %2x %2x %2x %2x\n", resp[0], resp[1], resp[2], resp[3], resp[4]);
	}

	do {
		resp = mmc_cmd(pdev, SD_APP_CMD55, 0);
		if (!resp) {
			resp = mmc_reset(pdev);
			resp = mmc_cmd(pdev, SD_APP_CMD55, 0);
			if (!resp ) {
				if (!bRetry) {
//					debug_pr("%s Err1\n", __func__);
					return -ENODEV ;
				}
			}
		}

		if (resp) {
			//bit 21 means 3.3 to 3.4 Volts
			resp = mmc_cmd(pdev, SD_APP_CMD41, (1<<21)|highCapacityAllowed);
			if ( !resp ) {
				if (!bRetry) {
//					debug_pr("%s Err2\n", __func__);
					return -ENODEV ;
				}
			} else {
				//response R3, returns 32 bit OCR register
				uint ocr = *((uint *)resp);
				if (ocr & 0x80000000) {
					if (highCapacityAllowed && (ocr & 0x40000000))
						pdev->bHighCapacity = 1;
					break;
				}
			}
		}
		bRetry++;
		delayMicro(1000);
		if (bRetry > 1000) {
			if (resp)
//				debug_pr("response %02x %02x %02x %02x\n", resp[0],resp[1],resp[2],resp[3]);
			return -ENODEV;
		}
	} while (1);
	return 0 ;
}

struct mmc_cid {
	uint16_t mdt_month:4,
		mdt_year:8,
		mdt_reserved:4;
	uint32_t psn;
	uint8_t	prv_fw:4,
		prv_hw:4;
	uint8_t pnm[5];
	uint8_t oid[2];
	uint8_t mid;
};

struct mmc_csd_v1_0 {
	uint8_t reserved1:2,
		file_format:2,
		temp_write_protection:1,
		perm_write_protection:1,
		copy_otp:1,
		file_format_grp:1;
	uint16_t reserved2:5,
		write_bl_partial:1,
		write_bl_len:4,
		r2w_factor:3,
		reserved3:2,
		wr_grp_enable:1;
	uint64_t wr_grp_size:7,
		sector_size:7,
		erase_blk_en:1,
		c_size_mult:3,
		vdd_w_curr_max:3,
		vdd_w_curr_min:3,
		vdd_r_curr_max:3,
		vdd_r_curr_min:3,
		c_size:12,
		reserved4:2,
		dsr_imp:1,
		read_blk_misalign:1,
		write_blk_misalign:1,
		read_bl_partial:1,
		read_bl_len:4,
		ccc:12;
	uint8_t	tran_speed;
	uint8_t	taac;
	uint8_t	nsac;
	uint8_t	reserved5:6,
		csd_structure:2;
};

static int mmc_init__(struct sdmmc_dev *pdev, int force_1wire)
{
	uint base = pdev->base;
	int retries;
	int rc = -ENODEV;
	unsigned short rca = MMC_DEFAULT_RCA;
	char isSD = 0 ;
	char allowed4bit=0;
	unsigned char *resp;
	struct mmc_cid *cid;
	struct mmc_csd_v1_0 *csd;
	unsigned c_size = 0;

	pdev->mmc_ready = 0;
	pdev->f4BitMode = 0;	//default to 1bit mode
	pdev->bHighCapacity = 0;

	clock_off(pdev);
	IO_WRITE(base, SD_SYSCTL, SYSCTL_SLOW_CLOCK);	/* slowest clock */
	debug_pr("HOSTVER %x\n", IO_READ(base, SD_HOSTVER));
	if( 0 == test_for_SDCard(pdev) ) {
		debug_pr( "SD card detected!\n" );
		isSD = 1;
	} else {
		isSD = 0;
		/* reset */
		resp = mmc_reset(pdev);
		resp = mmc_cmd(pdev, MMC_CMD_SEND_OP_COND, 0x00ffc000);
		if( 0 == resp ) {
			debug_pr( "MMC CMD1 error\n" );
			return -1 ;
		}
		retries = 0 ;
		do {
			delayMicro(100);
			resp = mmc_cmd(pdev, MMC_CMD_SEND_OP_COND,  0x00ffff00);
			retries++ ;
		} while( resp && ( 0 == ( resp[3] & 0x80 ) ) );

		if (!resp ) {
			debug_pr( "MMC CMD1 error2\n" );
			return -1 ;
		}

		do {
			delayMicro(100);
			resp = mmc_cmd(pdev, MMC_CMD_SEND_OP_COND, 0x00ffff00);
			retries++ ;
		} while( resp && ( 0 != ( resp[3] & 0x80 ) ) );

//		debug_pr( "after busy: %s, %d retries\n", resp ? "have INIT response" : "no INIT response", retries );
	}

	/* try to get card id */
	resp = mmc_cmd(pdev, MMC_CMD_ALL_SEND_CID, 0);
	if( !resp ) {
		debug_pr( "Bad CMDAT_R2 response\n" );
		return -1 ;
	}
	/* TODO configure mmc driver depending on card attributes */
	cid = (struct mmc_cid *)resp;

	debug_pr("MMC found. Card desciption is:\n");
//	debug_pr("Manufacturer ID = %02x oid=%02x%02x\n", cid->mid, cid->oid[1], cid->oid[2]);
//	debug_pr("HW/FW Revision = %x %x\n",cid->prv_hw, cid->prv_fw);
	cid->oid[0] = 0;	//terminate string
//	debug_pr("Product Name = %s\n",cid->pnm);
//	debug_pr("Serial Number = %08x\n", cid->psn);
//	debug_pr("Month = %d\n",cid->mdt_month);
//	debug_pr("Year = %d\n",1997 + cid->mdt_year);

	/* MMC exists, get CSD too */
	resp = mmc_cmd(pdev, MMC_CMD_SET_RCA, MMC_DEFAULT_RCA<<16);

	if (!resp) {
		debug_pr( "no SET_RCA response\n" );
		return -1 ;
	}
	rca = (isSD)? ((uint16_t*)resp)[1] : MMC_DEFAULT_RCA ;


	stop_clock(pdev);
	clock_off(pdev);
//	IO_MOD(base, SD_SYSCTL, SYSCTL_CLOCK_MASK, SYSCTL_CLOCK(1,1));	/* 54/2/2 = 13.5 Mhz clock */
	IO_MOD(base, SD_SYSCTL, SYSCTL_CLOCK_MASK, SYSCTL_CLOCK(0x8,0));	/* 54/16/1 = 3.375 Mhz clock */

	resp = mmc_cmd(pdev, MMC_CMD_SELECT_CARD, rca<<16);
	if (!resp) {
		debug_pr( "Error selecting RCA %x\n", rca );
		return -1 ;
	}

	resp = mmc_cmd(pdev, MMC_CMD_SELECT_CARD, 0);
	if (!resp) {
		// this is normal
	}

	resp = mmc_cmd(pdev, MMC_CMD_SEND_CSD, rca<<16);
	if (!resp) {
		debug_pr( "Error reading CSD\n" );
		return -1 ;
	}

	csd = (struct mmc_csd_v1_0 *)resp;
	c_size = csd->c_size;
	rc = 0;


	resp = mmc_cmd(pdev, MMC_CMD_SELECT_CARD, rca<<16);
	if (!resp) {
		debug_pr( "Error selecting RCA %x\n", rca );
		return -1 ;
	}

//	debug_pr("isSD %x\n", isSD);
	if (isSD) {
		int success = 0;
		int busWidthMode = (force_1wire) ? 0 : 2;	//2 means 4 bit wide
		allowed4bit = 1;
		resp = mmc_cmd(pdev, SD_APP_CMD55, rca<<16);
		if (resp) {
			resp = mmc_cmd(pdev, SD_APP_CMD_SET_BUSWIDTH, busWidthMode);
		}
		if (resp){
			pdev->f4BitMode = (busWidthMode)? 1 : 0;	//switch to 4bit mode
			IO_MOD(base, SD_PROCTL, 6, (pdev->f4BitMode)? 2 : 0);	//maybe switch to 4 bit mode
		}
		resp = mmc_cmd(pdev, SD_APP_CMD55, rca<<16);
//		debug_pr("resp %x\n", resp);
		if (resp) {
			/* send read command */
			unsigned char buf[64];
//			stop_clock(pdev);
			IO_MOD(base, SD_SYSCTL, SYSCTL_DATA_TIMEOUT_MASK, SYSCTL_DATA_TIMEOUT);
			IO_WRITE(base, SD_BLKATTR, (1 << 16) | 64);
			resp = mmc_cmd(pdev, SD_STATUS, 0); 	//get SD Status
			if (!resp)
				debug_pr( "Error sending SD_STATUS command\n");
			else {
				int r;
//				debug_pr("resp %x\n", resp);
				r = mmc_ReadFifo(pdev, (unsigned int*)buf,64);
//				debug_pr("buf[0]=%2x;  busWidthMode=%x\n", buf[0], busWidthMode);
				if (r) {
					debug_pr( "Error reading SD_STATUS data\n");
				} else if ((buf[0] >> 6)== busWidthMode) {
					success = 1;
				} else
					debug_pr( "SD_STATUS data unexpected\n");
			}
		}
		if (!success) {
			debug_pr( "!!!!!Error selecting bus width of %d bits\n",(busWidthMode)? 4 : 1);
			resp = mmc_cmd(pdev, SD_APP_CMD55, rca<<16);
			if (resp)
				resp = mmc_cmd(pdev, SD_APP_CMD_SET_BUSWIDTH, 0);
			if (!resp)
				debug_pr( "Error reselecting 1 bit mode\n");
			pdev->f4BitMode = 0;	//back to 1 bit mode
			IO_MOD(base, SD_PROCTL, 6, 0);	//back to 1 bit mode
		}
	}
	pdev->mmc_ready = 1;
	if (pdev->f4BitMode) {
		my_printf( "4-bit\n");
	} else if (allowed4bit) {
		my_printf( "!!! 1-bit\n");
	} else {
		my_printf( "1-bit\n");
	}

	return rc;
}



int mmc_init(struct sdmmc_dev *pdev)
{
	uint base = pdev->base;
	int rc;
	iomuxc_setup_mmc();

	if (0) {
		uint prsstate;
		prsstate = IO_READ(base, SD_PRSSTATE);
		debug_pr("\nPRSSTATE=%x\n", prsstate);
		if (!(prsstate & (1 << 18))) {
			my_printf("No sdcard\n");
			return -1;
		}
	} else {
		uint d = mmc_read_cd();
		if (d) {
			my_printf("%x, No sdcard\n", d);
			return -1;
		} else {
			debug_pr("sdcard detected\n");
		}
	}
	rc = mmc_init__(pdev, 0);
	if (rc) {
//		rc = mmc_init__(pdev, 1);
	}
//	stop_clock(pdev);
	return rc;
}

//Out: (val &0xffff) = # of sectors transferred, (val >> 16) = error code
int sd_read_blocks(struct sdmmc_dev *pdev, uint block_num, uint block_cnt, void* buf)
{
	int transfer_cnt = 0;
	int code = 0;
	int retry = 0;
	debug_pr("%s dst=%08x blk=%08x cnt=%08x stack=%x\n", __func__, (uint)buf, block_num, block_cnt, (uint)&code);
	while (block_cnt) {
		code = mmc_block_read(pdev, buf, block_num, 512);
		if (code) {
			retry++;
			if (retry > 10)
				break;
			mmc_init(pdev);
		} else {
			block_cnt--;
			transfer_cnt++;
			buf = ((uint8_t *)buf) + 512;
			block_num++;
			retry = 0;
		}
	}
	return MAKE_RET_CODE(code, transfer_cnt);
}

//Out: (val &0xffff) = # of sectors transferred, (val >> 16) = error code
int sd_write_blocks(struct sdmmc_dev *pdev, uint block_num, uint block_cnt, void* buf)
{
	int transfer_cnt = 0;
	int code = 0;
	debug_pr("%s dst=%08x blk=%08x cnt=%08x\n", __func__, (uint)buf, block_num, block_cnt);
	while (block_cnt) {
		code = mmc_block_write(pdev, block_num, buf, 512);
		if (code)
			break;
		block_cnt--;
		transfer_cnt++;
		buf = ((uint8_t *)buf) + 512;
		block_num++;
	}
	return MAKE_RET_CODE(code, transfer_cnt);
}

void repeat_error(struct sdmmc_dev *pdev, char* s)
{
	stop_clock(pdev);
	while (1) {
	        my_printf(s);
		delayMicro(1000000);
	}
}


unsigned char uboot_name[] __attribute__ ((aligned (32))) = "U-BOOT  BIN";	//located in root directory


struct dir_entry {
	uint8_t fname[11];	//0x00
#define ATTR_READONLY		0x01
#define ATTR_HIDDEN		0x02
#define ATTR_SYSTEM		0x04
#define ATTR_VOLUME_LABEL	0x08
#define ATTR_SUBDIR		0x10
#define ATTR_ARCHIVE		0x20
#define ATTR_DEVICE		0x40
#define ATTR_UNUSED		0x80

#define ATTR_LONG_ENTRY		0x0f
	uint8_t attrib;		//0x0b
	uint8_t reserved1;	//0x0c
	uint8_t create_time_ms;	//0x0d
	uint16_t create_time;	//0x0e
	uint16_t create_date;	//0x10
	uint16_t last_access_date;	//0x12
	uint16_t cluster_high;	//0x14
	uint16_t modify_time;	//0x16
	uint16_t modify_date;	//0x18
	uint16_t cluster_low;	//0x1a
	uint32_t file_size;	//0x1c, byte count
};

#define CLUSTER_FREE			0x0
#define CLUSTER_MIN			0x2
#define CLUSTER_MAX32			0x0fffffef
#define CLUSTER_LAST_IN_FILE_MIN32	0x0ffffff8
#define CLUSTER_MASK32			0x0fffffff

#define CLUSTER_MAX16			0x0ffef
#define CLUSTER_LAST_IN_FILE_MIN16	0x0fff8


static int check_cluster_range(unsigned cluster, struct info *pinfo)
{
	unsigned max = (pinfo->fat32) ? CLUSTER_MAX32 : CLUSTER_MAX16;
	if ((cluster < CLUSTER_MIN) || (cluster > max)) {
		print_hex(cluster, 8);
		repeat_error(&pinfo->dev, " cluster not valid\r\n");
		return STATUS_CLUSTER_NOT_VALID;
	}
	return 0;
}

static unsigned get_link(unsigned cluster, struct info *pinfo)
{
	unsigned x;
	x = pinfo->fat_sector + (cluster >> ((pinfo->fat32) ? 7 : 8));
	if (x >= pinfo->cluster2StartSector)
		return -1;
	if (pinfo->fat_block != x) {
		sd_read_blocks(&pinfo->dev, x, 1, pinfo->fat_buf);
		debug_dump(pinfo->fat_buf, x, 1);
		pinfo->fat_block = x;
	}
	if (pinfo->fat32) {
		uint32_t *u = (uint32_t *)pinfo->fat_buf;
		return u[cluster & 0x7f] & CLUSTER_MASK32;
	} else {
		uint16_t *u = (uint16_t *)pinfo->fat_buf;
		return u[cluster & 0xff];
	}
}

int scan_block_for_file(struct info *pinfo)
{
	uint32_t *fname = pinfo->fname;
	int i = 16;
	struct dir_entry *d = pinfo->c.buf;
	while (i) {
		if (d->attrib != ATTR_LONG_ENTRY) {
			uint32_t *u = (uint32_t *)d;
			if ((u[0] == fname[0]) && (u[1] == fname[1]) && (!((u[2] ^ fname[2]) & 0xffffff))) {
				pinfo->file_cluster = (d->cluster_high << 16) | d->cluster_low;
				pinfo->file_size = d->file_size;
				return STATUS_FILE_FOUND;
			}
			if (0) if (u[0]) {
				print_buf((char*)d->fname, 11);
				my_printf("\n");
				print_buf((char*)fname, 11);
				my_printf("\n");
				my_printf("u:     %x %x %x\n", u[0], u[1], u[2]);
				my_printf("fname: %x %x %x\n", fname[0], fname[1], fname[2]);
			}
		}
		i--;
		d++;
	}
	return 0;
}

int scan_chain(unsigned cluster, unsigned chain, struct info *pinfo, work_func work)
{
	int ret;
	unsigned last_marker = (pinfo->fat32) ? CLUSTER_LAST_IN_FILE_MIN32 : CLUSTER_LAST_IN_FILE_MIN16;
	do {
		unsigned remaining_in_cluster;
		unsigned x;
		if (chain) {
			ret = check_cluster_range(cluster, pinfo);
			if (ret)
				return ret;
			remaining_in_cluster = pinfo->sectorsPerCluster;
			x = pinfo->cluster2StartSector + ((cluster - 2) * pinfo->sectorsPerCluster);
		} else {
			remaining_in_cluster = pinfo->root_dir_blocks;
			x = cluster;
			ret = 0;
		}
		do {
			if ((x <= pinfo->partition_start) || (x >= pinfo->partition_end)) {
				print_hex(x, 8);
				repeat_error(&pinfo->dev, " block not valid\r\n");
				return STATUS_BLOCK_NOT_VALID;
			}
			sd_read_blocks(&pinfo->dev, x, 1, pinfo->c.buf);
			debug_dump(pinfo->c.buf, x, 1);
			ret = work(pinfo);
			if (ret)
				return ret;
			remaining_in_cluster--;
			x++;
		} while (remaining_in_cluster);
		
		if (!chain)
			break;
		cluster = get_link(cluster, pinfo);
		if (cluster >= last_marker)
			return STATUS_END_OF_CHAIN;
//	        debug_pr("%x link cluster\r\n", cluster);
	} while (1);
	return ret;
}
