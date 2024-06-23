/* qdblist: Disassemble games written with the CP/M version of the "Quill" 
           adventure game system
    Copyright (C) 2023  John Elliott

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
#include <stdlib.h>
#include <string.h>

typedef unsigned char zbyte;
typedef unsigned short zword;

int skipconn, skipmsg, skipsys, skipobj, skiploc, skipproc, skipresp, skipvoc;
int verbose;

#define BASE 0xF00

zbyte qdb[0xF100];
char *filename = NULL;

zword peekw(zword offset)
{
	return qdb[offset] + 256 * qdb[offset + 1];
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
		char c = ~qdb[offset];

		if (c < ' ')
		{
			if (c == 0x0d) printf("\\r");
			else if (c == 0x0a) printf("\\n\"\n\t\t%s\"", prefix);
			else printf("\\0x%02x", c);
		}
		else	putchar(c);
		if (c == 0x0d) break;
		offset++;
	}
}


void show_txttab(zword count, zword table, const char *what)
{	
	int n;
	zword addr;
	zword systab = peekw(table) - BASE;

	for (n = 0; n < qdb[count]; n++)
	{
		addr = peekw(systab + 2 * n);
		printf("%04X\t%s %d\n", addr, what, n);
		printf("\tTEXT\t\"");
		showtext(addr - BASE, "");
		printf("\"\n");
	}
}

void display_msg(zword nloc)
{
	zword addr = peekw((peekw(0x0F) - BASE) + 2 * nloc);
	printf("\t; \"");
	showtext(addr - BASE, "\t");
	putchar('"');
}

void display_sysmsg(zword nloc)
{
	zword addr = peekw((peekw(0x11) - BASE) + 2 * nloc);
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
	addr = peekw((peekw(0x0D) - BASE) + 2 * nloc);
	printf("\t; \"");
	showtext(addr - BASE, "\t");
	putchar('"');
}

void display_obj(zword nobj)
{
	zword addr = peekw((peekw(0x0B) - BASE) + 2 * nobj);
	printf("\t; \"");
	showtext(addr - BASE, "\t");
	putchar('"');
}


void show_word(zbyte w)
{
	int m;
	zword table = peekw(0x15) - BASE;

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


void show_condact(zword addr)
{
	while (1)
	{
		if (qdb[addr] == 0xFF)
		{
			putchar('\n');
			break;
		}
		printf("%04X\t", addr + BASE);
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
}


void show_logic(zword addr, int response)
{
	zword table = peekw(addr) - BASE;
	zword func;

	while (1)
	{
		printf("%04X\t", table);
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
			func = peekw(table + 2);
			show_condact(func - BASE);
			table += 4;
		}
		else
		{
			putchar('\n');
			func = peekw(table);
			show_condact(func - BASE);
			table += 2;
		}
	}

}

void show_obj() { show_txttab(3, 0x0b, "Object"); }
void show_loc() { show_txttab(4, 0x0d, "Location"); }
void show_msg() { show_txttab(5, 0x0f, "Message"); }
void show_sys() { show_txttab(6, 0x11, "System message"); }

void show_connections()
{
	int n;
	zword table = peekw(0x13) - BASE;
	zword conn;

	for (n = 0; n < qdb[4]; n++)
	{
		conn = peekw(table + 2 * n) - BASE;

		printf("%04X\tConnections for room %d\n",
			conn + BASE, n);
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
	zword table = peekw(0x17) - BASE;
	zbyte loc;

	for (n = 0; n < qdb[3]; n++)
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
	zword table = peekw(0x19) - BASE;
	zbyte noun;

	for (n = 0; n < qdb[3]; n++)
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

	zword table = peekw(0x15) - BASE;

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
	int n;
	FILE *fp;

	for (n = 1; n < argc; n++)
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

	printf("; Generated by QDBList v0.2\n\n[Header]\n");
	printf("0F00\t%02x\t; Unknown\n", qdb[0]);
	printf("0F01\t%02x\t; Version\n", qdb[1]);
	printf("0F02\t%02x\t; Debug mode?\n", qdb[2]);
	printf("0F03\t%02x\t; %d objects\n", qdb[3], qdb[3]);
	printf("0F04\t%02x\t; %d locations\n", qdb[4], qdb[4]);
	printf("0F05\t%02x\t; %d messages\n", qdb[5], qdb[5]);
	printf("0F06\t%02x\t; %d system messages\n", qdb[6], qdb[6]);
	printf("0F07\t%02x %02x\t; Response table at 0x%04x\n", qdb[7], qdb[8],
				peekw(7));
	printf("0F09\t%02x %02x\t; Process table at 0x%04x\n", qdb[9], qdb[10],
				peekw(9));
	printf("0F0B\t%02x %02x\t; Objects at 0x%04x\n", qdb[11], qdb[12], peekw(11));
	printf("0F0D\t%02x %02x\t; Locations at 0x%04x\n", qdb[13], qdb[14], peekw(13));
	printf("0F0F\t%02x %02x\t; Messages at 0x%04x\n", qdb[15], qdb[16], peekw(15));
	printf("0F11\t%02x %02x\t; System messages at 0x%04x\n", qdb[17], qdb[18], peekw(17));
	printf("0F13\t%02x %02x\t; Connections at 0x%04x\n", qdb[19], qdb[20], peekw(19));
	printf("0F15\t%02x %02x\t; Vocabulary at 0x%04x\n", qdb[21], qdb[22], peekw(21));
	printf("0F17\t%02x %02x\t; Object locations at 0x%04x\n", qdb[23], qdb[24], peekw(23));
	printf("0F19\t%02x %02x\t; Object words at 0x%04x\n", qdb[25], qdb[26], peekw(25));
	printf("0F1B\t%02x %02x\t; Database ends at 0x%04x\n", qdb[27], qdb[28], peekw(27));

	if (!skipresp)
	{
		printf("\n[Response table]\n");
		show_logic(7, 1);
	}
	if (!skipproc)
	{
		printf("\n[Process table]\n");
		show_logic(9, 0);
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

