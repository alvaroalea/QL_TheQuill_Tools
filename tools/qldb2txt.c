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

//FIXME: show 255 * in the list of vocabulary
//FIXME: remove/replace all putc()
//FIXME: fix verbose mode


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
#define DIR_SIZE 4
#define SEPARATOR ";-    -    -    -    -    -    -    -    -    -    -    -    -    -    \n"


typedef uint8_t zbyte;
typedef uint32_t zword;

int verbose;

#define BASE 0x00

zbyte qdb[0xFFFFF];
char *filename = NULL; /* BORRAR */
char *infile = NULL;
char *outfile = NULL;
FILE *fpout;

zword peekw(zword offset)
{
	return 0x1000000 * qdb[offset] + 0x10000 * qdb[offset + 1] + 0x100 * qdb[offset + 2] + qdb[offset + 3];
}

void syntax(const char *av0)
{
	fprintf(stderr, "Syntax: %s file.qdb {output.qse}\n\n", av0);
	exit(1);
}

void dotext(zword offset)
{
	static int xpos = 0;
	while (1)
	{
		zbyte c = ~qdb[offset];

		if (c==0x00) { fprintf(fpout, "\n"); return; }
		else if (c == 0xfe) { fprintf(fpout, "\n"); xpos = 0; }
/* A line cannot start with a slash or a semicolon. If it's the
 * case that the compiled adventure somehow contains one
 * anyway, escape it. */
		else if (xpos == 0 && (c == '/' || c == ';'))
		{
			fprintf(fpout, "\\%c", c);
			++xpos;
		}
/* That also means we need to escape backslashes */
		else if (c == '\\')
		{
			fprintf(fpout, "\\\\");
			++xpos;
		}
		else if (c < 32 || c > 128) { fprintf(fpout, "\\0x%02x", c); }
		else
		{
			fprintf(fpout, "%c", c);
			++xpos;
		}
		offset++;
	}
}

void dump_text(zword count, zword table)
{
	int n;
	zword addr;
	zword systab = peekw(table) - BASE;

	for (n = 0; n < qdb[count]; n++)
	{
		addr = peekw(systab + DIR_SIZE * n);
		fprintf(fpout, "/%d\n", n);
		dotext(addr - BASE);
	}
}

void show_word(zbyte w)
{
	int m;
	zword table = peekw(OFF_T_VOC - BASE);

	if (w == 0xFF)
	{
		fprintf(fpout, "_   ");
		return;
	}
	while(qdb[table + 4] != 0)
	{
		if (qdb[table + 4] == w)
		{
			for (m = 0; m < 4; m++)
			{
				fputc(~qdb[table+m], fpout);
			}
			return;
		}
		table += 5;
	}
/* Unknown word, render as (num) */
	fprintf(fpout, "(%d) ", w);
}
void printat(zbyte what, zword count, zword tab)
{
	zbyte ch;
	zword addr;

	fprintf(fpout, "\t;");

	if (what >= qdb[count]) return; /* Out of range */

	addr = peekw(peekw(tab) - BASE + DIR_SIZE * what) - BASE;
	do
	{
		ch = ~qdb[addr];
		if (isprint(ch)) fputc(ch, fpout);
		++addr;
	} while (ch != '\n' && ch != '\r');
}

void showobj(zword obj) { printat(obj, OFF_N_OBJ, OFF_T_OBJ); }
void showloc(zword loc) { printat(loc, OFF_N_LOC, OFF_T_LOC); }
void showmsg(zword msg) { printat(msg, OFF_N_MSG, OFF_T_MSG); }
void showsys(zword msg) { printat(msg, OFF_N_SMS, OFF_T_SMS); }

