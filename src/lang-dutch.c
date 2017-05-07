#include "num2words.h"

// Translations by Edwin

const Language LANG_DUTCH = {
	.hours = {
		"een",
		"twee",
		"drie",
		"vier",
		"vijf",
		"zes",
		"zeven",
		"acht",
		"negen",
		"tien",
		"elf",
		"twaalf"
	},

#ifdef SCREEN_WIDE
	.phrases = {
		"*$1 uur ",
		"vijf over *$1 ",
		"tien over *$1 ",
		"kwart over *$1 ",
		"tien voor half *$2 ",
		"vijf voor half *$2 ",
		"half *$2 ",
		"vijf over half *$2 ",
		"tien over half *$2 ",
		"kwart voor *$2 ",
		"tien voor *$2 ",
		"vijf voor *$2 "
	},

	.greetings = {
		"Goede morgen ",
		"Goede middag ",
		"Goede avond ",
		"Goede nacht "
	},
#else
	.phrases = {
		"*$1 uur ",
		"vijf over *$1 ",
		"tien  over *$1 ",
		"kwart over *$1 ",
		"tien  voor  half *$2 ",
		"vijf voor half *$2 ",
		"half *$2 ",
		"vijf over half *$2 ",
		"tien  over  half *$2 ",
		"kwart voor *$2 ",
		"tien  voor *$2 ",
		"vijf voor *$2 "
	},

	.greetings = {
		"Goede mor-  gen ",
		"Goede middag ",
		"Goede avond ",
		"Goede nacht "
	},
#endif
	.connection_lost = "Waar  is je tele- foon? ",

    .number_of_exceptions = 0
};
