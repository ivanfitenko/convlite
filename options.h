/* this is a tiny bycicle to replace GNU GETOPT */

enum options {
	CNVL_OPT_NOOPTS,
	CNVL_OPT_RESIZE,
	CNVL_OPT_BROKEN,
	CNVL_OPT_AUTOORIENT,
	CNVL_OPT_IGNORE_TWO,
	CNVL_OPT_VERSION,
	CNVL_OPT_IGNORE
};

typedef struct args_used {
	char *argnam;
	enum options opt_num;
} args_used;

args_used option[] = {
// having trailing and slashing zero is ambigious, choose one
{"-noopts", CNVL_OPT_IGNORE},
{"-resize", CNVL_OPT_RESIZE},
{"-thumbnail", CNVL_OPT_RESIZE},
{"-sample", CNVL_OPT_RESIZE},
{"registry:", CNVL_OPT_IGNORE}, // to be replaced with -define and ignore_two
{"-gravity", CNVL_OPT_IGNORE_TWO},
{"-extent", CNVL_OPT_IGNORE_TWO},
{"-broken", CNVL_OPT_BROKEN},
{"-auto-orient", CNVL_OPT_AUTOORIENT},
{"-version", CNVL_OPT_VERSION},
{"unused", CNVL_OPT_NOOPTS}
};
