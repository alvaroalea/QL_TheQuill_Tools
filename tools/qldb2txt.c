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
//FIXME: Add suport por inverse as \+ y \-


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
			case 0:	fprintf(fpout, "AT\t%d", qdb[addr + 1]);
				if (verbose) showloc(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 1:	fprintf(fpout, "NOTAT\t%d", qdb[addr + 1]);
				if (verbose) showloc(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 2:	fprintf(fpout, "ATGT\t%d", qdb[addr + 1]);
				if (verbose) showloc(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 3:	fprintf(fpout, "ATLT\t%d", qdb[addr + 1]);
				if (verbose) showloc(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 4:	fprintf(fpout, "PRESENT\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 5:	fprintf(fpout, "ABSENT\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 6:	fprintf(fpout, "WORN\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 7:	fprintf(fpout, "NOTWORN\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 8:	fprintf(fpout, "CARRIED\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 9:	fprintf(fpout, "NOTCARR\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 10: fprintf(fpout, "CHANCE\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 11: fprintf(fpout, "ZERO\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 12: fprintf(fpout, "NOTZERO\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 13: fprintf(fpout, "EQ\t%d %d\n", qdb[addr + 1],
					qdb[addr + 2]);
				addr += 3;
				break;
			case 14: fprintf(fpout, "GT\t%d %d\n", qdb[addr + 1],
					qdb[addr + 2]);
				addr += 3;
				break;
			case 15: fprintf(fpout, "LT\t%d %d\n", qdb[addr + 1],
					qdb[addr + 2]);
				addr += 3;
				break;
			default: fprintf(fpout, "??%02x??\n", qdb[addr]);
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
			case 0: fprintf(fpout, "INVEN\n"); /* was 18 CONFIRMED 0 */
				addr++;
				break;
			case 1: fprintf(fpout, "DESC\n"); /*was 19 CONFIRMED 1 */
				addr++;
				break;
			case 2: fprintf(fpout, "QUIT\n"); /* was 20 CONFIRMED 2 */
				addr++;
				break;
			case 3: fprintf(fpout, "END\n"); /* was 21 CONFIRMED 3 */
				addr++;
				break;
			case 4: fprintf(fpout, "DONE\n"); /* was 22 CONFIRMED 4 */
				addr++;
				break;
			case 5: fprintf(fpout, "OK\n"); /* was 23 CONFIRMED 5 */
				addr++;
				break;
			case 6: fprintf(fpout, "ANYKEY\n"); /* was 24 CONFIRMED 6 */
				addr++;
				break;
			case 7: fprintf(fpout, "SAVE\n"); /* was 25 CONFIRMED 7 */
				addr++;
				break;
			case 8: fprintf(fpout, "LOAD\n"); /* was 26 CONFIRMED 8 */
				addr++;
				break;
			case 9: fprintf(fpout, "TURNS\n"); /* was 27 CONFIRMED 9 */
				addr++;
				break;
			case 10: fprintf(fpout, "SCORE\n"); /* was 28 */
				addr++;
				break;
			case 11: fprintf(fpout, "CLS\n"); /* was 29 CONFIRMED 11 */
				addr++;
				break;
			case 12: fprintf(fpout, "DROPALL\n");
				addr++;
				break;
			case 13: fprintf(fpout, "AUTOG\n"); /* was 31 CONFIRM 13 */
				addr++;
				break;
			case 14: fprintf(fpout, "AUTOD\n"); /* was 32 CONFIRMED 14 */
				addr++;
				break;
			case 15: fprintf(fpout, "AUTOW\n");
				addr++;
				break;
			case 16: fprintf(fpout, "AUTOR\n");
				addr++;
				break;
			case 17: fprintf(fpout, "PAUSE\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 18: fprintf(fpout, "INK\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 19: fprintf(fpout, "PAPER\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 20: fprintf(fpout, "BORDER\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 21: fprintf(fpout, "GOTO\t%d", qdb[addr + 1]); /* was 37 CONFIRMADO 21 */
				if (verbose) showloc(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 22: fprintf(fpout, "MESSAGE\t%d", qdb[addr + 1]); /* was 38 CONFIRMADO 22 */
				if (verbose) showmsg(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 23: fprintf(fpout, "REMOVE\t%d", qdb[addr + 1]);
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 24: fprintf(fpout, "GET\t%d", qdb[addr + 1]); /* was 40 CONFIRMED 24 */
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 25: fprintf(fpout, "DROP\t%d", qdb[addr + 1]); /* was 41 CONFIRMED 25 */
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 26:	fprintf(fpout, "WEAR\t%d", qdb[addr + 1]); 
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 27: fprintf(fpout, "DESTROY\t%d", qdb[addr + 1]); /* was 43 CONFIRMED 27 */
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 28: fprintf(fpout, "CREATE\t%d", qdb[addr + 1]); /* was 44 CONFIRM 28 */
				if (verbose) showobj(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 29: fprintf(fpout, "SWAP\t%d %d", qdb[addr + 1], qdb[addr + 2]); /* was 45  CONFIRMED 29 */
				if (verbose)
				{
					showobj(qdb[addr + 1]);
					fprintf(fpout, "\n\t\t");
					showobj(qdb[addr + 2]);
				}
				fputc('\n', fpout);
				addr += 3;
				break;

			case 30: fprintf(fpout, "PLACE\t%d %d", qdb[addr + 1], qdb[addr + 2]);
				if (verbose)
				{
					showobj(qdb[addr + 1]);
					fprintf(fpout, "\n\t\t");
					showloc(qdb[addr + 2]);
				}
				fputc('\n', fpout);
				addr += 3;
				break;
			case 31: fprintf(fpout, "SET\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 32: fprintf(fpout, "CLEAR\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 33: fprintf(fpout, "PLUS\t%d %d\n", qdb[addr + 1], qdb[addr + 2]);
				addr += 3;
				break;
			case 34: fprintf(fpout, "MINUS\t%d %d\n", qdb[addr + 1], qdb[addr + 2]);
				addr += 3;
				break;
			case 35: fprintf(fpout, "LET\t%d %d\n", qdb[addr + 1], qdb[addr + 2]);
				addr += 3;
				break;
			case 36: fprintf(fpout, "NEWLINE\n");
				addr++;
				break;
			case 37: fprintf(fpout, "RAMSAVE\n"); /* NEW CONFIRMED 37 */
				addr++;
				break;
			case 38: fprintf(fpout, "RAMLOAD\n"); 
				addr++;
				break;
			case 39: fprintf(fpout, "PRINT\t%d\n", qdb[addr + 1]);
				addr += 2;
				break;
			case 40: fprintf(fpout, "SYSMESS\t%d", qdb[addr + 1]);
				if (verbose) showsys(qdb[addr + 1]);
				fputc('\n', fpout);
				addr += 2;
				break;
			case 41: fprintf(fpout, "COPYFF\t%d %d\n", qdb[addr + 1], qdb[addr + 2]);
				addr += 3;
				break;
			default: fprintf(fpout, "??%02x??\n", qdb[addr]);
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
  fputc('\n', fpout);
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

		fprintf(fpout, "%04X\tConnections for room %d\n", conn + BASE, n);
		if (verbose)
		{
			fprintf(fpout, "\t\t");
			showloc(n);
			fputc('\n', fpout);
		}
		while(qdb[conn] != 0xFF)
		{
			fputc('\t', fpout);
			show_word(qdb[conn]);
			fprintf(fpout, " to %d",qdb[conn+1]);
			if (verbose)
			{
				fprintf(fpout, "\t");
				showloc(qdb[conn+1]);
			}
			fputc('\n', fpout);
			conn += 2;
		}
		fputc('\n', fpout);
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

		fprintf(fpout, "%04X\tObject %d starts in room %d\n", table + BASE,
			n, loc);
		if (verbose)
		{
			fprintf(fpout, "\t\t");
			showobj(n);
			fputc('\n', fpout);
			fprintf(fpout, "\t\t");
			showloc(loc);
			fputc('\n', fpout);
		}
		fputc('\n', fpout);
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

		fprintf(fpout, "%04X\tObject %d is named ", table + BASE + n, n);
		show_word(noun);
		fputc('\n', fpout);

		if (verbose)
		{
			fprintf(fpout,"\t\t");
			showobj(n);
			fputc('\n', fpout);
		}
	}
	fputc('\n', fpout);
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

	fprintf(fpout, "/PRO\t0\t;Process 0 (Event Table - ie response)\n");
	dump_logic(OFF_T_RES);
	fprintf(fpout, SEPARATOR);

	fprintf(fpout, "/PRO\t2\t;Process 2 (Status Table - run every turn)\n");
	dump_logic(OFF_T_PRC);
	return 0;
}
