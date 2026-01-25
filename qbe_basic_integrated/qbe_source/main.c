#include "all.h"
#include "config.h"
#include <ctype.h>
#include <getopt.h>
#include <unistd.h>
#include <libgen.h>

/* FasterBASIC frontend integration */
extern FILE* compile_basic_to_il(const char *basic_path);
extern int is_basic_file(const char *filename);

/* Get the directory where this executable is located */
static char*
get_exe_dir(void)
{
	static char buf[1024];
	ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (len == -1) {
		/* macOS doesn't have /proc/self/exe, try _NSGetExecutablePath */
		uint32_t size = sizeof(buf);
		if (_NSGetExecutablePath(buf, &size) == 0) {
			len = strlen(buf);
		} else {
			return ".";
		}
	}
	buf[len] = '\0';
	return dirname(buf);
}

/* Execute a shell command and return exit status */
static int
run_command(const char *cmd)
{
	int ret = system(cmd);
	return WIFEXITED(ret) ? WEXITSTATUS(ret) : -1;
}

Target T;

char debug['Z'+1] = {
	['P'] = 0, /* parsing */
	['M'] = 0, /* memory optimization */
	['N'] = 0, /* ssa construction */
	['C'] = 0, /* copy elimination */
	['F'] = 0, /* constant folding */
	['K'] = 0, /* if-conversion */
	['A'] = 0, /* abi lowering */
	['I'] = 0, /* instruction selection */
	['L'] = 0, /* liveness */
	['S'] = 0, /* spilling */
	['R'] = 0, /* reg. allocation */
};

extern Target T_amd64_sysv;
extern Target T_amd64_apple;
extern Target T_arm64;
extern Target T_arm64_apple;
extern Target T_rv64;

static Target *tlist[] = {
	&T_amd64_sysv,
	&T_amd64_apple,
	&T_arm64,
	&T_arm64_apple,
	&T_rv64,
	0
};
static FILE *outf;
static int dbg;

static void
data(Dat *d)
{
	if (dbg)
		return;
	emitdat(d, outf);
	if (d->type == DEnd) {
		fputs("/* end data */\n\n", outf);
		freeall();
	}
}

static void
func(Fn *fn)
{
	uint n;

	if (dbg)
		fprintf(stderr, "**** Function %s ****", fn->name);
	if (debug['P']) {
		fprintf(stderr, "\n> After parsing:\n");
		printfn(fn, stderr);
	}
	T.abi0(fn);
	fillcfg(fn);
	filluse(fn);
	promote(fn);
	filluse(fn);
	ssa(fn);
	filluse(fn);
	ssacheck(fn);
	fillalias(fn);
	loadopt(fn);
	filluse(fn);
	fillalias(fn);
	coalesce(fn);
	filluse(fn);
	filldom(fn);
	ssacheck(fn);
	gvn(fn);
	fillcfg(fn);
	simplcfg(fn);
	filluse(fn);
	filldom(fn);
	gcm(fn);
	filluse(fn);
	ssacheck(fn);
	if (T.cansel) {
		ifconvert(fn);
		fillcfg(fn);
		filluse(fn);
		filldom(fn);
		ssacheck(fn);
	}
	T.abi1(fn);
	simpl(fn);
	fillcfg(fn);
	filluse(fn);
	T.isel(fn);
	fillcfg(fn);
	filllive(fn);
	fillloop(fn);
	fillcost(fn);
	spill(fn);
	rega(fn);
	fillcfg(fn);
	simpljmp(fn);
	fillcfg(fn);
	assert(fn->rpo[0] == fn->start);
	for (n=0;; n++)
		if (n == fn->nblk-1) {
			fn->rpo[n]->link = 0;
			break;
		} else
			fn->rpo[n]->link = fn->rpo[n+1];
	if (!dbg) {
		T.emitfn(fn, outf);
		fprintf(outf, "/* end function %s */\n\n", fn->name);
	} else
		fprintf(stderr, "\n");
	freeall();
}

static void
dbgfile(char *fn)
{
	emitdbgfile(fn, outf);
}

