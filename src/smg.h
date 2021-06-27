#ifndef __SMG_H_
#define __SMG_H_

#ifdef __GNUC__
#	define ATTRIBUTE_MALLOC __attribute__((malloc))
#else
#	define ATTRIBUTE_MALLOC
#endif

struct MetaData {
	char *name, *section, *date, *version, *title;
};

struct Tag {
	char symbol;
	char *replace;
};

enum ListState { NOT_IN_LIST, FIRST_SUBLIST, ADDITIONAL_SUBLIST };

/* Values from the hash() function */
enum { NAME = 4, SECTION = 12, DATE = 9, VERSION = 7, TITLE = 5 };

#endif
