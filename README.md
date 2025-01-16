# CORE32
A small, stack-based virtual machine.

## Format
Instruction format: `ooooorff`

Opcodes (`o`):

```
00000 data  01000 -     10000 !=    11000 jump
00001 pop   01001 +     10001 ==    11001 call
00010 nip   01010 /     10010 >     11010 ret
00011 dupe  01011 *     10011 >>    11011 int
00100 swap  01100 |     10100 --    11100 get
00101 over  01101 ^     10101 <     11101 set
00110 from  01110 &     10110 <<    11110 else
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