int
main(int ac, char *av[])
{
	Target **t;
	FILE *inf, *hf;
	char *f, *sep, *output_file = NULL;
	char *runtime_path = NULL;
	char temp_asm[256] = {0};
	char cmd[2048];
	int c, compile_only = 0, is_basic = 0;

	T = Deftgt;
	outf = stdout;
	while ((c = getopt(ac, av, "hcd:o:t:")) != -1)
		switch (c) {
		case 'c':
			compile_only = 1;
			break;
		case 'd':
			for (; *optarg; optarg++)
				if (isalpha(*optarg)) {
					debug[toupper(*optarg)] = 1;
					dbg = 1;
				}
			break;
		case 'o':
			output_file = optarg;
			/* Don't open file yet - we'll decide later based on file type */
			break;
		case 't':
			if (strcmp(optarg, "?") == 0) {
				puts(T.name);
				exit(0);
			}
			for (t=tlist;; t++) {
				if (!*t) {
					fprintf(stderr, "unknown target '%s'\n", optarg);
					exit(1);
				}
				if (strcmp(optarg, (*t)->name) == 0) {
					T = **t;
					break;
				}
			}
			break;
		case 'h':
		default:
			hf = c != 'h' ? stderr : stdout;
			fprintf(hf, "%s [OPTIONS] {file.ssa, file.bas, -}\n", av[0]);
			fprintf(hf, "\t%-11s prints this help\n", "-h");
			fprintf(hf, "\t%-11s output to file\n", "-o file");
			fprintf(hf, "\t%-11s compile only (stop at assembly)\n", "-c");
			fprintf(hf, "\t%-11s generate for a target among:\n", "-t <target>");
			fprintf(hf, "\t%-11s ", "");
			for (t=tlist, sep=""; *t; t++, sep=", ") {
				fprintf(hf, "%s%s", sep, (*t)->name);
				if (*t == &Deftgt)
					fputs(" (default)", hf);
			}
			fprintf(hf, "\n");
			fprintf(hf, "\t%-11s dump debug information\n", "-d <flags>");
			exit(c != 'h');
		}

	/* Debug: show what files we're processing */
	if (optind >= ac) {
		fprintf(stderr, "error: no input file specified\n");
		exit(1);
	}

	do {
		f = av[optind];
		if (!f || strcmp(f, "-") == 0) {
			inf = stdin;
			f = "-";
		} else {
			/* Check if this is a BASIC source file */
			is_basic = is_basic_file(f);
			
			/* Set up output file based on file type and flags */
			if (is_basic && output_file && !compile_only) {
				/* BASIC file with -o but not -c: compile to executable */
				snprintf(temp_asm, sizeof(temp_asm), "/tmp/qbe_basic_%d.s", getpid());
				outf = fopen(temp_asm, "w");
				if (!outf) {
					fprintf(stderr, "cannot create temp file '%s'\n", temp_asm);
					exit(1);
				}
			} else if (output_file && strcmp(output_file, "-") != 0) {
				/* Regular output to file (assembly) */
				outf = fopen(output_file, "w");
				if (!outf) {
					fprintf(stderr, "cannot open '%s'\n", output_file);
					exit(1);
				}
			}
			/* else: outf remains stdout */
			
			if (is_basic) {
				inf = compile_basic_to_il(f);
				if (!inf) {
					fprintf(stderr, "failed to compile BASIC file '%s'\n", f);
					exit(1);
				}
			} else {
				inf = fopen(f, "r");
				if (!inf) {
					fprintf(stderr, "cannot open '%s'\n", f);
					exit(1);
				}
			}
		}
		parse(inf, f, dbgfile, data, func);
		fclose(inf);
	} while (++optind < ac);

	if (!dbg)
		T.emitfin(outf);

	/* If we created a temp assembly file, now assemble and link */
	if (temp_asm[0]) {
		fclose(outf);
		
		/* Find precompiled runtime_stubs.o - it should be in obj/ relative to executable */
		char exe_dir_buf[1024];
		ssize_t len = readlink("/proc/self/exe", exe_dir_buf, sizeof(exe_dir_buf) - 1);
		if (len == -1) {
			/* macOS - use _NSGetExecutablePath */
			extern int _NSGetExecutablePath(char *buf, uint32_t *size);
			uint32_t size = sizeof(exe_dir_buf);
			if (_NSGetExecutablePath(exe_dir_buf, &size) == 0) {
				len = strlen(exe_dir_buf);
			}
		}
		if (len > 0) {
			exe_dir_buf[len] = '\0';
			char *dir = dirname(exe_dir_buf);
			snprintf(cmd, sizeof(cmd), "%s/obj/runtime_stubs.o", dir);
			runtime_path = strdup(cmd);
		}
		
		/* If precompiled runtime not found, fall back to source */
		if (!runtime_path || access(runtime_path, R_OK) != 0) {
			runtime_path = "../fsh/runtime_stubs.c";
			if (access(runtime_path, R_OK) != 0) {
				fprintf(stderr, "Error: runtime library not found\n");
				exit(1);
			}
			/* Compile from source */
			snprintf(cmd, sizeof(cmd), "cc -O2 %s %s -o %s", temp_asm, runtime_path, output_file);
		} else {
			/* Link with precompiled runtime */
			snprintf(cmd, sizeof(cmd), "cc %s %s -o %s", temp_asm, runtime_path, output_file);
		}
		
		int ret = run_command(cmd);
		unlink(temp_asm);  /* Clean up temp file */
		
		if (ret != 0) {
			fprintf(stderr, "assembly/linking failed\n");
			exit(1);
		}
		
		if (!dbg) {
			fprintf(stderr, "Compiled %s -> %s\n", f, output_file);
		}
	}

	exit(0);
}
