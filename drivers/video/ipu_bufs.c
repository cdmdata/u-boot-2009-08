/*
 * (C) Copyright 2010 Boundary Devices
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
#include <asm/io.h>
#include <asm/errno.h>

DECLARE_GLOBAL_DATA_PTR;

struct ipu_ch_param_word {
	unsigned data[5];
	unsigned res[3];
};

struct ipu_ch_param {
	struct ipu_ch_param_word word[2];
};

static unsigned const buf_offsets[] = {
    0x268
,   0x270
};

static void print_field(
	char const *name,
	struct ipu_ch_param_word const *param_word,
	unsigned startbit,
	unsigned numbits)
{
	unsigned value ; 
	unsigned wordnum=(startbit/32); 
	if( wordnum == ((startbit+numbits-1)/32) ) { 
		unsigned shifted = ((*param_word).data[wordnum] >> (startbit & 31)); 
		value = shifted & ((1<<numbits)-1) ; 
	} /* fits in one word */	
	else { 
		unsigned lowbits = 32-(startbit&31); 
		unsigned low = ((*param_word).data[wordnum] >> (startbit & 31)) & ((1<<lowbits)-1) ; 
		unsigned highbits = numbits-lowbits ; 
		unsigned high = (*param_word).data[wordnum+1] & ((1<<highbits)-1); 
		value = low | (high << lowbits); 
	} 
	printf( "%s %3u:%-2u\t == 0x%08x\n", name, startbit, numbits, value ); 
}

struct bitfield {
	char const *name ;
	unsigned wordnum ;
	unsigned startbit ;
	unsigned numbits ;
};

static void set_field(
	struct ipu_ch_param_word *param_word,
	struct bitfield const *field, unsigned value)
{
	printf( "set field %s to value %u/0x%x here: start %u, count %u\n", field->name, value, value, field->startbit, field->numbits );
	unsigned const max = (1<<field->numbits)-1 ;
	if( value > max ){
		fprintf(stderr, "Error: range of %s is [0..0x%x]\n", field->name, max );
		return ;
	}
	unsigned wordnum=(field->startbit/32); 
	if( wordnum == ((field->startbit+field->numbits-1)/32) ) { 
		unsigned shifted = value << (field->startbit & 31);
		unsigned mask = ~(max << (field->startbit&31));
		unsigned oldval = (*param_word).data[wordnum] & ~mask ;
		printf( "single word value 0x%08x, mask 0x%08x, oldval 0x%08x\n", shifted, mask, oldval );
		if( shifted != oldval ) {
                        (*param_word).data[wordnum] = ((*param_word).data[wordnum]&mask) | shifted ;
			printf( "value changed\n");
		}
	} /* fits in one word */	
	else { 
		printf( "no multi-word support yet\n" );
		unsigned oldlow = (*param_word).data[wordnum];
		unsigned lowbits = 32-(field->startbit&31);
		unsigned lowval = value & ((1<<lowbits)-1);
		unsigned newlow = (oldlow&((1<<field->startbit)-1)) | (lowval<<field->startbit);
		printf( "oldlow == 0x%08x\nnewval == 0x%08x\n", oldlow, newlow);
		unsigned oldhigh = (*param_word).data[wordnum+1];
		unsigned highbits = field->numbits-lowbits;
		unsigned highval = value >> lowbits ;
		unsigned newhigh = (oldhigh&(~((1<<highbits)-1))) | highval ;
		printf( "oldhigh == 0x%08x\nnewval == 0x%08x\n", oldhigh, newhigh);
		(*param_word).data[wordnum] = newlow ;
		(*param_word).data[wordnum+1] = newhigh ;
	} 
}

static struct bitfield const fields[] = {
    {"XV",0,0,9},
    {"YV",0,10,9},
    {"XB",0,19,13},
    {"YB",0,32,12},
    {"NSB_B",0,44,1},
    {"CF",0,45,1},
    {"UBO",0,46,22},
    {"VBO",0,68,22},
    {"IOX",0,90,4},
    {"RDRW",0,94,1},
    {"BPP",0,107,3},
    {"SO",0,113,1},
    {"BNDM",0,114,3},
    {"BM",0,117,2},
    {"ROT",0,119,1},
    {"HF",0,120,1},
    {"VF",0,121,1},
    {"THE",0,122,1},
    {"CAP",0,123,1},
    {"CAE",0,124,1},
    {"FW",0,125,13},
    {"FH",0,138,12},
    {"EBA0",1,0,29},
    {"EBA1",1,29,29},
    {"ILO",1,58,20},
    {"NPB",1,78,7},
    {"PFS",1,85,4},
    {"ALU",1,89,1},
    {"ALBM",1,90,2},
    {"ID",1,93,2},
    {"TH",1,95,7},
    {"SLY",1,102,14},
    {"WID3",1,125,3},
    {"SLUV",1,128,14},
    {"CRE",1,149,1},
};

