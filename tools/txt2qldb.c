/* txt2qdb: Assemble PAW-style source to a database compatible with
           the Sinclair QL version of the "Quill" adventure game system.
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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

typedef uint8_t zbyte;
typedef uint32_t zword;

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
#define MAX_FLAGS 64

#define BASE 0x000
#define MAXLINE 257	/* Maximum line length */

zbyte qdb[0xFFFF];
zword write_ptr = 0x3c;

char *infile = NULL;
char *outfile = NULL;
char nullword = '_';

int fixup     = 0;
int debugmode = 0;

FILE *fpin;
FILE *fpout;

/* File sections */
int	ctl_done = 0;	/* CTL section processed? */
long	ctl_pos;	int ctl_line;
long	voc_pos;	int voc_line;
long	stx_pos;	int stx_line;
long	mtx_pos;	int mtx_line;
long	otx_pos;	int otx_line;
long	ltx_pos;	int ltx_line;
long	con_pos;	int con_line;
long	obj_pos;	int obj_line;
long	pro0_pos;	int pro0_line;
long	pro2_pos;	int pro2_line;
char	lnk_name[MAXLINE];
char	lnkfile[MAXLINE];

static const char *def_sysmsg[] =
{
/* 0 */	 "Everything is dark. I can't see.",
/* 1 */	 "I can also see:",
/* 2 */	 "I await your command.", 
/* 3 */	 "I'm ready for your instructions.", 
/* 4 */	 "Tell me what to do.", 
/* 5 */  "Give me your command.",
/* 6 */	 "Sorry, I don't understand that. Try some different words.",
/* 7 */	 "I can't go in that direction.",
/* 8 */	 "I can't.",
/* 9 */	 "I have with me:-",
/* 10 */  " (worn)",
/* 11 */ "Nothing at all",
/* 12 */ "Do you really want to quit now?",
/* 13 */ "*** End of Game ***\n\nDo you want to try again?",
/* 14 */ "Bye. Have a nice day.",
/* 15 */ "OK.",
/* 16 */ "Press RETURN to continue...",
/* 17 */ "You have taken ",
/* 18 */ " turn",
/* 19 */ "s",
/* 20 */ ".",
/* 21 */ "You have scored ",
/* 22 */ ".",
/* 23 */ "I'm not wearing it.",
/* 24 */ "I can't. My hands are full.",
/* 25 */ "I already have it.",
/* 26 */ "It's not here.",
/* 27 */ "I can't carry any more.",
/* 28 */ "I don't have it.",
/* 29 */ "I'm already wearing it.",
/* 30 */ "Y",
/* 31 */ "N",
/* 32 */ ">",
/* 33 */ "File not found",
/* 34 */ "Directory full",
/* 35 */ "Disk full",
/* 36 */ "File name error",
/* 37 */ "Type in name of file.",
/* 38 */ "Close error",
/* 39 */ "Change disc(s) if required, then press RETURN.",
/* 40 */ "File corrupt",
/* 41 */ "Rename error"

};



static void append_byte(zbyte b)
{
	if (write_ptr > sizeof(qdb))
	{
		fprintf(stderr, "%s: Out of database space\n", infile);
		exit(1);
	}
	qdb[write_ptr++] = b;
}

static void pokew(zword addr, zword value)
{
	qdb[addr  ] = ( value & 0xFF000000 )>> 24 ;
	qdb[addr+1] = ( value & 0x00FF0000 )>> 16 ;
	qdb[addr+2] = ( value & 0x0000FF00 )>>  8 ;
	qdb[addr+3] = ( value & 0x000000FF )      ;
}

zword peekw(zword offset)
{
	return 0x1000000 * qdb[offset] + 0x10000 * qdb[offset + 1] + 0x100 * qdb[offset + 2] + qdb[offset + 3];
}


/* There are three types of line in a PAW script (and 
 * therefore this system):
 *    Control: First character is a /
 *    Comment: First character is a ;
 *    Data:    Anything else 
 */

static int curline = 0;
static zbyte linebuf[MAXLINE];

static void makeupper(char *p)
{
	while (*p)
	{
		if (islower(*p)) *p = toupper(*p);
		++p;
	}
}

/* Trim specified character(s) from the end of a string */
static void rtrim(char *p, char snip)
{
	char *q = p + strlen(p) - 1;

	while (q >= p && q[0] == snip)
	{
		q[0] = 0;
		--q;
	}
}



