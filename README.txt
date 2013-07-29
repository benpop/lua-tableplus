Add table/list utilities to Lua.  Pollutes the Lua standard library "table".

Supports Lua 5.1 and 5.2.

Contents:
	[Table]
	count  -->  number of all pairs in table

	[Sequence]
	sum  -->  sum of all numbers in table
	product  -->  product of all numbers in table
	foldl(t, binfunc [, init])  -->  left fold/reduce
	foldr(t, binfunc [, init])  -->  right fold/reduce
	any  -->  do any values evaluate to true?
	all  -->  do all values evaluate to true?
	copy  -->  exact copy.  references stay the same.
	slice  -->  sub-sequence
	map(t, func)  -->  map func to t
	filter(t [, predicate])  -->  keep values that evaluate true
	reversed  -->  reversed list

	[Iterator over sequence]
	imap(t, func)
	ifilter(t [, predicate])
	ireverse

	[Backport to Lua 5.1]
	pack  -->  vararg (...) to table
	unpack  -->  table to vararg (...)

	[Forward port to Lua 5.2]
	foreach  -->  iterate over key/value pairs
	foreachi  -->  iterate over index/value pairs

	[Lua globals]
	ordpairs  -->  iterate over key/value pairs, ordered by keys
	foreach  -->  like table.foreach.  "print" is the default function.
	foreachi  -->  like table.foreachi.  "print" is the default function.

Lua manual on tables, and sequences in particular:

http://www.lua.org/manual/5.2/manual.html#2.1
http://www.lua.org/manual/5.2/manual.html#3.4.6
http://www.lua.org/manual/5.2/manual.html#6.5

##benpop##
