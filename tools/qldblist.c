/* qldblist: Disassemble games written with the Sinclair QL version of the "Quill"
           adventure game system
    Copyright (C) 2023  John Elliott and (C) 2024 Alvaro Alea

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* OFFSET OF DATA AND TABLES */
#define OFF_N_OBJ 7
#define OFF_N_LOC 8
#define OFF_N_MSG 9
#define OFF_N_SMS 0xA
#define OFF_T_RES 0x0C
#define OFF_T_PRC 0x10
#define OFF_T_OBJ 0x14
#define OFF_T_LOC 0x18
#define OFF_T_MSG 0x1C
#define OFF_T_SMS 0x20
#define OFF_T_CON 0x24
#define OFF_T_VOC 0x28
#define OFF_T_OBL 0x2C
#define OFF_T_OBW 0x30


typedef uint8_t zbyte;
typedef uint32_t zword;

int skipconn, skipmsg, skipsys, skipobj, skiploc, skipproc, skipresp, skipvoc = 0;
int verbose;

#define BASE 0x00

zbyte qdb[0xFFFFF];
char *filename = NULL;

zword peekw(zword offset)
{
	return qdb[offset] + 256 * qdb[offset + 1];

}

zword peek2w(zword offset)
{
	return 0x1000000 * qdb[offset] + 0x10000 * qdb[offset + 1] + 0x100 * qdb[offset + 2] + qdb[offset + 3];
}


void syntax(const char *av0)
{
	fprintf(stderr, "Syntax: %s { options } file.qdb\n\n", av0);
	fprintf(stderr, "Options:\n"
			"\t-sl\tSkip locations\n"
			"\t-sm\tSkip messages\n"
			"\t-sn\tSkip connections\n"
			"\t-so\tSkip objects\n"
			"\t-sp\tSkip Process table\n"
			"\t-sr\tSkip Response table\n"
			"\t-sv\tSkip vocabulary\n"
			"\t-v\tVerbose\n");
	exit(1);
}

void showtext(zword offset, const char *prefix)
{
	while (1)
	{
		uint8_t c = ~qdb[offset];

		if (c==0x00) printf("\\r");
		else if (c == 0xfe) printf("\\n\"\n\t\t%s\"", prefix);
		else if (c<32) printf("[\\0x%02x]", c);
		else if (c>128) printf("{\\0x%02x}", c);
		else	putchar(c);
		if (c == 0x00) break;
		offset++;
	}
}


void show_txttab(zword count, zword table, const char *what)
{	
	int n;
	zword addr;
	zword systab = peek2w(table) - BASE;

	for (n = 0; n < qdb[count]; n++)
	{
		addr = peek2w(systab + 4 * n);
		printf("%06X\t%s %d\n", addr, what, n);
		printf("\tTEXT\t\"");
		showtext(addr - BASE, "");
		printf("\"\n");
	}
}

void display_msg(zword nloc)
{
	zword addr = peekw((peekw(0x1C) - BASE) + 4 * nloc);
	printf("\t; \"");
	showtext(addr - BASE, "\t");
	putchar('"');
}

void display_sysmsg(zword nloc)
{
	zword addr = peekw((peekw(0x20) - BASE) + 4 * nloc);
	printf("\t; \"");
	showtext(addr - BASE, "\t");
	putchar('"');
}


void display_loc(zword nloc)
{
	zword addr;
	switch(nloc)
	{
		case 0xFC: printf("\t; Destroyed\n"); return;
		case 0xFD: printf("\t; Worn\n"); return;
		case 0xFE: printf("\t; Carried\n"); return;
	}
	addr = peek2w((peek2w(OFF_T_LOC) - BASE) + 4 * nloc);
	printf("\t; \"");
	showtext(addr - BASE, "\t");
	putchar('"');
}

void display_obj(zword nobj)
{
	zword addr = peekw((peekw(0x30) - BASE) + 4 * nobj);
	printf("\t; \"");
	showtext(addr - BASE, "\t");
	putchar('"');
}


