/*
 * (C) Copyright 20011, Boundary Devices
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>

#if CONFIG_I2C_MXC
#include <i2c.h>
void bus_i2c_init(unsigned base, int speed, int unused);
int bus_i2c_read(unsigned base, uchar chip, uint addr, int alen, uchar *buf, int len);
int bus_i2c_write(unsigned base, uchar chip, uint addr, int alen, uchar *buf, int len);
#else
#error attempt to include DA90_i2c_pmic without I2C_MXC
#endif

#ifndef DA90_I2C_BUS
#error DA90_I2C_BUS is undefined
#endif

#define DA90REG_PAGE_CON	0
#define DA90REG_STATUS_A	1
#define DA90REG_STATUS_B	2
#define DA90REG_STATUS_C	3
#define DA90REG_STATUS_D	4
#define DA90REG_EVENT_A		5
#define DA90REG_EVENT_B		6
#define DA90REG_EVENT_C		7
#define DA90REG_EVENT_D		8
#define DA90REG_FAULT_LOG52	9
#define DA90REG_IRQ_MASK_A	10
#define DA90REG_IRQ_MASK_B	11
#define DA90REG_IRQ_MASK_C	12
#define DA90REG_IRQ_MASK_D	13
#define DA90REG_CONTROL_A	14
#define DA90REG_CONTROL_B	15
#define DA90REG_CONTROL_C	16
#define DA90REG_CONTROL_D	17
#define DA90REG_PD_DIS		18
#define DA90REG_INTERFACE	19
#define DA90REG_RESET		20
#define DA90REG_GPIO_0_1	21
#define DA90REG_GPIO_2_3	22
#define DA90REG_GPIO_4_5	23
#define DA90REG_GPIO_6_7	24
#define DA90REG_GPIO_8_9	25
#define DA90REG_GPIO_10_11	26
#define DA90REG_GPIO_12_13	27
#define DA90REG_GPIO_14_15	28
#define DA90REG_ID_0_1		29
#define DA90REG_ID_2_3		30
#define DA90REG_ID_4_5		31
#define DA90REG_ID_6_7		32
#define DA90REG_ID_10_		34
#define DA90REG_ID_10_11	34
#define DA90REG_ID_12_13	35
#define DA90REG_ID_14_15	36
#define DA90REG_ID_16_17	37
#define DA90REG_ID_18_19	38
#define DA90REG_ID_20_21	39
#define DA90REG_SEQ_STATUS	40
#define DA90REG_SEQ_A		41
#define DA90REG_SEQ_B		42
#define DA90REG_SEQ_TIMER	43
#define DA90REG_BUCK_A		44
#define DA90REG_BUCK_B		45
#define DA90REG_BUCKCORE	46
#define DA90REG_BUCKPRO		47
#define DA90REG_BUCKMEM		48
#define DA90REG_BUCKPERI	49
#define DA90REG_LDO1		50
#define DA90REG_LDO2		51
#define DA90REG_LDO3		52
#define DA90REG_LDO4		53
#define DA90REG_LDO5		54
#define DA90REG_LDO6		55
#define DA90REG_LDO7		56
#define DA90REG_LDO8		57
#define DA90REG_LDO9		58
#define DA90REG_LDO10		59
#define DA90REG_SUPPLY		60
#define DA90REG_PULLDOWN	61
#define DA90REG_CHG_BUCK	62
#define DA90REG_WAIT_CONT	63
#define DA90REG_ISET		64
#define DA90REG_BAT_CHG		65
#define DA90REG_CHG_CONT	66
#define DA90REG_INPUT_CONT	67
#define DA90REG_CHG_TIME	68
#define DA90REG_BBAT_CONT	69
#define DA90REG_BOOST		70
#define DA90REG_LED_CONT	71
#define DA90REG_LEDMIN		72
#define DA90REG_LED1_CONF	73
#define DA90REG_LED2_CONF	74
#define DA90REG_LED3_CONF	75
#define DA90REG_LED1_CONT	76
#define DA90REG_LED2_CONT	77
#define DA90REG_LED3_CONT	78
#define DA90REG_LED4_CONT	79
#define DA90REG_LED5_CONT	80
#define DA90REG_ADC_MAN		81
#define DA90REG_ADC_CONT	82
#define DA90REG_ADC_RES_L	83
#define DA90REG_ADC_RES_H	84
#define DA90REG_VDD_RES		85
#define DA90REG_VDD_MON		86
#define DA90REG_ICHG_AV		87
#define DA90REG_ICHG_THD	88
#define DA90REG_ICHG_END	89
#define DA90REG_TBAT_RES	90
#define DA90REG_TBAT_HIGHP	91
#define DA90REG_TBAT_HIGHN	92
#define DA90REG_TBAT_LOW	93
#define DA90REG_T_OFFSET	94
#define DA90REG_ADCIN4_RES	95
#define DA90REG_AUTO4_HIGH	96
#define DA90REG_AUTO4_LOW	97
#define DA90REG_ADCIN5_RES	98
#define DA90REG_AUTO5_HIGH	99
#define DA90REG_AUTO5_LOW	100
#define DA90REG_ADCIN6_RES	101
#define DA90REG_AUTO6_HIGH	102
#define DA90REG_AUTO6_LOW	103
#define DA90REG_TJUNC_RES	104
#define DA90REG_TSI_CONT_A	105
#define DA90REG_TSI_CONT_B	106
#define DA90REG_TSI_X_MSB	107
#define DA90REG_TSI_Y_MSB	108
#define DA90REG_TSI_LSB		109
#define DA90REG_TSI_Z_MSB	110
#define DA90REG_COUNT_S		111
#define DA90REG_COUNT_MI	112
#define DA90REG_COUNT_H		113
#define DA90REG_COUNT_D		114
#define DA90REG_COUNT_MO	115
#define DA90REG_COUNT_Y		116
#define DA90REG_ALARM_MI	117
#define DA90REG_ALARM_H		118
#define DA90REG_ALARM_D		119
#define DA90REG_ALARM_MO	120
#define DA90REG_ALARM_Y		121
#define DA90REG_SECOND_A	122
#define DA90REG_SECOND_B	123
#define DA90REG_SECOND_C	124
#define DA90REG_SECOND_D	125

static char const * const regnames[] = {
	"PAGE_CON",
	"STATUS_A",
	"STATUS_B",
	"STATUS_C",
	"STATUS_D",
	"EVENT_A",
	"EVENT_B",
	"EVENT_C",
	"EVENT_D",
	"FAULT_LOG52",
	"IRQ_MASK_A",
	"IRQ_MASK_B",
	"IRQ_MASK_C",
	"IRQ_MASK_D",
	"CONTROL_A",
	"CONTROL_B",
	"CONTROL_C",
	"CONTROL_D",
	"PD_DIS",
	"INTERFACE",
	"RESET",
	"GPIO_0_1",
	"GPIO_2_3",
	"GPIO_4_5",
	"GPIO_6_7",
	"GPIO_8_9",
	"GPIO_10_11",
	"GPIO_12_13",
	"GPIO_14_15",
	"ID_0_1",
	"ID_2_3",
	"ID_4_5",
	"ID_6_7",
	"ID_10_",
	"ID_10_11",
	"ID_12_13",
	"ID_14_15",
	"ID_16_17",
	"ID_18_19",
	"ID_20_21",
	"SEQ_STATUS",
	"SEQ_A",
	"SEQ_B",
	"SEQ_TIMER",
	"BUCK_A",
	"BUCK_B",
	"BUCKCORE",
	"BUCKPRO",
	"BUCKMEM",
	"BUCKPERI",
	"LDO1",
	"LDO2",
	"LDO3",
	"LDO4",
	"LDO5",
	"LDO6",
	"LDO7",
	"LDO8",
	"LDO9",
	"LDO10",
	"SUPPLY",
	"PULLDOWN",
	"CHG_BUCK",
	"WAIT_CONT",
	"ISET",
	"BAT_CHG",
	"CHG_CONT",
	"INPUT_CONT",
	"CHG_TIME",
	"BBAT_CONT",
	"BOOST",
	"LED_CONT",
	"LEDMIN",
	"LED1_CONF",
	"LED2_CONF",
	"LED3_CONF",
	"LED1_CONT",
	"LED2_CONT",
	"LED3_CONT",
	"LED4_CONT",
	"LED5_CONT",
	"ADC_MAN",
	"ADC_CONT",
	"ADC_RES_L",
	"ADC_RES_H",
	"VDD_RES",
	"VDD_MON",
	"ICHG_AV",
	"ICHG_THD",
	"ICHG_END",
	"TBAT_RES",
	"TBAT_HIGHP",
	"TBAT_HIGHN",
	"TBAT_LOW",
	"T_OFFSET",
	"ADCIN4_RES",
	"AUTO4_HIGH",
	"AUTO4_LOW",
	"ADCIN5_RES",
	"AUTO5_HIGH",
	"AUTO5_LOW",
	"ADCIN6_RES",
	"AUTO6_HIGH",
	"AUTO6_LOW",
	"TJUNC_RES",
	"TSI_CONT_A",
	"TSI_CONT_B",
	"TSI_X_MSB",
	"TSI_Y_MSB",
	"TSI_LSB",
	"TSI_Z_MSB",
	"COUNT_S",
	"COUNT_MI",
	"COUNT_H",
	"COUNT_D",
	"COUNT_MO",
	"COUNT_Y",
	"ALARM_MI",
	"ALARM_H",
	"ALARM_D",
	"ALARM_MO",
	"ALARM_Y",
	"SECOND_A",
	"SECOND_B",
	"SECOND_C",
	"SECOND_D",
};

#define NUM_REGISTERS (sizeof(regnames)/sizeof(regnames[0]))

int do_pmic (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if ((2==argc) && (0 ==strcmp(argv[1],"all")) ){
		unsigned regnum ;
		for( regnum=0 ; regnum < NUM_REGISTERS ; regnum++ ) {
			uchar val ;
			if (0 == bus_i2c_read(DA90_I2C_BUS, DA90_I2C_ADDR,
					regnum, 1, &val, 1)) {
				printf( "0x%02x %3u  %-20s 0x%02x\n", regnum,
						regnum, regnames[regnum], val);
			} else
				printf( "%s: error reading register %u\n",
						__func__, regnum);
		}
	} else if (1 < argc) {
		unsigned char buf[4];
		unsigned val;
		uchar prev;
		unsigned reg = simple_strtoul(argv[1], NULL, 0);
		if (reg >= NUM_REGISTERS) {
			printf("invalid register %s, max is %u (0x%x)\n",
				argv[1], NUM_REGISTERS-1, NUM_REGISTERS-1 );
			return -1;
		}
		if (bus_i2c_read(DA90_I2C_BUS, DA90_I2C_ADDR, reg, 1,
				&prev, 1)) {
			printf( "%s: error reading register %u\n", __func__, reg);
			return -1;
		}
		printf("0x%02x %3u  %-20s 0x%02x", reg, reg, regnames[reg], prev );
		if (argc <= 2) {
			printf("\n");
			return 0;
		}
		val = simple_strtoul(argv[2],NULL,0);
		if (val > 0xff) {
			printf("%s: value(%x) too large\n", __func__, val);
			return -1;
		}
		if (reg == DA90REG_CONTROL_B)
			val |= (1 << 5);	/* repeated write mode */
		printf(" --> 0x%02x", val );

		if (reg != DA90REG_CONTROL_B) {
			if (bus_i2c_read(DA90_I2C_BUS, DA90_I2C_ADDR,
					DA90REG_CONTROL_B, 1, &prev, 1)) {
				printf( "%s: error reading register %u\n",
						__func__, DA90REG_CONTROL_B);
				return -1;
			}
			if (!((prev >> 5) & 1)) {
				/* enable repeated write mode */
				buf[0] = (prev | (1 << 5)) & ~0xc0;
				bus_i2c_write(DA90_I2C_BUS, DA90_I2C_ADDR,
						DA90REG_CONTROL_B, 1, buf, 1);
			}
		}
		buf[0] = (unsigned char)val;
		buf[1] = DA90REG_STATUS_A;
		buf[2] = 0;
		if (bus_i2c_write(DA90_I2C_BUS, DA90_I2C_ADDR, reg, 1,
				buf, 3)) {
			printf("%s: i2c_write error\n", __func__);
			return -1;
		}
		printf("\n");
	} else {
		cmd_usage(cmdtp);
	}
	return 0 ;
}

/***************************************************/
U_BOOT_CMD(
	pmic,	5,	1,	do_pmic,
	"pmic    - Read/write PMIC registers through I2C\n",
	"register [value]\n"
);
