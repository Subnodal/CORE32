# CORE32
A small, stack-based virtual machine and assembly language.

## Building and running
To build the CORE32 runtime and assembler, run:

```bash
./build.sh
```

To run the binary file `$FILE` using the runtime, run:

```bash
runtime/build/core32 $FILE
```

To assemble the assembly code file `$IN_FILE` to `$OUT_FILE`, run:

```bash
assembler/build/coreasm $IN_FILE -o $OUT_FILE
```

This will produce a relocatable C32 executable file. To produce a raw binary file, use the `--raw` switch. Debug information from the assembler can be shown by using the `--debug` switch.

## Format
Instruction format: `ooooorff`

Opcodes (`o`):

```
00000 ret   01000 -     10000 !=    11000 put
00001 drop  01001 +     10001 =     11001 jump
00010 %     01010 /     10010 >     11010 call
00011 dup   01011 *     10011 >>    11011 int
00100 swap  01100 |     10100 --    11100 get
00101 over  01101 ^     10101 <     11101 set
00110 from  01110 &     10110 <<    11110 cif
00111 to    01111 !     10111 ++    11111 if
```

Relativity (`r`):
```
0 absolute
1 relative
```

Data format (`f`):
```
00 float
01 byte
10 short
11 long
```

## Assembly syntax

```
( abc )         ( comment )
123             ( denary number )
123.456         ( decimal number )
0b01011010      ( binary number )
0o123           ( octal number )
0xAB            ( hexadecimal number )
'a'             ( character )
"abc"           ( string )
[123 'a' "abc"] ( raw )
123'            ( force byte )
123"            ( force short )
pop'            ( byte op )
pop"            ( short op )
pop             ( long op )
pop%            ( float op )
{0x1234 get}    ( group )
?{0x1234 get}   ( conditional group )
:{0x1234 get}   ( quoted group )
${0x1234 get}   ( skipped group )
abcdef:         ( define global label )
abcdef          ( call global label )
?abcdef         ( conditionally call global label )
$abcdef         ( address of global label )
.abcdef:        ( define local label )
.abcdef         ( call local label )
?.abcdef        ( conditionally call local label )
$.abcdef        ( address of local label )
$abc.def        ( address of external local label )
abcdef~         ( size of label )
.abcdef~        ( size of local label )
abc.def~        ( size of external local label )
@123            ( set absolute position )
~123            ( offset position )
~abcdef         ( offset by size of label )
~.abcdef        ( offset by size of local label )
~abc.def        ( offset by size of external local label )
#.abcdef        ( get local offset )
#abc.def        ( get local offset from global )
#abc: 123       ( define macro )
#abc            ( get macro )
+code.casm      ( include and assemble )
[+code.casm]    ( include raw data )
```