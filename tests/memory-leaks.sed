# Quick and simply valgrind output parser

s/==[0-9]+==\s+(.* lost: [1-9][0-9]* bytes in [1-9][0-9]* blocks)/\1/
t matched
d
:matched