char * nextline()
{
	char *p;
	int n;

	do
	{
		if (!fgets(linebuf, sizeof(linebuf) - 1, fpin)) return NULL;
		++curline;
		/* Chop off any terminating CR / LF */
		rtrim(linebuf, '\n');
		rtrim(linebuf, '\r');
		if (linebuf[0] == ';') continue;	/* Skip over lines that are entirely comments */
		if (linebuf[0] == '/')			/* Control line */
		{
			p = strchr(linebuf, ';');
			if (p) *p = 0;			/* Remove any comment */
			rtrim(linebuf, ' ');
			rtrim(linebuf, '\t');			 /* Trim off any trailing whitespace. */

			/* If this is not a LNK line, force it to uppercase */
			for (n = 0; n < 4 && linebuf[n]; n++)
			{
				linebuf[n] = toupper(linebuf[n]);
			}
			if (strncmp(linebuf, "/LNK", 4)) makeupper(linebuf);
		}
		return linebuf;
	}
	while (1);		
	return NULL;	/* Never happens */
}

int issep(char c) { return (c == ' ' || c == '\t'); }
int ishexdigit(char c) { return (isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')); }
int xdigit(char c) 
{
	if (isdigit(c)) return c - '0';
	return toupper(c) - 'A' + 10;
}


static void pass1(void)
{
	char heading[MAXLINE];
	char *data;
	int n, npro;

	ctl_pos = voc_pos = stx_pos = mtx_pos = otx_pos = ltx_pos = con_pos = 
	obj_pos = pro0_pos = pro2_pos = -1;
	lnk_name[0] = 0;
	rewind(fpin);
	curline = 0;
	while (nextline())
	{
		if (linebuf[0] != '/') continue;	/* Data or comment line */
		if (!isalpha(linebuf[1])) continue;	/* Start of text block */

		for (n = 0; linebuf[n] && !issep(linebuf[n]); n++)
		{
			heading[n] = linebuf[n];
		}
		heading[n] = 0;

		if (!strcmp(heading, "/CTL"))      { ctl_pos = ftell(fpin); ctl_line = curline; }
		else if (!strcmp(heading, "/VOC")) { voc_pos = ftell(fpin); voc_line = curline; }
		else if (!strcmp(heading, "/STX")) { stx_pos = ftell(fpin); stx_line = curline; }
		else if (!strcmp(heading, "/MTX")) { mtx_pos = ftell(fpin); mtx_line = curline; }
		else if (!strcmp(heading, "/OTX")) { otx_pos = ftell(fpin); otx_line = curline; }
		else if (!strcmp(heading, "/LTX")) { ltx_pos = ftell(fpin); ltx_line = curline; }
		else if (!strcmp(heading, "/CON")) { con_pos = ftell(fpin); con_line = curline; }
		else if (!strcmp(heading, "/OBJ")) { obj_pos = ftell(fpin); obj_line = curline; }
		else if (!strcmp(heading, "/PRO"))
		{
			data = linebuf;
			while (!issep(*data)) ++data;
			while (issep(*data)) ++data;
		
			npro = atoi(data);	
			if (!isdigit(*data) || (npro != 0 && npro != 2))
			{
				fprintf(stderr, "%s(%d): /PRO must be followed by 0 or 2\n",
						infile, curline);
				exit(1);
			}
			if (npro == 0) { pro0_pos = ftell(fpin); pro0_line = curline; }
			if (npro == 2) { pro2_pos = ftell(fpin); pro2_line = curline; }
		}
		else if (!strcmp(heading, "/LNK"))
		{
			data = linebuf;
			while (!issep(*data)) ++data;
			while (issep(*data)) ++data;
			if (!*data || *data == ';')
			{
				fprintf(stderr, "%s(%d): /LNK must be followed by a filename\n",
						infile, curline);
			}
			strcpy(lnk_name, data);
			data = lnk_name;
			/* Token ends at whitespace or ; */
			while (*data)
			{
				if (issep(*data) || *data == ';') { *data = 0; break; }
				++data;
			}	
		}
		else
		{
			fprintf(stderr, "%s(%d): Unknown section heading '%s' (ignored)\n",
					infile, curline, heading);
		}
	}	
}


static void parse_ctl(void)
{
	int state = 0;

	if (ctl_pos < 0)
	{
		fprintf(stderr, "%s: No CTL section found\n", infile);
		exit(1);
	}

	fseek(fpin, ctl_pos, SEEK_SET); curline = ctl_line;
	while (nextline())
	{
		if (linebuf[0] == ';') continue;	/* Comment line */
		if (linebuf[0] == '/')
		{
			fprintf(stderr, "%s(%d): Premature end of /CTL section\n", infile, curline);
			exit(1);
		}
		if (state == 0 && (linebuf[0] == 'q' || linebuf[0] == 'Q'))
		{
			state = 1;
		}
		else if (state == 0)
		{
			fprintf(stderr, "%s(%d): CTL: Data drive must be Q to indicate Quill data\n", infile, curline);
			exit(1);
		}
		else if (state == 1 && isprint(linebuf[0]) && !isalnum(linebuf[0]))
		{
			nullword = linebuf[0];	
			ctl_done = 1;
			return;
		}
		else if (state == 1 && isalpha(linebuf[0]))
		{
			fprintf(stderr, "%s(%d): CTL: Null word '%c' is alphanumeric\n", 
					infile, curline, linebuf[0]);
			exit(1);
		}	
		else if (state == 1)
		{
			fprintf(stderr, "%s(%d): CTL: Null word 0x%02x is not printable\n", 
					infile, curline, linebuf[0]);
			exit(1);
		}	
	}	
	fprintf(stderr, "%s(%d): Premature end of /CTL section\n", infile, curline);
	exit(1);
}

char *get_word(char *data, char *result, int *word_id)
{
	int n;
	unsigned vocbase;
	char match[5];

	if (word_id) *word_id = -1;
	strcpy(result, "    ");
	/* Skip any whitespace before the word */
	while (issep(*data)) ++data;

	/* An extension: Allow (number) to specify a
	 * word ID that isn't in the vocabulary table */
	if (*data == '(' && word_id)
	{
		++data;
		*word_id = atoi(data);
		while (isdigit(*data)) ++data;
		if (*data == ')') ++data;
		return data;
	}
	/* "_" is a null word */
	if (*data == nullword)
	{
		++data;
		result[0] = nullword;	
		*word_id = 0xFF;
		return data;
	}

	if (*data == 0 || !isalnum(*data)) 
	{
		return NULL;
	}
	for (n = 0; n < 4; n++)
	{
		if (issep(*data) || *data == ';' || *data == 0) break;

		if (isalnum(*data)) result[n] = toupper(*data);
		else return NULL;

		++data;
	}
	/* If we want to know the word ID, look it up */
	if (word_id)
	{
		if (!peekw(OFF_T_VOC))
		{
			fprintf(stderr, "%s(%d): Trying to access the vocabulary"
				" but none is loaded\n", infile, curline);
			exit(1);
		}
		vocbase = peekw(OFF_T_VOC) - BASE;

		while (qdb[vocbase + 4] != 0)
		{
			memcpy(match, qdb + vocbase, 4);
			match[0] = ~match[0];
			match[1] = ~match[1];
			match[2] = ~match[2];
			match[3] = ~match[3];
			match[4] = 0;
			if (!strcmp(result, match))
			{
				*word_id = qdb[vocbase + 4];
				break;
			}
			vocbase += 5;
		}	
	}

	return data;
}


static void parse_voc(void)
{
	zbyte *data, *p;
	zbyte wrd[5];
	int n, word_id;
	zword vocbase = write_ptr;	
	
	pokew(OFF_T_VOC, vocbase + BASE);
	fseek(fpin, voc_pos, SEEK_SET); curline = voc_line;
	while (nextline())
	{
		if (linebuf[0] == '/') break;	/* End of section */

		/* Skip leading whitespace */
		data = linebuf;
		while (issep(*data)) ++data;
		p = get_word(data, wrd, &word_id);

		if (p && wrd[0] == nullword)
		{
			fprintf(stderr, "%s(%d): Warning in VOC: Ignoring the null word\n",
				infile, curline);
			continue;
		}
		if (!p)
		{
			fprintf(stderr, "%s(%d): VOC: Expecting a word at '%s'",
				infile, curline, data);
			exit(1);
		}
		data = p;
/* Check for duplicates */
		if (word_id >= 0)
		{
			fprintf(stderr, "%s(%d): VOC: Word '%s' is already present\n", 
				infile, curline, wrd);
			exit(1);
		}
		while (issep(*data)) ++data;
		n = atoi(data);
		if (n < 1 || n > 254)
		{
			fprintf(stderr, "%s(%d): VOC: Word '%s' must be numbered 1-254\n", 
				infile, curline, wrd);
			exit(1);
		}
		//fprintf(stderr, "found word %i: %c%c%c%c\n",n,wrd[0],wrd[1],wrd[2],wrd[3]);
		append_byte(~wrd[0]);
		append_byte(~wrd[1]);
		append_byte(~wrd[2]);
		append_byte(~wrd[3]);
		append_byte(n);
	}
	//FIXME check if QL add a void word at end.
	append_byte(~'*');append_byte(~' ');append_byte(~' ');append_byte(~' ');append_byte(0xFF);
	for (n = 0; n < 5; n++) append_byte(0);	/* End marker */
}

static void parse_tx(long tx_pos, int tx_line, 
	unsigned otable, unsigned ocount, int sys)
{
	zword table[256];
	zword txbase = write_ptr;
	int ntx = 0;
	int txmax = 0;
	int begin = 0;
	char *data;
	zbyte hexbyte;
	int n;
	int state;

	fseek(fpin, tx_pos, SEEK_SET); curline = tx_line;
	state = 0;	/* Not in a message */
	while (nextline())
	{
		if (linebuf[0] == '/') 	/* A control line */
		{
			if (!isdigit(linebuf[1])) break;	/* End of section */

			if (state == 1)		/* Message under construction */
			{
				append_byte(0xFF);			
				txbase = write_ptr;
				state = 0;	
			}
			ntx = atoi(linebuf + 1);
			if (ntx != txmax)
			{
				fprintf(stderr, "%s(%d): Expected next string to be number %d\n",
						infile, curline, txmax);
				exit(1);
			}
			if (txmax < ntx + 1) txmax = ntx + 1;
			table[ntx] = txbase;
			begin = 1;
			state = 1;
		}
		else	/* A data line */
		{
			if (state == 0)
			{
				fprintf(stderr, "%s(%d): String data before string number: %s\n",
						infile, curline, linebuf);
			}
			else
			{
				/* If there was a previous line end it with a newline */
				if (!begin) append_byte(~0x0A);	
				begin = 0;
				for (data = linebuf; data[0]; ++data)
				{
					if (data[0] != '\\')
					{
						append_byte(~data[0]);
					}
					else
					{
/* \+ and \- are used by txt2adv to indicate "inverse on" and "inverse off". 
 * Not supported by QDB, so we skip them */
//FIXME: QL suppport it. 
//FIXME: extra chars here for SINclair QL
						if (data[1] == '+' || data[1] == '-')
						{
							++data;
						}
						else if (data[1] == '0' && data[2] == 'x')
						{
							data += 3;
							hexbyte = 0;
							if (ishexdigit(data[0])) 
							{
								hexbyte = xdigit(data[0]);
								++data;
								if (isxdigit(data[0]))
								{
									hexbyte = (hexbyte << 4) | xdigit(data[0]);
									++data;
								}
							}
							append_byte(~hexbyte);
						}
						else if (data[1])	/* Literal character */
						{
							append_byte(~data[1]);
							++data;
						}
					}	
				}
			}	
			
		}
	}
	if (state == 1)		/* Terminate the message under construction */
	{
		append_byte(0xFF);			
	}
/* If the script supplies fewer than 32 system messages, we have
 * the option to supply the deficit from defaults */
//FIXME: check if in QL there are 32 messages or more for save/load msg
	if (sys && txmax < 32)
	{
		if (fixup)
		{
			while (txmax < 32)
			{
				table[txmax] = write_ptr;
				data = (char *)def_sysmsg[txmax];
				while (*data)
				{	
					append_byte(~*data);
					++data;
				}
				append_byte(~'\r');
				++txmax;
			}
		}
		else
		{
			fprintf(stderr, "%s: Warning: Fewer than 32 system messages.\n"
				"Recompile with -f to add default messages for the missing entries.\n", infile);
		}
	}

	pokew(otable, write_ptr + BASE);
	qdb[ocount] = txmax;
	for (n = 0; n < txmax; n++)
	{
		table[n] += BASE;
		append_byte((table[n] & 0xFF000000 )>>24);	
		append_byte((table[n] & 0x00FF0000 )>>16);	
		append_byte((table[n] & 0x0000FF00 )>> 8);	
		append_byte((table[n] & 0x000000FF )    );	
	}
}

static void parse_con(void)
{
	char word[5];
	zword table[256];
	zword conbase = write_ptr;
	int ncon = 0;
	int conmax = 0;
	char *data, *p;
	int n, word_id, loc_id;
	int state;

	fseek(fpin, con_pos, SEEK_SET); curline = con_line;
	state = 0;	/* Not in a location */
	while (nextline())
	{
		if (linebuf[0] == '/') 	/* A control line */
		{
			if (!isdigit(linebuf[1])) break;	/* End of section */

			if (state == 1)		/* Connection under construction */
			{
				append_byte(0xFF);
				conbase = write_ptr;
				state = 0;	
			}
			ncon = atoi(linebuf + 1);
			if (ncon != conmax)
			{
				fprintf(stderr, "%s(%d): Expected next connection to be number %d\n",
						infile, curline, conmax);
			}
			if (conmax < ncon + 1) conmax = ncon + 1;
			table[ncon] = conbase;
			state = 1;
		}
		else	/* A data line */
		{
			if (state == 0)
			{
				fprintf(stderr, "%s(%d): Connection data before connection number: %s\n",
						infile, curline, linebuf);
			}
			else
			{
				data = linebuf;
				/* Skip whitespace */
				p = get_word(data, word, &word_id);
				if (word_id < 0 || !p || word[0] == nullword)
				{
					fprintf(stderr, "%s(%d): Expecting a word at '%s'\n",
							infile, curline, data);
					exit(1);
				}	
				data = p;
				while (issep(*data)) ++data;

				if (isdigit(*data)) loc_id = atoi(data);
				else 
				{
					fprintf(stderr, "%s(%d): Expecting a location number at '%s'\n",
							infile, curline, data);
					exit(1);
				}	
				if (qdb[4] != 0 && loc_id >= qdb[4])
				{
					fprintf(stderr, "%s(%d): Location number %d out of range\n",
							infile, curline, loc_id);
				}
				append_byte(word_id);
				append_byte(loc_id);
			}	
			
		}
	}
	if (state == 1)		/* Terminate the message under construction */
	{
		append_byte(0xFF);			
	}
	pokew(OFF_T_CON, write_ptr + BASE);
	for (n = 0; n < conmax; n++)
	{
		table[n] += BASE;
		append_byte((table[n] & 0xFF000000 )>>24);	
		append_byte((table[n] & 0x00FF0000 )>>16);	
		append_byte((table[n] & 0x0000FF00 )>> 8);	
		append_byte((table[n] & 0x000000FF )    );	
	}

}


static void parse_obj(void)
{
	char word[5];
	zbyte tab1[256];
	zbyte tab2[256];
	int nobj, nloc, maxobj, n, word_id;
	char *data, *p;

	memset(tab1, 0xFC, sizeof(tab1));
	memset(tab2, 0xFF, sizeof(tab2));

	maxobj = 0;
	fseek(fpin, obj_pos, SEEK_SET); curline = obj_line;
	while (nextline())
	{
		if (linebuf[0] == '/') 	/* A control line */
		{
			if (!isdigit(linebuf[1])) break;	/* End of section */

			nobj = atoi(linebuf + 1);

			if (nobj >= maxobj) maxobj = nobj + 1;
			if (nobj < 0 || nobj >= 252 || (qdb[3] && nobj >= qdb[3]))
			{
				fprintf(stderr, "%s(%d): Object number %d out of range\n",
						infile, curline, nobj);
			}
			data = linebuf + 1;
			/* Skip digits */
			while (isdigit(*data)) ++data;
			/* Skip whitespace */
			while (issep(*data)) ++data;
			/* The next token should be a location */
			if (isdigit(*data))	
			{
				nloc = atoi(data);
				while (isdigit(*data)) ++data;
			}
			else if (*data == nullword)
			{
				nloc = 252;
				++data;
			}
			else if (!strncmp(data, "WORN", 4))
			{
				nloc = 253;
				data += 4;
			}
			else if (!strncmp(data, "CARRIED", 7))
			{
				nloc = 254;
				data += 7;
			}
			else
			{
				fprintf(stderr, "%s(%d): Expecting a location at '%s'\n", infile, curline, data);
				exit(1);
			}
			if (qdb[4] != 0 && nloc >= qdb[4] && nloc < 252)
			{
				fprintf(stderr, "%s(%d): Location number %d out of range\n",
						infile, curline, nloc);
			}
			tab1[nobj] = nloc;
			/* Skip whitespace */
			while (issep(*data)) ++data;
			/* The next token should be a word or null */
			p = get_word(data, word, &word_id);
			if (!p)
			{
				fprintf(stderr, "%s(%d): Expecting a word at '%s'\n", infile, curline, data);
				exit(1);
			}
			tab2[nobj] = word_id;
		}
	}
	if (maxobj < qdb[3]) maxobj = qdb[3];
	
	pokew(OFF_T_OBL, write_ptr + BASE);
	for (n = 0; n < maxobj; n++) append_byte(tab1[n]); 
	append_byte(0xFF);	/* To terminate the table */
	pokew(OFF_T_OBW, write_ptr + BASE);
	for (n = 0; n < maxobj; n++) append_byte(tab2[n]);
	append_byte(0x00);	/* To terminate the table */
}

/* CondAct syntax table. Each row is formed token|encoding|arguments. 
 * Arguments can be: f=>flag i=>integer l=>location o=>object m=>message 
 *                   s=>sys message 
 * 
 * Set the encoding to X for CondActs which are supported by other Quill
 * versions but not this one.
 */

static const char *condacts[] =
{
	"AT|0|c|l",
	"NOTAT|1|c|l",
	"ATGT|2|c|l",
	"ATLT|3|c|l",
	"PRESENT|4|c|o",
	"ABSENT|5|c|o",
	"WORN|6|c|o",
	"NOTWORN|7|c|o",
	"CARRIED|8|c|o",
	"NOTCARR|9|c|o",
	"CHANCE|10|c|i",
	"ZERO|11|c|f",
	"NOTZERO|c|12|f",
	"EQ|13|c|fi",
	"GT|14|c|fi",
	"LT|15|c|fi",

		"INVEN|0|a|",
	"DESC|1|a|",
	"QUIT|2|a|",
	"END|3|a|",
	"DONE|4|a|",
	"OK|5|a|",
	"ANYKEY|6|a|",
	"SAVE|7|a|",
	"LOAD|8|a|",
	"TURNS|9|a|",
		"SCORE|10|a|",
	"CLS|11|a|",
		"DROPALL|12|a|",
	"AUTOG|13|a|",
	"AUTOD|14|a|",
		"AUTOW|15|a|",
		"AUTOR|16|a|",
		"PAUSE|17|a|i",
		"INK|18|a|i",
		"PAPER|19|a|i",
		"BORDER|20|a|i",
	"GOTO|21|a|l",
	"MESSAGE|22|a|m",
		"REMOVE|23|a|o",
	"GET|24|a|o",
	"DROP|25|a|o",
		"WEAR|26|a|o",
	"DESTROY|27|a|o",
	"CREATE|28|a|o",
	"SWAP|29|a|oo",
		"PLACE|30|a|ol",
		"SET|31|a|f",
		"CLEAR|32|a|f",
		"PLUS|33|a|fi",
		"MINUS|34|a|fi",
		"LET|35|a|fi",
		"NEWLINE|36|a|",
	"RAMSAVE|37|a|",
	"RAMLOAD|38|a|",
		"PRINT|39|a|f",
		"SYSMESS|40|a|s",
		"SOUND|41|a|ii",
	NULL
};

int parse_condact(char *condact,int isaction)
{
	char token[MAXLINE];
	char *p = token;
	const char *args;
	int n, l, arg;
	int matched;
//	fprintf(stderr,"evaluamos %s\n",condact);

	while (issep(*condact)) ++condact; 	// remove spaces from start of line.
	if (!isalnum(*condact)) 			// if not alfanumeric error
	{
		fprintf(stderr, "%s(%d): Expecting a CondAct but found '%s'\n", 
				infile, curline, condact);
		exit(1);
	}
	while (isalnum(*condact))
	{
		*p++ = *condact++;	
	}
	*p++ = '|';

	while (issep(*condact)) ++condact;
	makeupper(token);
	*p = 0;
	l = strlen(token);
	// So now token is fist word of line in capitals, and condact point to second word/number
//	fprintf(stderr,"token  %s\n",token);
//	fprintf(stderr,"resto  %s\n",condact);

	matched = -1;
	for (n = 0; condacts[n]; n++)
	{
		if (!strncmp(condacts[n], token, l))
		{
			if (condacts[n][l] == 'X') //l is next character after | in found line on table condacts
			{
				rtrim(token, '|');
				fprintf(stderr, "%s(%d): Warning: CondAct '%s' is not supported by this Quill variant\n", 
				infile, curline, token);
				return isaction;
			}

			matched = atoi(&condacts[n][l]);
			args = condacts[n] + l;
			while (isdigit(*args) || '|' == *args) ++args;
			break;
		}	
	}	
	if (matched < 0)
	{
		rtrim(token, '|');
		fprintf(stderr, "%s(%d): Expecting a CondAct but found '%s'\n", 
				infile, curline, token);
		exit(1);
	}

//	fprintf(stderr,"args0  %s\n",args);

/*
	if (args[0] == 'c') 
	{ 
		fprintf(stderr, "Is Command\n");
	}
*/ 
	if (args[0] == 'a') // I add a charecter an a | to diferenciate action of conditions
	{ 
//		fprintf(stderr, "Is Action\n");
		if (isaction == 0) 
		{
		isaction = 1;
		append_byte(0xFF);
		}
	} 
	args ++ ; args++ ;
//	fprintf(stderr,"args1  %s\n",args);

	append_byte(matched);

	while (*args)
	{
		if (!isdigit(*condact))
		{
			fprintf(stderr, "%s(%d): Expecting a digit but found '%s'\n", 
					infile, curline, condact);
			exit(1);
		}
		arg = atoi(condact);
		while (isdigit(*condact)) ++condact;
		while (issep(*condact)) ++condact;

		if (arg < 0 || arg > 255)
		{
			fprintf(stderr, "%s(%d): Numerical value %d is out of range\n", 
					infile, curline, arg);
			exit(1);
		}
		switch(*args)
		{
			case 'f': if (arg > MAX_FLAGS) 
					fprintf(stderr, "%s(%d): Flag number %d out of range\n", 
						infile, curline, arg);
				  break;
			case 'l': if (qdb[4] && arg > qdb[4])
					fprintf(stderr, "%s(%d): Location number %d out of range\n",
						infile, curline, arg);
				  break;
			case 'm': if (qdb[5] && arg > qdb[5])
					fprintf(stderr, "%s(%d): Message number %d out of range\n",
						infile, curline, arg);
				  break;
			case 'o': if (qdb[3] && arg > qdb[3])
					fprintf(stderr, "%s(%d): Object number %d out of range\n",
						infile, curline, arg);
				  break;
			case 's': if (qdb[6] && arg > qdb[6])
					fprintf(stderr, "%s(%d): System message number %d out of range\n",
						infile, curline, arg);
				  break;
		}	
		append_byte(arg);
		++args;
	}
	return isaction;
}


/* Parse a 'process' line, which has to be formed either: 
 * Type 0: Blank 
 * Type 1: Verb Noun Condact 
 * Type 2: Whitespace Condact 
 */
int parse_pro_line(int *wid1, int *wid2, char *condact)
{
	char wbuf[5];
	char *data = linebuf;
	char *p;

	if (0 == *data || ';' == *data)	/* Blank line */
	{
		return 0;
	}
	if (!issep(*data))
	{
		p = get_word(data, wbuf, wid1);
		if (p == NULL || *wid1 < 0)
		{
			fprintf(stderr, "%s(%d): Expected word at '%s'\n", infile, curline, data);
			exit(1);
		} 
		data = p;
		p = get_word(data, wbuf, wid2);
		if (p == NULL || *wid2 < 0)
		{
			fprintf(stderr, "%s(%d): Expected word at '%s'\n", infile, curline, data);
			exit(1);
		} 
		data = p;
	}
	else
	{
		*wid1 = -1;
		*wid2 = -1;
	}
	while (issep(*data)) ++data;
	if (*data == ';' || *data == 0)
	{
		condact[0] = 0;
		return 0;	/* Blank line */
	} 
	strcpy(condact, data);
	if (*wid1 == -1 && *wid2 == -1) return 2;
	return 1;	
}


static void parse_pro(long pos, int line, int ptr, int response)
{
	int word1, word2;
	char condact[MAXLINE];
	int n;
	int state = 0;
	int entries = 0;
	int table_base = write_ptr;
	int isaction= 0;
	int part = 0;

/* First pass: Count the number of entries and store verb and noun */
	fseek(fpin, pos, SEEK_SET);
	curline = line;

	while (nextline())
	{
		if (linebuf[0] == '/') break;	/* End of section */

		n = parse_pro_line(&word1, &word2, condact);
		if (state == 0)
		{
			if (n == 1) 
			{ 
				if (response) // alwais in QL.
				{
					append_byte(word1);
					append_byte(word2);
				}
				append_byte(0);
				append_byte(0);
				append_byte(0);
				append_byte(0);
				++entries; 
				state = 1; 
			}
			else
			{
				fprintf(stderr, "%s(%d): Process "
					"entry '%s' not formed <verb> <noun> <condact>\n",
					infile, curline, linebuf);
				exit(1);
			}
		}
		else // state != 0
		{	
			if (n == 0)	/* End */
			{
				state = 0;
			}
			else if (n != 2)
			{
				fprintf(stderr, "%s(%d): Process "
					"entry '%s' not formed <whitespace> <condact>\n",
					infile, curline, linebuf);
				exit(1);

			}
		}
	}
/* End of table */
	append_byte(0);
	append_byte(0);
	pokew(ptr, table_base + BASE);
	if (entries == 0)
	{
		return;	/* Vacuous */
	}

/* Second pass: Actually parse the entries */
	fseek(fpin, pos, SEEK_SET);
	curline = line;
	entries = 0;
	while (nextline())
	{
		if (linebuf[0] == '/') break;	/* End of section */

		n = parse_pro_line(&word1, &word2, condact);
		if (state == 0)
		{
			if (n == 1) 
			{ 
				if (response)	pokew(table_base + 2 + entries * 6, write_ptr + BASE);
				else		pokew(table_base + entries * 2, write_ptr + BASE);	
				++entries; 
				state = 1;
				isaction = 0; 
			}
			else	/* Should have been rejected in pass 1 */
			{
				fprintf(stderr, "%s(%d): Process "
					"entry '%s' not formed <verb> <noun> <condact>\n",
					infile, curline, linebuf);
				exit(1);
			}
			isaction= parse_condact(condact,isaction);
		}
		else
		{	
			if (n == 0)	/* End */
			{
				state = 0;
				append_byte(0xFF);
			}
			else 
			{
				isaction= parse_condact(condact,isaction);
			}
		}
	}
}



static void syntax(const char *av0)
{
	fprintf(stderr, "Syntax: %s { -d } file.qse file.qdb\n\n", av0);
	fprintf(stderr, "\n-d sets the debug mode flag in the output file.\n");
	exit(1);
}



static void parse_file(void)
{
	do
	{
		fpin = fopen(infile, "r");
		if (!fpin)
		{
			perror(infile);
			exit(1);
		}
/* First pass: Locate the section headings */
		pass1();
/* Locate the CTL section and parse it */
		if (!ctl_done) parse_ctl();
		if (voc_pos >= 0) parse_voc();
		if (obj_pos >= 0) parse_obj();
		if (con_pos >= 0) parse_con();
		if (mtx_pos >= 0) parse_tx(mtx_pos, mtx_line, OFF_T_MSG, OFF_N_MSG, 0);
		if (stx_pos >= 0) parse_tx(stx_pos, stx_line, OFF_T_SMS, OFF_N_SMS, 1);
		if (ltx_pos >= 0) parse_tx(ltx_pos, mtx_line, OFF_T_LOC, OFF_N_LOC, 0);
		if (otx_pos >= 0) parse_tx(otx_pos, mtx_line, OFF_T_OBJ, OFF_N_OBJ, 0);
		if (pro0_pos >= 0) parse_pro(pro0_pos, pro0_line, OFF_T_RES, 1);
		if (pro2_pos >= 0) parse_pro(pro2_pos, pro2_line, OFF_T_PRC, 1);

		fclose(fpin);

		/* If a LNK block was found, repeat on that file */
		if (lnk_name[0])
		{
			strcpy(lnkfile, lnk_name);
			infile = lnkfile;
		}
	}
	while (lnk_name[0]);
}


int main(int argc, char **argv)
{
	int n;

	for (n = 1; n < argc; n++)
	{
		if (!strcmp(argv[n], "-f") || !strcmp(argv[n], "-F")) fixup = 1;
		else if (!strcmp(argv[n], "-d") || !strcmp(argv[n], "-D")) debugmode = 1;
		else if (infile == NULL) infile = argv[n];
		else if (outfile == NULL) outfile = argv[n];
		else syntax(argv[0]);
	}
	if (infile == NULL || outfile == NULL)
	{
		syntax(argv[0]);
	}
/* Initialise the header to zeroes */
	memset(qdb, 0, sizeof(qdb));
	qdb[1] = 1;	/* version */


	parse_file();
	qdb[2] = 7;	/* Ink Color */
	qdb[3] = 0;	/* Paper Color */
	qdb[4] = 1;	/* Border Witdh */
	qdb[5] = 7;	/* Border Color */
	qdb[6] = 4;	/* Conveyable Objects */
	// if (debugmode) qdb[0xB] = 1; //FIXME: not sure if work on QL

	pokew(0x34, write_ptr + BASE);
	//FIXME: Calculate a good memory allocation size
	pokew(0x38, 32768);

	fpout = fopen(outfile, "wb");
	if (!fpout)
	{
		perror(outfile);
		exit(1);
	}
	if ((fwrite(qdb, 1, write_ptr, fpout) < write_ptr) ||
	     fclose(fpout))
	{
		perror(outfile);
		exit(1);
	}
	return 0;
}
