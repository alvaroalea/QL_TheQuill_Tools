# Sinclair QL Quill database file format (v0.1)

This database is used by the Quill adventure system under Sinclair QL. It's not used on  the  distribution of games, but is used by Quill to store the source code of the games.

This document has been produced studing the saved files from version A07 of the quill for Sinclair QL. so I may well have missed aspects of the file format not covered by this particular implementation.

To abreviate, I refer as _QLdb to this files, even extension on filename is not necesary.

Memory address in this document are references to the initial of the .QLdb file, so it's assumed to start at  memory 0x0000. It begins with a header:

DB is a byte (8 bits)
DW is a 32 bits word (4 bytes)

```
0x00	DB	0x00		;Unknown -- zero in both CAVE1.QDB and CAVE2.QDB
0x01	DB	0x01		;Database version, 1
0x02	DB	0x01		;Ink Color, 1
0x03	DB	0x01		;Paper Color , 1
0x04	DB	0x01		;Border Width, 1
0x05	DB	0x01		;Border Color, 1
0x06	DB	0x01		;Conveyable objects, 1
0x07	DB	objcount	;Number of objects
0x08	DB	loccount	;Number of locations
0x09	DB	msgcount	;Number of messages
0x0A	DB	syscount	;Number of system messages
0x0B	DB	debugmode	;Nonzero in debug mode (interpreter prints a
				        ;banner at startup, and issues warnings if an
				        ;out-of-range object / location / flag is 
			        	;accessed). TO BE CONFIRMED.
0x0C	DW	response	;Address of response table
0x10	DW	process		;Address of process table
0x14	DW	objects		;Address of objects table
0x18	DW	locations	;Address of locations table
0x1C 	DW	messages	;Address of messages table
0x20	DW	sysmsg		;Address of system messages table
0x24	DW	connections	;Address of connections table
0x28	DW	vocab		;Address of vocabulary table
0x2C	DW	ob_locs		;Address of object initial locations table
0x30	DW	ob_words	;Address of object words table
0x34	DW	end	    	;Address of database end
0x38    DW  ramtop      ;Size of the allocated memory

```

The format of the individual tables is then:
## Process / Response

The Process table is executed before user input, the Response after. Both table has four bytes per entry:

```
	DB	word_id	;Verb to match, 0xFF is a wildcard
	DB	word_id	;Noun to match, 0xFF is a wildcard
	DW	address	;Address of bytecode
```

Its not clear if in process table all entries are executed with no checking of verb / noun.

The end of the table is marked by an all-zeroes entry.

Diferent from CP/M version and PAW, similar to earlier versions of the Quill, there is distinction at the bytecode level between conditions and actions; they are separated in two blocks, first conditions and later actions, each block end is marked with 0xFF

Conditions are:

```
0x00 loc: AT loc
    Execution continues if the player is at the specified location.
0x01 loc: NOTAT loc
    Execution continues if the player is not at the specified location.
0x02 loc: ATGT loc
    Execution continues if the player's location number is higher than the number specified.
0x03 loc: ATLT loc
    Execution continues if the player's location number is lower than the number specified.
0x04 obj: PRESENT obj
    Execution continues if the specified object is in the same location as the player, carried, or worn.
0x05 obj: ABSENT obj
    Execution continues if the specified object is not in the same location as the player, carried, or worn.
0x06 obj: WORN obj
    Execution continues if the player is wearing the specified object.
0x07 obj: NOTWORN obj
    Execution continues if the player is not wearing the specified object.
0x08 obj: CARRIED obj
    Execution continues if the player is carrying the specified object.
0x09 obj: NOTCARR obj
    Execution continues if the player is not carrying the specified object.
0x0A number: CHANCE number
    Execution continues if a pseudo-random number is less than number.
0x0B flag: ZERO flag
    Execution continues if the specified flag is zero.
0x0C flag: NOTZERO flag
    Execution continues if the specified flag is not zero.
0x0D flag const: EQ flag const
    Execution continues if the specified flag has the correct value.
0x0E flag const: GT flag const
    Execution continues if the specified flag has a value greater than the amount indicated.
0x0F flag const: LT flag const
    Execution continues if the specified flag has a value less than the amount indicated.
0xFF
    End of bytecode sequence.
```

Actions are:

```
0x01: INVEN
    Print the player's inventory.
0x01: DESC
    Stop bytecode processing, describe the player's current location and prompt for a new input.
0x03: QUIT
    Ask the player if they want to quit. Execution continues if they say yes. 
0x04: END
    Stop bytecode processing, display the game over message, and ask if the player wants another go. If they do, restart the game; otherwise, return to CP/M.
0x05: DONE
    Stop bytecode processing and prompt for a new input.
0x06: OK
    Display system message 15 ("OK") and behave as DONE.
0x07: ANYKEY
    Display system message 16 ("Press any key to continue") and wait for a keypress.
0x08: SAVE
    Prompt for a filename to save the game state to, and save it.
0x09: LOAD
    Prompt for a filename to load the game state from, and load it.
0x0A: TURNS
    Display the number of turns the player has taken.
0x0B: SCORE  FIXME:TO CONFIRM
    Display the player's score.
0x0C: CLS
    Clear the screen.
0x0D: DROPALL  FIXME:TO CONFIRM
    Move everything the player is carrying to the current location.
0x0E: AUTOG
    Try to match the current noun with an object number. If successful, execute GET on that object.
0x0F: AUTOD
    Try to match the current noun with an object number. If successful, execute DROP on that object.
0x10: AUTOW  FIXME:TO CONFIRM
    Try to match the current noun with an object number. If successful, execute WEAR on that object.
0x11: AUTOR  FIXME:TO CONFIRM
    Try to match the current noun with an object number. If successful, execute REMOVE on that object.
0x12 ticks: PAUSE ticks  FIXME:TO CONFIRM
    Delay for roughly ticks/50 seconds.
0x13: BELL  FIXME:TO CONFIRM
    Sound the beeper by writing character 7 (BEL) to the screen.
0x14: ??

0x15: ??

0x16 loc: GOTO loc
    Move the player to the specified location.
0x17 msg: MESSAGE msg
    Display the specified message.
0x18 obj: REMOVE obj  FIXME:TO CONFIRM
    If the specified object is worn, move it to the player's inventory. If this isn't possible, display an error saying why.
0x19 obj: GET obj
    If the specified object is in the current location, move it to the player's inventory. If this isn't possible, display an error saying why.
0x1A obj: DROP obj
    If the specified object is in the player's inventory, move it to the current location. If this isn't possible, display an error saying why.
0x1B obj: WEAR obj
    If the specified object is in the player's inventory, the player wears it. If this isn't possible, display an error saying why.
0x1C obj: DESTROY obj
    Destroy the specified object. If it was in the player's inventory, the number of objects carried decreases by 1.
0x1D obj: CREATE obj
    Move the specified object to the current location. If it was in the player's inventory, the number of objects carried decreases by 1 (fixing a bug in earlier Quill and Quill-like engines.)
0x1E obj1 obj2: SWAP obj1 obj2
    Exchange the locations of the two objects.
0x1F obj loc: PLACE obj loc
    Move the specified object to the specified location.
0x20 flag: SET flag
    Set the specified flag to 255.
0x30 flag: CLEAR flag
    Set the specified flag to 0.
0x31 flag amount: PLUS flag amount
    Add amount to the specified flag. If the total exceeds 255 it will be capped at 255.
0x32 flag amount: MINUS flag amount
    Subtract amount from the specified flag. If the total is less than zero it will be set to zero.
0x33 flag amount: LET flag amount
    Set the value of the specified flag to amount.
0x34: NEWLINE
    Print a carriage return / line feed.
0x35 flag: PRINT flag
    Print the value of a flag as a decimal number.
0x36 msg: SYSMESS msg
    Display the specified system message.
0x37 obj loc: ISAT obj loc
    Execution continues if the specified object is at the specified location.
0x38 obj flag: COPYOF obj flag
    Set flag to the location of the specified object.
0x39 obj1 obj2: COPYOO obj1 obj2
    Move object obj2 to the location of obj1.
0x3A flag obj: COPYFO flag obj
    Move object obj to the location specified in flag.
0x3B flag1 flag2: COPYFF flag1 flag2
    Set flag2 to the value of flag1.
0x3C: ISDESC
    Execution continues if the location description has been displayed this turn.
0x3D nn: EXTERN nn
    Call external code, passing the supplied byte to it as a parameter. This requires the external code to be added to the interpreter; by default it is a no-op.
0xFF
    End of bytecode sequence.
```

## Text tables

The object, location, message and system message tables are all stored in the same way. The corresponding word in the header points to a table of words giving the addresses of the object/location/message strings.

The strings are stored with each byte complemented (XORed with 0xFF) as a protection against casual snooping. End of string is indicated by 0xF5, which complemented is 0x0D (carriage return). The only other control code I have seen used is 0x0A (newline).

Regarding character set: since CP/M has no standard character set beyond ASCII, I would expect QDB files only to contain ASCII and not to make assumptions about characters 0x80 and up.
Connections

As with location texts, the word in the header points to a table of words with one entry per location. Each word points to a list of connections:

```
	DB	word_id		;Direction
	DB	location	;Where it leads to	
```

The list is terminated by a word_id of 0xFF.

## Vocabulary

The vocabulary table contains five bytes per word. The first four are the word (complemented ASCII) and the fifth is its word_id. Words 1-12 are used for movement verbs.

The end of the table is marked by two entries:
- an entry containing "*   ", 0xFF.
- an entry containing five zero bytes.
## Object initial locations

One byte per object giving its start location. Values of 0xFC or greater mean:

```
0xFC:	Object destroyed
0xFD:	Object worn
0xFE:	Object carried
```

The table must be followed by a 0xFF byte.

## Object words

One byte per object giving the word_id for that object — used by the AUTOG / AUTOD / AUTOW / AUTOR operations. For example, in the Very Big Cave Adventure, object 4 ("A shiny brass lamp") has word 20 (LAMP) as its name. Objects with no name use a word_id of 0xFF.

## Format of saved game state

The game state is saved as a 512-byte file with the extension .QGP. The first 256 bytes hold flags:

```
	DB	game flags	;Flags  0-29: 30 bytes
	DB	score		;Flag  30:    Score
	DW	turns		;Flags 31-32: Number of turns
	DB	verb		;Flag  33: Current verb	
	DB	noun		;Flag  34: Current noun
	DB	word3		;Flag  35: Third recognised word
	DB	word4		;Flag  36: Fourth recognised word
	DB	ability		;Flag  37: Maximum number of objects player
    				;          can carry
	DB	location	;Flag  38: Player's current location	
;
; Flags 39-255 appear not to be used and contain zeroes.
;
```

The second 256 bytes hold the locations of the 256 objects, with the following special meanings:

```
0xFC:	Object destroyed
0xFD:	Object worn
0xFE:	Object carried
```

## Executable Game

TODO