static int first = 1;
zword show_cond(zword addr)
{
	while (1)
	{
		if (qdb[addr] == 0xFF)
		{
			addr++;
			break;
		}
		if (!first) {
			fprintf(fpout, "\t\t");
		}
		first = 0;
		switch(qdb[addr])
		{
			case 0:	printf("AT\t%d", qdb[addr + 1]);
				if (verbose) showloc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 1:	printf("NOTAT\t%d", qdb[addr + 1]);
				if (verbose) showloc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 2:	printf("ATGT\t%d", qdb[addr + 1]);
				if (verbose) showloc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 3:	printf("ATLT\t%d", qdb[addr + 1]);
				if (verbose) showloc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 4:	printf("PRESENT\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 5:	printf("ABSENT\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 6:	printf("WORN\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 7:	printf("NOTWORN\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 8:	printf("CARRIED\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 9:	printf("NOTCARR\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
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
			/* This are not cont
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
			*/
			case 55: printf("ISAT\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose)
				{
					showobj(qdb[addr + 1]);
					printf("\n\t\t");
					showloc(qdb[addr + 2]);
				}
				putchar('\n');
				addr += 3;
				break;
			case 56: printf("COPYOF\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose) showobj(qdb[addr + 1]);
				putchar('\n');
				addr += 3;
				break;
			case 57: printf("COPYOO\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose)
				{
					showobj(qdb[addr + 1]);
					printf("\n\t\t");
					showobj(qdb[addr + 2]);
				}
				putchar('\n');
				addr += 3;
				break;
			case 58: printf("COPYFO\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose) showobj(qdb[addr + 2]);
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
	while (1)
	{
		if (qdb[addr] == 0xFF)
		{
			addr++;
			break;
		}
		if (!first) {
			fprintf(fpout, "\t\t");
		}
		first = 0;

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
				if (verbose) showloc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 19:	printf("NOTAT\t%d", qdb[addr + 1]); /* was 01 */
				if (verbose) showloc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 20:	printf("ATGT\t%d", qdb[addr + 1]); /* was 2 */
				if (verbose) showloc(qdb[addr + 1]);
				putchar('\n');
				addr += 2;
				break;
			case 21: printf("GOTO\t%d", qdb[addr + 1]); /* was 37 CONFIRMADO 21 */
				if (verbose) showloc(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 22: printf("MESSAGE\t%d", qdb[addr + 1]); /* was 38 CONFIRMADO 22 */
				if (verbose) showmsg(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 39: printf("REMOVE\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 24: printf("GET\t%d", qdb[addr + 1]); /* was 40 CONFIRMED 24 */
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 25: printf("DROP\t%d", qdb[addr + 1]); /* was 41 CONFIRMED 25 */
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 26:	printf("CARRIED\t%d", qdb[addr + 1]); /* was 8 */
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 27: printf("DESTROY\t%d", qdb[addr + 1]); /* was 43 CONFIRMED 27 */
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 28: printf("CREATE\t%d", qdb[addr + 1]); /* was 44 CONFIRM 28 */
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 29: printf("SWAP\t%d %d", qdb[addr + 1], qdb[addr + 2]); /* was 45  CONFIRMED 29 */
				if (verbose)
				{
					showobj(qdb[addr + 1]);
					printf("\n\t\t");
					showobj(qdb[addr + 2]);
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
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 23:	printf("ABSENT\t%d", qdb[addr + 1]); /* was 5  */
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 40:	printf("WORN\t%d", qdb[addr + 1]); /* was 6 */
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 41:	printf("NOTWORN\t%d", qdb[addr + 1]); /* was 7 */
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 42: printf("WEAR\t%d", qdb[addr + 1]);
				if (verbose)showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 43:	printf("NOTCARR\t%d", qdb[addr + 1]); /* was 9 */
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
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
					showobj(qdb[addr + 1]);
					printf("\n\t\t");
					showloc(qdb[addr + 2]);
				}
				fputc('\n', fpout);
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
				if (verbose) showsys(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 55: printf("ISAT\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose)
				{
					showobj(qdb[addr + 1]);
					printf("\n\t\t");
					showloc(qdb[addr + 2]);
				}
				fputc('\n', fpout);
				addr += 3;
				break;
			case 56: printf("COPYOF\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 3;
				break;
			case 57: printf("COPYOO\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose)
				{
					showobj(qdb[addr + 1]);
					printf("\n\t\t");
					showobj(qdb[addr + 2]);
				}
				putchar('\n');
				addr += 3;
				break;
			case 58: printf("COPYFO\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose) showobj(qdb[addr + 2]);
				fputc('\n', fpout);
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
				if (verbose) showloc(qdb[addr + 1]);
				fputc('\n', fpout);
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
  first = 1;
  a = show_cond(addr);
  show_act(a);
  putchar('\n');
}



void dump_logic(zword addr)
{
	zword table = peekw(addr) - BASE;
	zword func;

	while (1)
	{
		if (qdb[table] == 0 && qdb[table + 1] == 0)
		{
			return;
		}
		show_word(qdb[table]);
		fprintf(fpout, "\t");
		show_word(qdb[table+1]);
		fprintf(fpout, "\t");
		func = peekw(table + 2);
		show_condact(func - BASE);
		table += 6;
	}
}

void show_connections()
{
	int n;
	zword table = peekw(OFF_T_CON) - BASE;
	zword conn;

	for (n = 0; n < qdb[OFF_N_LOC]; n++)
	{
		conn = peekw(table + 4 * n) - BASE;

		printf("%04X\tConnections for room %d\n", conn + BASE, n);
		if (verbose)
		{
			printf("\t\t");
			showloc(n);
			putchar('\n');
		}
		while(qdb[conn] != 0xFF)
		{
			putchar('\t');
			show_word(qdb[conn]);
			printf(" to %d",qdb[conn+1]);
			if (verbose)
			{
				fprintf(fpout, "\t");
				showloc(qdb[conn+1]);
			}
			putchar('\n');
			conn += 2;
		}
		putchar('\n');
	}
}


void dump_connections()
{
	int n;
	zword table = peekw(OFF_T_CON) - BASE;
	zword conn;

	for (n = 0; n < qdb[OFF_N_LOC]; n++)
	{
		conn = peekw(table + DIR_SIZE * n) - BASE;

		fprintf(fpout, "/%d", n);
		if (verbose)
		{
			fprintf(fpout, "\t\t\t");
			showloc(n);
		}
		fputc('\n', fpout);
		while(qdb[conn] != 0xFF)
		{
			fprintf(fpout, "\t");
			show_word(qdb[conn]);
			fprintf(fpout, "\t%d",qdb[conn+1]);
			if (verbose)
			{
				fprintf(fpout, "\t");
				showloc(qdb[conn+1]);
			}
			fprintf(fpout, "\n");
			conn += 2;
		}
	}
}

void show_objlocs()
{
	int n;
	zword table = peekw(OFF_T_OBL) - BASE;
	zbyte loc;

	for (n = 0; n < qdb[OFF_N_OBJ]; n++)
	{
		loc = qdb[table + n];

		printf("%04X\tObject %d starts in room %d\n", table + BASE,
			n, loc);
		if (verbose)
		{
			printf("\t\t");
			showobj(n);
			putchar('\n');
			printf("\t\t");
			showloc(loc);
			putchar('\n');
		}
		putchar('\n');
	}


}

void show_objwords()
{
	int n;
	zword table = peekw(OFF_T_OBW) - BASE;
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
			showobj(n);
			putchar('\n');
		}
	}
	putchar('\n');
}

void dump_obdefs()
{
	int n;
	zword places = peekw(OFF_T_OBL) - BASE;
	zword names = peekw(OFF_T_OBW) - BASE;
	zbyte loc;
	zbyte name;

	for (n = 0; n < qdb[OFF_N_OBJ]; n++)
	{
		loc  = qdb[places + n];
		name = qdb[names + n];

		fprintf(fpout, "/%d\t", n);
		switch (loc)
		{
			case 252:	fprintf(fpout, "_"); break;
			case 253:	fprintf(fpout, "WORN"); break;
			case 254:	fprintf(fpout, "CARRIED"); break;
			default:	fprintf(fpout, "%d", loc); break;
		}
		fputc('\t', fpout);
		show_word(name);
		if (verbose)
		{
			fputc('\t', fpout);
			showobj(n);
		}
		fputc('\n', fpout);
	}
}


void dump_vocab()
{
	int  m;

	zword table = peekw(OFF_T_VOC) - BASE;

	while(qdb[table + 4] != 0)
	{
		for (m = 0; m < 4; m++) fputc(~qdb[table+m], fpout);
		fprintf(fpout, "\t%d\n", qdb[table + 4]);
		table += 5;
	}

}

int main(int argc, char **argv)
{
	int n,m;
	FILE *fp;

	for (n = 1; n < argc; n++)
	{
		if (!strcmp(argv[n], "-v") || !strcmp(argv[n], "-V")) verbose = 1;
		else if (infile == NULL) infile = argv[n];
		else if (outfile == NULL) outfile = argv[n];
		else syntax(argv[0]);
	}
	if (infile == NULL)
	{
		syntax(argv[0]);
	}
	fp = fopen(infile, "rb");
	if (!fp)
	{
		perror(infile);
		return 1;
	}
	memset(qdb, 0, sizeof(qdb));
	fread(qdb, 1, sizeof(qdb), fp);
	fclose(fp);
	if (outfile == NULL)
	{
		outfile = "standard output";
		fpout = stdout;
	}
	else
	{
		fpout = fopen(outfile, "w");
		if (!fpout)
		{
			perror(outfile);
			exit(1);
		}
	}

	fprintf(fpout, "/CTL\t\t;Control Section\n");
	fprintf(fpout, "Q\t\t;Indicates Quill rather than PAW data\n");
	fprintf(fpout, "_\t\t;Null word character is an underline\n");
	fprintf(fpout,"; Ink Color: %d\n",  qdb[2]);
	fprintf(fpout,"; Paper Color: %d\n", qdb[3]);
	fprintf(fpout,"; Border Witdh: %d\n", qdb[4]);
	fprintf(fpout,"; Border Color: %d\n", qdb[5]);
	fprintf(fpout,"; Conveyable Objects :%d\n", qdb[6]);
	m=0x38 ;
	fprintf(fpout,"; Allocated memory buffer: %d\n", 0x1000000*qdb[m]+ 0x10000*qdb[m+1]+ 0x100*qdb[m+2]+qdb[m+3]);
	fprintf(fpout, SEPARATOR);

	fprintf(fpout, "/VOC\t\t;Vocabulary\n");
	fprintf(fpout, ";\t\tMovements are numbered < 13\n");
	dump_vocab();
	fprintf(fpout, SEPARATOR);

	fprintf(fpout, "/STX\t\t;System messages\n");
	dump_text(OFF_N_SMS, OFF_T_SMS);
	fprintf(fpout, SEPARATOR);

	fprintf(fpout, "/MTX\t\t;Messages\n");
	dump_text(OFF_N_MSG, OFF_T_MSG);
	fprintf(fpout, SEPARATOR);

	fprintf(fpout, "/OTX\t\t;Object descriptions\n");
	dump_text(OFF_N_OBJ, OFF_T_OBJ);
	fprintf(fpout, SEPARATOR);

	fprintf(fpout, "/LTX\t\t;Location descriptions\n");
	dump_text(OFF_N_LOC, OFF_T_LOC);
	fprintf(fpout, SEPARATOR);

	fprintf(fpout, "/CON\t\t;Connections\n");
	dump_connections();
	fprintf(fpout, SEPARATOR);

	fprintf(fpout, "/OBJ\t\t;Object definitions\n");
	fprintf(fpout, ";obj\tstarts\tword\n");
	fprintf(fpout, ";num\t  at\n");
	dump_obdefs();
	fprintf(fpout, SEPARATOR);

	fprintf(fpout, "/PRO\t0\t;Process 0 (ie response)\n");
	dump_logic(OFF_T_RES);
	fprintf(fpout, SEPARATOR);

	fprintf(fpout, "/PRO\t2\t;Process 2 (run every turn)\n");
	dump_logic(OFF_T_PRC);
	return 0;
}