int
do_ipu_bufs (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	unsigned char const *rdy_base ;
	unsigned const *cur_buf_base ;
	struct ipu_ch_param *params ;
	unsigned chan ;
	if( 2 > argc ){
	    fprintf(stderr, "Usage: %s buffernum [field value]\n", argv[0]);
	    return -1 ;
	}

	rdy_base = (unsigned char *)IPU_CM_REG_BASE ;
	cur_buf_base = (unsigned *)(IPU_CM_REG_BASE + 0x23C);
	params = (struct ipu_ch_param *)IPU_CPMEM_REG_BASE ;
	chan = simple_strtoul(argv[1],0,0);
	if( 80 >= chan ){
		unsigned i ;
                struct ipu_ch_param *param = &params[chan];
                printf( "--------------- ipu ch %u ---------------\n", chan );
		for( i = 0 ; i < ARRAY_SIZE(param->word); i++ ){
                        unsigned j ;
			struct ipu_ch_param_word const *w = &param->word[i];
			unsigned addr = (char *)w - (char *)params + IPU_CPMEM_REG_BASE ;
			printf( "[%08x]: ", addr );
			for(j = 0 ; j < ARRAY_SIZE(w->data); j++ ) {
	                        unsigned char b ;
                                unsigned char *bytes = (unsigned char *)(w->data+j);
				for( b = 0 ; b < 4 ; b++ ){
					printf( "%02x ", bytes[b]);
				}
				printf( " " );
			}
			printf("\n");
		}
		// printf( "bufs == %08x %08x\n", param->word[1].data[0], param->word[1].data[1]);
		unsigned long addrs[] = {
			(param->word[1].data[0] & ((1<<29)-1))*8,
			((param->word[1].data[0] >> 29)|(param->word[1].data[1]<<3))*8
		};
		
		unsigned curbuf_long = chan/32 ;
		unsigned curbuf_bit = chan%31 ;
		unsigned curbuf = (cur_buf_base[curbuf_long] >> curbuf_bit)&1 ;
		
		for( i = 0 ; i < 2 ; i++ ){
			unsigned char bits = rdy_base[buf_offsets[i]+chan/8];
			unsigned char mask = 1<<(chan&7);
			int ready = 0 != (bits & mask);
			printf( "ipu_buf[chan %u][%d] == 0x%08lx %s %s\n", chan, i, addrs[i], 
					ready ? "READY" : "NOT READY",
					(i==curbuf) ? "<-- current" : "");
		}

		if( 2 < argc ) {
		    char const *fieldname = argv[2];
		    for( i = 0 ; i < ARRAY_SIZE(fields); i++ ){
			    if( 0 == strcmp(fields[i].name,fieldname)) {
				    print_field(fields[i].name,
						&param->word[fields[i].wordnum],
						fields[i].startbit,
						fields[i].numbits);
				    if( 3 < argc ) {
					    unsigned const value = simple_strtoul(argv[3],0,0);
					    set_field(&param->word[fields[i].wordnum],&fields[i],value);
				    }
				    return 0 ;
			    }
		    }
		    fprintf( stderr, "field %s not found\n", fieldname );
		} else {
			for( i = 0 ; i < ARRAY_SIZE(fields); i++ ){
				print_field(fields[i].name,&param->word[fields[i].wordnum],fields[i].startbit,fields[i].numbits);
			}
		} // print all fields
	} else
                fprintf(stderr, "Invalid channel %s, 0x%x\n", argv[1],chan);
	
	return 0 ;
}

U_BOOT_CMD(
	ipu_bufs, 4, 0,	do_ipu_bufs,
	"ipu_bufs - show ipu_bufs\n",
	""
);

