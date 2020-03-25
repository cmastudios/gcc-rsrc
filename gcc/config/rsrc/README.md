How did I made this?

1. I copied all the files from the 'moxie' target
2. I set up the target definitions in rsrc.c/h with values that made sense.
	I made certain decisions, such as storing return addresses in r15, hiding
	r0 from the compiler to prevent displacement-based addressing issues (and
	allowing me to use it as a scratch register whenever), and storing frame
	and stack pointers at r30, r31.
3. I worked on rsrc.md with the help of the GCC Internals manual, which
	describes what the compiler actually needs out of the machine description,
	as well as definitions for all of the lisp operations used.
4. I tried to get libgcc to build, changing instructions as necessary. It would
	not configure for a while because one of my register definitions was wrong,
	preventing DWARF (the debugger support) from building.

Known Bugs

1. Does not support storing 8-bit or 16-bit numbers in memory.
2. Does not correctly perform comparisons with unsigned numbers
3. Probably many more because I don't know what I'm doing

