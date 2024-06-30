# QL_TheQuill_Tools
Tools and Adventures source for the The Quill on Sinclair QL

## Introduction
In this repository I will try to create some tools to help in the creation of Adventures with The Quill for the Sinclair QL.

In the path is my intention to document all characteristics of the version for Sinclair QL.

As a second objetive, I will try to port adventures from other systems to the Sinclair QL.

And as a 3º objetive, I will try to create other tools that fix bugs, or enhaced the quill, like the ilustrator in other system, or the tool maluva por DAAD.

And as a final objetive, I will try to create parser and compiler that allow create adventures based on The Paws and DAAD sytems.

## Database

The version for the QL is very similar to the version for CP/M, so the information of John Elliot (here: https://www.seasip.info/Unix/UnQuill/) was essential.

As the CP/M version, QL version store the information in a separate file, so from The Quill you can save the game as a un-editable executable, or the database, that you can load later and edit.

The main diferences between CP/M and QL are:
* store some aditional information in the header of the file (like colors, border and ram allocated)
* use address in 32bits for internal references
* the CONDitions use the same binary code tham CP/M, but the ACTions use a diferente code.

## Roadmap
-[ ] [DONE 75%] Create a QLdb2txt program that dump the database in .QSE format (editable text)
-[ ] [DONE 25%] Document aditional features of The Quill for QL
-[ ] [DONE 50%] Create a TXT2QLDB program that create a database from a .QSE file

*
-[ ] Create some tools that configure files for use in ql (file header)
-[ ] Convert several only-text adventures to QL and test the reability of previous tools.
-[ ] Identify all "only text", "text and full screen images" and "text and images" adventures ever created for QL.
-[ ] Convert all "only text" adventures to QL
-[ ] Translate documents to Spanish
-[ ] Create tools to incorporate graphics to QL Adventures
-[ ] Port all others adventures to QL
-[ ] Create CAAD for QL
-[ ] Create some post on blog about all of this.
