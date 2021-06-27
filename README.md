# The **S**imple **M**anpage **G**enerator (smg)

This is a simple manpage generator that uses a markdown-like syntax. Program usage is extremely
simple. It takes an `smg` formatted file as its first and only parameter, and outputs the `man`
formatted text to standard output.

Here is a basic example of how it can be used:
```
$ cat example
@name SMG
@date 2021-06-16
@section 1
@version 1.0.0
@title General Commands Manual

# This is a heading
This is some **bold text**

## This is a sub-heading
With some __underlined text__
EOF
$ smg example
.TH "SMG" "1" "2021-06-16" "1.0.0" "General Commands Manual"
.PP
.SH This is a heading
This is some \fBbold text\fR
.PP
.SS This is a sub-heading
With some \fIunderlined text\fR
$
```

An example of the used syntax can be found in the `tests/` directory.

## Compilation Instructions

Compilation is simple:
```
$ make
...
$
```

If you want to install the binary and manpages:
```
# make install
...
#
```
