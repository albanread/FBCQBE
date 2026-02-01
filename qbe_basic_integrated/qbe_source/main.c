#include "all.h"
#include "config.h"
#include <ctype.h>
#include <getopt.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <string.h>

/* FasterBASIC frontend integration */
extern FILE* compile_basic_to_il(const char *basic_path);
extern int is_basic_file(const char *filename);
extern void set_trace_cfg(int enable);

/* Global flag for MADD fusion control */
static int enable_madd_fusion = 1;  /* Enabled by default */

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
	FILE *inf;
	char *f, *sep, *output_file = NULL;
	char *runtime_dir = NULL;
	char temp_asm[256] = {0};
	char cmd[2048];
	int c, compile_only = 0, is_basic = 0, il_only = 0;
	int need_linking = 0;
	int trace_cfg = 0;
	int i, j;
	char **filtered_av;
	int filtered_ac;

	T = Deftgt;
	outf = stdout;
	
	/* Parse and filter long options before getopt */
	filtered_av = malloc(ac * sizeof(char*));
	filtered_av[0] = av[0];
	filtered_ac = 1;
	
	for (i = 1; i < ac; i++) {
		if (strcmp(av[i], "--enable-madd-fusion") == 0) {
			enable_madd_fusion = 1;
			/* Skip this argument */
		} else if (strcmp(av[i], "--disable-madd-fusion") == 0) {
			enable_madd_fusion = 0;
			/* Skip this argument */
		} else {
			/* Keep this argument */
			filtered_av[filtered_ac++] = av[i];
		}
	}
	
	/* Reset optind for getopt */
	optind = 1;
	
	while ((c = getopt(filtered_ac, filtered_av, "hicd:o:t:G")) != -1)
		switch (c) {
		case 'i':
			il_only = 1;
			break;
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
		case 'G':
			trace_cfg = 1;
			break;
		case 'h':
		default:
			fprintf(stderr, "%s [OPTIONS] {file.ssa, file.bas, -}\n", av[0]);
			fprintf(stderr, "\t%-11s prints this help\n", "-h");
			fprintf(stderr, "\t%-11s output to file\n", "-o file");
			fprintf(stderr, "\t%-11s output IL only (stop before assembly)\n", "-i");
			fprintf(stderr, "\t%-11s compile only (stop at assembly)\n", "-c");
			fprintf(stderr, "\t%-11s trace CFG and exit (BASIC files only)\n", "-G");
			fprintf(stderr, "\t%-11s enable MADD/MSUB fusion (default)\n", "--enable-madd-fusion");
			fprintf(stderr, "\t%-11s disable MADD/MSUB fusion\n", "--disable-madd-fusion");
			fprintf(stderr, "\t%-11s generate for a target among:\n", "-t <target>");
			fprintf(stderr, "\t%-11s ", "");
			for (t=tlist, sep=""; *t; t++, sep=", ") {
				fprintf(stderr, "%s%s", sep, (*t)->name);
				if (*t == &Deftgt)
					fputs(" (default)", stderr);
			}
			fprintf(stderr, "\n");
			fprintf(stderr, "\t%-11s dump debug information\n", "-d <flags>");
			exit(c != 'h');
		}

	if (optind >= filtered_ac) {
		fprintf(stderr, "error: no input file specified\n");
		free(filtered_av);
		exit(1);
	}
	
	/* Set trace-cfg flag before compilation */
	if (trace_cfg) {
		set_trace_cfg(1);
	}
	
	/* Set MADD fusion environment variable for ARM64 backend */
	if (enable_madd_fusion) {
		setenv("ENABLE_MADD_FUSION", "1", 1);
	} else {
		setenv("ENABLE_MADD_FUSION", "0", 1);
	}

	/* Process input files */
	do {
		f = filtered_av[optind];
		if (!f || strcmp(f, "-") == 0) {
			inf = stdin;
			f = "-";
			is_basic = 0;
		} else {
			is_basic = is_basic_file(f);
			
			if (is_basic) {
				inf = compile_basic_to_il(f);
				if (!inf) {
					fprintf(stderr, "failed to compile BASIC file '%s'\n", f);
					exit(1);
				}
				
				/* If trace-cfg was enabled, compilation stops after CFG dump */
				if (trace_cfg) {
					fclose(inf);
					exit(0);
				}
			} else {
				inf = fopen(f, "r");
				if (!inf) {
					fprintf(stderr, "cannot open '%s'\n", f);
					exit(1);
				}
			}
		}
		
		/* Decide output strategy:
		 * -i: Output IL only to -o or stdout
		 * -c: Output assembly to -o or stdout
		 * BASIC + -o (no flags): Create executable (temp asm, then link)
		 * Otherwise: Output assembly to stdout
		 */
		
		if (il_only) {
			/* Output IL only - just copy through */
			if (output_file && strcmp(output_file, "-") != 0) {
				outf = fopen(output_file, "w");
				if (!outf) {
					fprintf(stderr, "cannot open '%s'\n", output_file);
					exit(1);
				}
			} else {
				outf = stdout;
			}
			
			char buf[4096];
			size_t n;
			while ((n = fread(buf, 1, sizeof(buf), inf)) > 0) {
				fwrite(buf, 1, n, outf);
			}
			fclose(inf);
			
			if (outf != stdout)
				fclose(outf);
				
		} else if (is_basic && !il_only) {
			/* BASIC file: compile to assembly and link to executable */
			/* Generate a default output name if not specified */
			char default_output[256];
			if (!output_file) {
				/* Strip .bas extension and use as executable name */
				const char *base = strrchr(f, '/');
				base = base ? base + 1 : f;
				char *dot = strrchr(base, '.');
				if (dot && (strcmp(dot, ".bas") == 0 || strcmp(dot, ".BAS") == 0)) {
					snprintf(default_output, sizeof(default_output), "%.*s", (int)(dot - base), base);
				} else {
					snprintf(default_output, sizeof(default_output), "%s.out", base);
				}
				output_file = default_output;
			}
			
			snprintf(temp_asm, sizeof(temp_asm), "/tmp/qbe_basic_%d.s", getpid());
			outf = fopen(temp_asm, "w");
			if (!outf) {
				fprintf(stderr, "cannot create temp file '%s'\n", temp_asm);
				exit(1);
			}
			need_linking = !compile_only;
			
			parse(inf, f, dbgfile, data, func);
			fclose(inf);
			
			if (!dbg)
				T.emitfin(outf);
			fclose(outf);
			
		} else {
			/* Regular QBE processing - output assembly */
			if (output_file && strcmp(output_file, "-") != 0) {
				outf = fopen(output_file, "w");
				if (!outf) {
					fprintf(stderr, "cannot open '%s'\n", output_file);
					exit(1);
				}
			} else {
				outf = stdout;
			}
			
			parse(inf, f, dbgfile, data, func);
			fclose(inf);
			
			if (!dbg)
				T.emitfin(outf);
				
			if (outf != stdout)
				fclose(outf);
		}
		
	} while (++optind < ac);

	/* If we need to link (BASIC + -o without -i or -c) */
	if (need_linking && temp_asm[0]) {
		/* Find runtime library - try multiple locations */
		char runtime_path[1024];
		char *search_paths[] = {
			"runtime",                         /* Local runtime directory (preferred) */
			"qbe_basic_integrated/runtime",    /* From project root */
			"../runtime",                      /* From qbe_basic_integrated/ when in project root */
			"fsh/FasterBASICT/runtime_c",      /* Development location from project root */
			"../fsh/FasterBASICT/runtime_c",   /* Development location from qbe_basic_integrated/ */
			NULL
		};
		
		/* Find runtime directory */
		for (char **search = search_paths; *search; search++) {
			if (access(*search, R_OK) == 0) {
				runtime_dir = *search;
				break;
			}
		}
		
		if (!runtime_dir) {
			fprintf(stderr, "Error: runtime library not found\n");
			fprintf(stderr, "Searched:\n");
			for (char **search = search_paths; *search; search++) {
				fprintf(stderr, "  %s\n", *search);
			}
			unlink(temp_asm);
			exit(1);
		}
		
		/* Runtime source files */
		char *runtime_files[] = {
			"basic_runtime.c",
			"io_ops.c",
			"io_ops_format.c",
			"math_ops.c",
			"string_ops.c",
			"string_pool.c",
			"string_utf32.c",
			"conversion_ops.c",
			"array_ops.c",
			"array_descriptor_runtime.c",
			"memory_mgmt.c",
			"basic_data.c",
			NULL
		};
		
		/* Check if we have precompiled runtime objects */
		char obj_dir[1024];
		snprintf(obj_dir, sizeof(obj_dir), "%s/.obj", runtime_dir);
		
		int need_rebuild = 0;
		if (access(obj_dir, R_OK) != 0) {
			/* Create object directory if it doesn't exist */
			snprintf(cmd, sizeof(cmd), "mkdir -p %s", obj_dir);
			run_command(cmd);
			need_rebuild = 1;
		} else {
			/* Check if any source is newer than its object */
			for (char **src = runtime_files; *src; src++) {
				char src_path[1024], obj_path[1024];
				snprintf(src_path, sizeof(src_path), "%s/%s", runtime_dir, *src);
				snprintf(obj_path, sizeof(obj_path), "%s/.obj/%s.o", runtime_dir, *src);
				
				/* If object doesn't exist or source is newer, rebuild */
				struct stat src_stat, obj_stat;
				if (stat(obj_path, &obj_stat) != 0 ||
				    (stat(src_path, &src_stat) == 0 && src_stat.st_mtime > obj_stat.st_mtime)) {
					need_rebuild = 1;
					break;
				}
			}
		}
		
		/* Build runtime objects if needed */
		if (need_rebuild) {
			if (!dbg) {
				fprintf(stderr, "Building runtime library...\n");
			}
			
			for (char **src = runtime_files; *src; src++) {
				char src_path[1024], obj_path[1024];
				snprintf(src_path, sizeof(src_path), "%s/%s", runtime_dir, *src);
				snprintf(obj_path, sizeof(obj_path), "%s/.obj/%s.o", runtime_dir, *src);
				
				/* Compile source to object */
				snprintf(cmd, sizeof(cmd), "cc -O2 -c %s -o %s 2>&1 | grep -v warning || true", 
					src_path, obj_path);
				
				int ret = run_command(cmd);
				if (ret != 0) {
					fprintf(stderr, "Failed to compile %s\n", *src);
					unlink(temp_asm);
					exit(1);
				}
			}
		}
		
		/* Build link command with precompiled runtime objects */
		char obj_list[2048] = "";
		for (char **src = runtime_files; *src; src++) {
			char obj_path[256];
			snprintf(obj_path, sizeof(obj_path), "%s/.obj/%s.o ", runtime_dir, *src);
			strcat(obj_list, obj_path);
		}
		
		snprintf(cmd, sizeof(cmd), "cc -O2 %s %s -o %s", temp_asm, obj_list, output_file);
		
		int ret = run_command(cmd);
		unlink(temp_asm);
		
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