void show_word(zbyte w)
{
	int m;
	zword table = peek2w(OFF_T_VOC - BASE);
	if (w == 0xFF) 
	{
		printf("_   ");
		return;
	}
	while(qdb[table + 4] != 0)
	{
		if (qdb[table + 4] == w)
		{
			for (m = 0; m < 4; m++) putchar(~qdb[table+m]);
			return;
		}
		table += 5;
	}
	printf("?    ");
}


zword show_cond(zword addr)
{
	printf("\tconds\n");
	while (1)
	{
		if (qdb[addr] == 0xFF)
		{
			addr++;
			break;
		}
		printf("%06X\t\t", addr + BASE);
		switch(qdb[addr])
		{
			case 0:	printf("AT\t%d", qdb[addr + 1]);
				if (verbose) display_loc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 1:	printf("NOTAT\t%d", qdb[addr + 1]);
				if (verbose) display_loc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 2:	printf("ATGT\t%d", qdb[addr + 1]);
				if (verbose) display_loc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 3:	printf("ATLT\t%d", qdb[addr + 1]);
				if (verbose) display_loc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 4:	printf("PRESENT\t%d", qdb[addr + 1]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 5:	printf("ABSENT\t%d", qdb[addr + 1]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 6:	printf("WORN\t%d", qdb[addr + 1]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 7:	printf("NOTWORN\t%d", qdb[addr + 1]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 8:	printf("CARRIED\t%d", qdb[addr + 1]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 9:	printf("NOTCARR\t%d", qdb[addr + 1]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 10: printf("CHANCE\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 11: printf("ZERO\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 12: printf("NOTZERO\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 13: printf("EQ\t%d %d\n", qdb[addr + 1],
					qdb[addr + 2]);
				addr += 3;
				break;
			case 14: printf("GT\t%d %d\n", qdb[addr + 1],
					qdb[addr + 2]);
				addr += 3;
				break;
			case 15: printf("LT\t%d %d\n", qdb[addr + 1],
					qdb[addr + 2]);
				addr += 3;
				break;
			case 16: printf("WORD3\t%d\t", qdb[addr + 1]);
				if (verbose) show_word(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 17: printf("WORD4\t%d\t", qdb[addr + 1]);
				if (verbose) show_word(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 18: printf("INVEN\n");
				addr++;
				break;
			case 19: printf("DESC\n");
				addr++;
				break;
			case 20: printf("QUIT\n");
				addr++;
				break;
			case 21: printf("END\n");
				addr++;
				break;
			case 22: printf("DONE\n");
				addr++;
				break;
			case 23: printf("OK\n");
				addr++;
				break;
			case 24: printf("ANYKEY\n");
				addr++;
				break;
			case 25: printf("SAVE\n");
				addr++;
				break;
			case 26: printf("LOAD\n");
				addr++;
				break;
			case 27: printf("TURNS\n");
				addr++;
				break;
			case 28: printf("SCORE\n");
				addr++;
				break;
			case 29: printf("CLS\n");
				addr++;
				break;
			case 30: printf("DROPALL\n");
				addr++;
				break;
			case 31: printf("AUTOG\n");
				addr++;
				break;
			case 32: printf("AUTOD\n");
				addr++;
				break;
			case 33: printf("AUTOW\n");
				addr++;
				break;
			case 34: printf("AUTOR\n");
				addr++;
				break;
			case 35: printf("PAUSE\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 36: printf("BELL\n");
				addr++;
				break;
			case 37: printf("GOTO\t%d", qdb[addr + 1]);
				if (verbose) display_loc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 38: printf("MESSAGE\t%d", qdb[addr + 1]);
				if (verbose) display_msg(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 39: printf("REMOVE\t%d", qdb[addr + 1]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 40: printf("GET\t%d", qdb[addr + 1]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 41: printf("DROP\t%d", qdb[addr + 1]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 42: printf("WEAR\t%d", qdb[addr + 1]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 43: printf("DESTROY\t%d", qdb[addr + 1]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 44: printf("CREATE\t%d", qdb[addr + 1]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 45: printf("SWAP\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose)
				{
					display_obj(qdb[addr + 1]);
					printf("\n\t\t");
					display_obj(qdb[addr + 2]);
				}
				putchar('\n');
				addr += 3;
				break;
			case 46: printf("PLACE\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose)
				{
					display_obj(qdb[addr + 1]);
					printf("\n\t\t");
					display_loc(qdb[addr + 2]);
				}
				putchar('\n');
				addr += 3;
				break;
			case 47: printf("SET\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 48: printf("CLEAR\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 49: printf("PLUS\t%d %d\n", qdb[addr + 1], qdb[addr + 2]);
				addr += 3;
				break;
			case 50: printf("MINUS\t%d %d\n", qdb[addr + 1], qdb[addr + 2]);
				addr += 3;
				break;
			case 51: printf("LET\t%d %d\n", qdb[addr + 1], qdb[addr + 2]);
				addr += 3;
				break;
			case 52: printf("NEWLINE\n");
				addr++;
				break;
			case 53: printf("PRINT\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 54: printf("SYSMESS\t%d", qdb[addr + 1]);
				if (verbose) display_sysmsg(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 55: printf("ISAT\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose)
				{
					display_obj(qdb[addr + 1]);
					printf("\n\t\t");
					display_loc(qdb[addr + 2]);
				}
				putchar('\n');
				addr += 3;
				break;
			case 56: printf("COPYOF\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 3;
				break;
			case 57: printf("COPYOO\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose)
				{
					display_obj(qdb[addr + 1]);
					printf("\n\t\t");
					display_obj(qdb[addr + 2]);
				}
				putchar('\n');
				addr += 3;
				break;
			case 58: printf("COPYFO\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose) display_obj(qdb[addr + 2]);
				putchar('\n');
				addr += 3;
				break;
			case 59: printf("COPYFF\t%d %d\n", qdb[addr + 1], qdb[addr + 2]);
				addr += 3;
				break;
			case 60: printf("ISDESC\n");
				addr++;
				break;
			case 61: printf("EXTERN\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			default: printf("??%02x??\n", qdb[addr]);
				addr++;
				break;
		}
	}
	return (addr) ;
}

void show_act(zword addr)
{
	printf("\tacts\n");
	while (1)
	{
		if (qdb[addr] == 0xFF)
		{
			addr++;
			break;
		}
		printf("%06X\t\t", addr + BASE);
		switch(qdb[addr])
		{
			case 0: printf("INVEN\n"); /* was 18 CONFIRMED 0 */
				addr++;
				break;
			case 1: printf("DESC\n"); /*was 19 CONFIRMED 1 */
				addr++;
				break;
			case 2: printf("QUIT\n"); /* was 20 CONFIRMED 2 */
				addr++;
				break;
			case 3: printf("END\n"); /* was 21 CONFIRMED 3 */
				addr++;
				break;
			case 4: printf("DONE\n"); /* was 22 CONFIRMED 4 */
				addr++;
				break;
			case 5: printf("OK\n"); /* was 23 CONFIRMED 5 */
				addr++;
				break;
			case 6: printf("ANYKEY\n"); /* was 24 CONFIRMED 6 */
				addr++;
				break;
			case 7: printf("SAVE\n"); /* was 25 CONFIRMED 7 */
				addr++;
				break;
			case 8: printf("LOAD\n"); /* was 26 CONFIRMED 8 */
				addr++;
				break;
			case 9: printf("TURNS\n"); /* was 27 CONFIRMED 9 */
				addr++;
				break;
			case 10: printf("CHANCE\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 11: printf("CLS\n"); /* was 29 CONFIRMED 11 */
				addr++;
				break;
			case 12: printf("NOTZERO\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 13: printf("AUTOG\n"); /* was 31 CONFIRM 13 */
				addr++;
				break;
			case 14: printf("AUTOD\n"); /* was 32 CONFIRMED 14 */
				addr++;
				break;
			case 15: printf("LT\t%d %d\n", qdb[addr + 1],
					qdb[addr + 2]);
				addr += 3;
				break;
			case 16: printf("WORD3\t%d\t", qdb[addr + 1]);
				if (verbose) show_word(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 17: printf("WORD4\t%d\t", qdb[addr + 1]);
				if (verbose) show_word(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 18:	printf("AT\t%d", qdb[addr + 1]); /* was 0 */
				if (verbose) display_loc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 19:	printf("NOTAT\t%d", qdb[addr + 1]); /* was 01 */
				if (verbose) display_loc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 20:	printf("ATGT\t%d", qdb[addr + 1]); /* was 2 */
				if (verbose) display_loc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 21: printf("GOTO\t%d", qdb[addr + 1]); /* was 37 CONFIRMADO 21 */
				if (verbose) display_loc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 22: printf("MESSAGE\t%d", qdb[addr + 1]); /* was 38 CONFIRMADO 22 */
				if (verbose) display_msg(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 39: printf("REMOVE\t%d", qdb[addr + 1]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 24: printf("GET\t%d", qdb[addr + 1]); /* was 40 CONFIRMED 24 */
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 25: printf("DROP\t%d", qdb[addr + 1]); /* was 41 CONFIRMED 25 */
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 26:	printf("CARRIED\t%d", qdb[addr + 1]); /* was 8 */
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 27: printf("DESTROY\t%d", qdb[addr + 1]); /* was 43 CONFIRMED 27 */
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 28: printf("CREATE\t%d", qdb[addr + 1]); /* was 44 CONFIRM 28 */
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 29: printf("SWAP\t%d %d", qdb[addr + 1], qdb[addr + 2]); /* was 45  CONFIRMED 29 */
				if (verbose)
				{
					display_obj(qdb[addr + 1]);
					printf("\n\t\t");
					display_obj(qdb[addr + 2]);
				}
				putchar('\n');
				addr += 3;
				break;
			case 30: printf("DROPALL\n");
				addr++;
				break;
			case 31: printf("SET\t%d\n", qdb[addr + 1]); /* was 47 */
				addr += 2;
				break;
			case 32: printf("GT\t%d %d\n", qdb[addr + 1], /* was 14 */
					qdb[addr + 2]);
				addr += 3;
				break;
			case 33: printf("AUTOW\n");
				addr++;
				break;
			case 34: printf("AUTOR\n");
				addr++;
				break;
			case 35: printf("PAUSE\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 36: printf("BELL\n");
				addr++;
				break;
			case 37: printf("RAMSAVE\n"); /* NEW CONFIRMED 37 */
				addr++;
				break;
			case 38:	printf("PRESENT\t%d", qdb[addr + 1]); /* was 4  */
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 23:	printf("ABSENT\t%d", qdb[addr + 1]); /* was 5  */
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 40:	printf("WORN\t%d", qdb[addr + 1]); /* was 6 */
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 41:	printf("NOTWORN\t%d", qdb[addr + 1]); /* was 7 */
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 42: printf("WEAR\t%d", qdb[addr + 1]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 43:	printf("NOTCARR\t%d", qdb[addr + 1]); /* was 9 */
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 44: printf("SCORE\n"); /* was 28 */
				addr++;
				break;
			case 45: printf("ZERO\t%d\n", qdb[addr + 1]); /* was 11 */
				addr += 2;
				break;
			case 46: printf("PLACE\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose)
				{
					display_obj(qdb[addr + 1]);
					printf("\n\t\t");
					display_loc(qdb[addr + 2]);
				}
				putchar('\n');
				addr += 3;
				break;
			case 47: printf("EQ\t%d %d\n", qdb[addr + 1], /* was 13 */
					qdb[addr + 2]);
				addr += 3;
				break;
			case 48: printf("CLEAR\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 49: printf("PLUS\t%d %d\n", qdb[addr + 1], qdb[addr + 2]);
				addr += 3;
				break;
			case 50: printf("MINUS\t%d %d\n", qdb[addr + 1], qdb[addr + 2]);
				addr += 3;
				break;
			case 51: printf("LET\t%d %d\n", qdb[addr + 1], qdb[addr + 2]);
				addr += 3;
				break;
			case 52: printf("NEWLINE\n");
				addr++;
				break;
			case 53: printf("PRINT\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 54: printf("SYSMESS\t%d", qdb[addr + 1]);
				if (verbose) display_sysmsg(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 55: printf("ISAT\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose)
				{
					display_obj(qdb[addr + 1]);
					printf("\n\t\t");
					display_loc(qdb[addr + 2]);
				}
				putchar('\n');
				addr += 3;
				break;
			case 56: printf("COPYOF\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose) display_obj(qdb[addr + 1]);
				putchar('\n');
				addr += 3;
				break;
			case 57: printf("COPYOO\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose)
				{
					display_obj(qdb[addr + 1]);
					printf("\n\t\t");
					display_obj(qdb[addr + 2]);
				}
				putchar('\n');
				addr += 3;
				break;
			case 58: printf("COPYFO\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose) display_obj(qdb[addr + 2]);
				putchar('\n');
				addr += 3;
				break;
			case 59: printf("COPYFF\t%d %d\n", qdb[addr + 1], qdb[addr + 2]);
				addr += 3;
				break;
			case 60: printf("ISDESC\n");
				addr++;
				break;
			case 61: printf("EXTERN\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 62:	printf("ATLT\t%d", qdb[addr + 1]); /* was 3 */
				if (verbose) display_loc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 63: printf("RAMLOAD\n");
				addr++;
				break;
			default: printf("??%02x??\n", qdb[addr]);
				addr++;
				break;
		}
	}
}


void show_condact(zword addr)
{
  zword a;
  a = show_cond(addr);
  show_act(a);
  putchar('\n');
}



void show_logic(zword addr, int response)
{
	zword table = peek2w(addr) - BASE;
	zword func;

	while (1)
	{
		printf("%06X\t", table);
		if (qdb[table] == 0 && qdb[table + 1] == 0)
		{
			putchar('\n');
			return;
		}
		if (response)
		{
			show_word(qdb[table]);
			putchar(' ');
			show_word(qdb[table+1]);
			putchar('\n');
			func = peek2w(table + 2);
			show_condact(func - BASE);
			table += 6;
		}
		else /* NOT SURE WHAT IS THIS, BUT NOT USED */
		{
			putchar('\n');
			func = peek2w(table);
			show_condact(func - BASE);
			table += 2;
		}
	}

}

void show_obj() { show_txttab(OFF_N_OBJ, OFF_T_OBJ, "Object"); }
void show_loc() { show_txttab(OFF_N_LOC, OFF_T_LOC, "Location"); }
void show_msg() { show_txttab(OFF_N_MSG, OFF_T_MSG, "Message"); }
void show_sys() { show_txttab(OFF_N_SMS, OFF_T_SMS, "System message"); }

void show_connections()
{
	int n;
	zword table = peek2w(OFF_T_CON) - BASE;
	zword conn;

	for (n = 0; n < qdb[OFF_N_LOC]; n++)
	{
		conn = peek2w(table + 4 * n) - BASE;

		printf("%04X\tConnections for room %d\n", conn + BASE, n);
		if (verbose)
		{
			printf("\t\t");
			display_loc(n);
			putchar('\n');
		}
		while(qdb[conn] != 0xFF)
		{
			putchar('\t');
			show_word(qdb[conn]);
			printf(" to %d",qdb[conn+1]);			
			if (verbose) 
			{
				display_loc(qdb[conn+1]);
			}
			putchar('\n');
			conn += 2;
		}
		putchar('\n');
	}
}

void show_objlocs()
{
	int n;
	zword table = peek2w(OFF_T_OBL) - BASE;
	zbyte loc;

	for (n = 0; n < qdb[OFF_N_OBJ]; n++)
	{
		loc = qdb[table + n];

		printf("%04X\tObject %d starts in room %d\n", table + BASE, 
			n, loc);
		if (verbose) 
		{
			printf("\t\t");
			display_obj(n);
			putchar('\n');
			printf("\t\t");
			display_loc(loc);
			putchar('\n');
		}
		putchar('\n');
	}
	

}

void show_objwords()
{
	int n;
	zword table = peek2w(OFF_T_OBW) - BASE;
	zbyte noun;

	for (n = 0; n < qdb[OFF_N_OBJ]; n++)
	{
		noun = qdb[table + n];

		printf("%04X\tObject %d is named ", table + BASE + n, n);
		show_word(noun);
		putchar('\n');

		if (verbose) 
		{
			printf("\t\t");
			display_obj(n);
			putchar('\n');
		}
	}
	putchar('\n');
}


void show_vocab()
{
	int  m;

	zword table = peek2w(OFF_T_VOC) - BASE;

	while(qdb[table + 4] != 0)
	{
		printf("%04X\t\"", table + BASE);
		for (m = 0; m < 4; m++) putchar(~qdb[table+m]);
		printf("\"\t%d\n", qdb[table + 4]);	
		table += 5;
	}	

}

int main(int argc, char **argv)
{
	int n,m;
	FILE *fp;

	for (n = 1; n < argc; n++) /*FIXME not work if to much parameters */
	{
		if (!strcmp(argv[n], "-sl")) skiploc = 1;
		else if (!strcmp(argv[n], "-sm")) skipmsg = 1;
		else if (!strcmp(argv[n], "-sn")) skipconn = 1;
		else if (!strcmp(argv[n], "-so")) skipobj = 1;
		else if (!strcmp(argv[n], "-sp")) skipproc = 1;
		else if (!strcmp(argv[n], "-sr")) skipresp = 1;
		else if (!strcmp(argv[n], "-ss")) skipsys = 1;
		else if (!strcmp(argv[n], "-v")) verbose = 1;
		else if (filename == NULL) filename = argv[n];
		else syntax(argv[0]);
	}
	if (filename == NULL)
	{
		syntax(argv[0]);
	}

	fp = fopen(filename, "rb");
	if (!fp)
	{
		perror(filename);
		return 1;
	}	
	memset(qdb, 0, sizeof(qdb));
	fread(qdb, 1, sizeof(qdb), fp);
	fclose(fp);

	printf("; Generated by QLdbList v0.1\n\n[Header]\n");
	printf("000000\t%02x\t; Unknown\n", qdb[0]);
	printf("000001\t%02x\t; Version\n", qdb[1]);
	printf("000002\t%02x\t; Ink Color %d\n", qdb[2], qdb[2]);
	printf("000003\t%02x\t; Paper Color %d\n", qdb[3], qdb[3]);
	printf("000004\t%02x\t; Border Witdh %d\n", qdb[4], qdb[4]);
	printf("000005\t%02x\t; Border Color %d\n", qdb[5], qdb[5]);
	printf("000006\t%02x\t; Conveyable Objects %d\n", qdb[6], qdb[6]);
	printf("000007\t%02x\t; %d objects\n", qdb[7], qdb[7]);
	printf("000008\t%02x\t; %d locations\n", qdb[8], qdb[8]);
	printf("000009\t%02x\t; %d messages\n", qdb[9], qdb[9]);
	printf("00000A\t%02x\t; %d system messages\n", qdb[0xa], qdb[0xa]);
	printf("00000B\t%02x\t; Unknown\n", qdb[0xb]);
	m=OFF_T_RES ; printf("%06X\t%02x %02x %02x %02x\t; Response table at 0x%08x\n",m, qdb[m], qdb[m+1], qdb[m+2], qdb[m+3], peek2w(m));
	m=OFF_T_PRC ; printf("%06X\t%02x %02x %02x %02x\t; Process table at 0x%08x\n",m, qdb[m], qdb[m+1], qdb[m+2], qdb[m+3], peek2w(m));
	m=OFF_T_OBJ ; printf("%06X\t%02x %02x %02x %02x\t; Objects at 0x%08x\n",m, qdb[m], qdb[m+1], qdb[m+2], qdb[m+3], peek2w(m));
	m=OFF_T_LOC ; printf("%06X\t%02x %02x %02x %02x\t; Locations at 0x%08x\n",m, qdb[m], qdb[m+1], qdb[m+2], qdb[m+3], peek2w(m));
	m=OFF_T_MSG ; printf("%06X\t%02x %02x %02x %02x\t; Messages at 0x%08x\n",m, qdb[m], qdb[m+1], qdb[m+2], qdb[m+3], peek2w(m));
	m=OFF_T_SMS ; printf("%06X\t%02x %02x %02x %02x\t; System messages at 0x%08x\n",m, qdb[m], qdb[m+1], qdb[m+2], qdb[m+3], peek2w(m));
	m=OFF_T_CON ; printf("%06X\t%02x %02x %02x %02x\t; Connections at 0x%08x\n",m, qdb[m], qdb[m+1], qdb[m+2], qdb[m+3], peek2w(m));
	m=OFF_T_VOC ; printf("%06X\t%02x %02x %02x %02x\t; Vocabulary at 0x%08x\n",m, qdb[m], qdb[m+1], qdb[m+2], qdb[m+3], peek2w(m));
	m=OFF_T_OBL ; printf("%06X\t%02x %02x %02x %02x\t; Object locations at 0x%08x\n",m, qdb[m], qdb[m+1], qdb[m+2], qdb[m+3], peek2w(m));
	m=OFF_T_OBW ; printf("%06X\t%02x %02x %02x %02x\t; Object words at 0x%08x\n",m, qdb[m], qdb[m+1], qdb[m+2], qdb[m+3], peek2w(m));
	m=0x34 ; printf("%06X\t%02x %02x %02x %02x\t; Database ends at 0x%08x\n",m, qdb[m], qdb[m+1], qdb[m+2], qdb[m+3], peek2w(m));
	m=0x38 ; printf("%06X\t%02x %02x %02x %02x\t; Allocated memory buffer: %d\n",m, qdb[m], qdb[m+1], qdb[m+2], qdb[m+3], peek2w(m));

		if (!skipresp)
	{
		printf("\n[Event (Response) table]\n");
		show_logic(OFF_T_RES, 1);
	}
	if (!skipproc)
	{
		printf("\n[Status (Process) table]\n");
		show_logic(OFF_T_PRC, 1);
	}
	if (!skipsys)
	{
		printf("\n[System messages]\n");
		show_sys();
	}
	if (!skipmsg)
	{
		printf("\n[Messages]\n");
		show_msg();
	}
	if (!skipobj)
	{
		printf("\n[Objects]\n");
		show_obj();
	}
	if (!skiploc)
	{
		printf("\n[Locations]\n");
		show_loc();
	}
	if (!skipconn)
	{
		printf("\n[Connections]\n");
		show_connections();
	}
	if (!skipvoc)
	{
		printf("\n[Vocabulary]\n");
		show_vocab();
	}
	if (!skipobj)
	{
		printf("\n[Object locations]\n");
		show_objlocs();
		printf("\n[Object words]\n");
		show_objwords();
	}
	return 0;
}

