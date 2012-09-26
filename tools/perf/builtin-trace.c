#include "builtin.h"
#include "util/evlist.h"
#include "util/parse-options.h"
#include "util/thread_map.h"
#include "event-parse.h"

#include <libaudit.h>
#include <stdlib.h>

static struct syscall_fmt {
	const char *name;
	bool	   errmsg;
	bool	   timeout;
} syscall_fmts[] = {
	{ .name	    = "futex",	  .errmsg = true, },
	{ .name	    = "poll",	  .errmsg = true, .timeout = true, },
	{ .name	    = "ppoll",	  .errmsg = true, .timeout = true, },
	{ .name	    = "read",	  .errmsg = true, },
	{ .name	    = "recvfrom", .errmsg = true, },
	{ .name	    = "select",	  .errmsg = true, .timeout = true, },
};

static int syscall_fmt__cmp(const void *name, const void *fmtp)
{
	const struct syscall_fmt *fmt = fmtp;
	return strcmp(name, fmt->name);
}

static struct syscall_fmt *syscall_fmt__find(const char *name)
{
	const int nmemb = ARRAY_SIZE(syscall_fmts);
	return bsearch(name, syscall_fmts, nmemb, sizeof(struct syscall_fmt), syscall_fmt__cmp);
}

struct syscall {
	struct event_format *tp_format;
	const char	    *name;
	struct syscall_fmt  *fmt;
};

struct trace {
	int			audit_machine;
	struct {
		int		max;
		struct syscall  *table;
	} syscalls;
	struct perf_record_opts opts;
};

static int trace__read_syscall_info(struct trace *trace, int id)
{
	char tp_name[128];
	struct syscall *sc;

	if (id > trace->syscalls.max) {
		struct syscall *nsyscalls = realloc(trace->syscalls.table, (id + 1) * sizeof(*sc));

		if (nsyscalls == NULL)
			return -1;

		if (trace->syscalls.max != -1) {
			memset(nsyscalls + trace->syscalls.max + 1, 0,
			       (id - trace->syscalls.max) * sizeof(*sc));
		} else {
			memset(nsyscalls, 0, (id + 1) * sizeof(*sc));
		}

		trace->syscalls.table = nsyscalls;
		trace->syscalls.max   = id;
	}

	sc = trace->syscalls.table + id;
	sc->name = audit_syscall_to_name(id, trace->audit_machine);
	if (sc->name == NULL)
		return -1;

	snprintf(tp_name, sizeof(tp_name), "sys_enter_%s", sc->name);

	sc->tp_format = event_format__new("syscalls", tp_name);
	sc->fmt = syscall_fmt__find(sc->name);

	return sc->tp_format != NULL ? 0 : -1;
}

static size_t syscall__fprintf_args(struct syscall *sc, unsigned long *args, FILE *fp)
{
	int i = 0;
	size_t printed = 0;

	if (sc->tp_format != NULL) {
		struct format_field *field;

		for (field = sc->tp_format->format.fields->next; field; field = field->next) {
			printed += fprintf(fp, "%s%s: %ld", printed ? ", " : "",
					   field->name, args[i++]);
		}
	} else {
		while (i < 6) {
			printed += fprintf(fp, "%sarg%d: %ld", printed ? ", " : "", i, args[i]);
			++i;
		}
	}

	return printed;
}

static int trace__run(struct trace *trace)
{
	struct perf_evlist *evlist = perf_evlist__new(NULL, NULL);
	struct perf_evsel *evsel, *evsel_enter, *evsel_exit;
	int err = -1, i, nr_events = 0, before;

	if (evlist == NULL) {
		printf("Not enough memory to run!\n");
		goto out;
	}

	evsel_enter = perf_evsel__newtp("raw_syscalls", "sys_enter", 0);
	if (evsel_enter == NULL) {
		printf("Couldn't read the raw_syscalls:sys_enter tracepoint information!\n");
		goto out_delete_evlist;
	}

	perf_evlist__add(evlist, evsel_enter);

	evsel_exit = perf_evsel__newtp("raw_syscalls", "sys_exit", 1);
	if (evsel_exit == NULL) {
		printf("Couldn't read the raw_syscalls:sys_exit tracepoint information!\n");
		goto out_delete_evlist;
	}

	perf_evlist__add(evlist, evsel_exit);

	err = perf_evlist__create_maps(evlist, &trace->opts.target);
	if (err < 0) {
		printf("Problems parsing the target to trace, check your options!\n");
		goto out_delete_evlist;
	}

	perf_evlist__config_attrs(evlist, &trace->opts);

	err = perf_evlist__open(evlist);
	if (err < 0) {
		printf("Couldn't create the events: %s\n", strerror(errno));
		goto out_delete_evlist;
	}

	err = perf_evlist__mmap(evlist, UINT_MAX, false);
	if (err < 0) {
		printf("Couldn't mmap the events: %s\n", strerror(errno));
		goto out_delete_evlist;
	}

	perf_evlist__enable(evlist);
again:
	before = nr_events;

	for (i = 0; i < evlist->nr_mmaps; i++) {
		union perf_event *event;

		while ((event = perf_evlist__mmap_read(evlist, i)) != NULL) {
			const u32 type = event->header.type;
			struct syscall *sc;
			struct perf_sample sample;
			int id;

			++nr_events;

			switch (type) {
			case PERF_RECORD_SAMPLE:
				break;
			case PERF_RECORD_LOST:
				printf("LOST %" PRIu64 " events!\n", event->lost.lost);
				continue;
			default:
				printf("Unexpected %s event, skipping...\n",
					perf_event__name(type));
				continue;
			}

			err = perf_evlist__parse_sample(evlist, event, &sample);
			if (err) {
				printf("Can't parse sample, err = %d, skipping...\n", err);
				continue;
			}

			evsel = perf_evlist__id2evsel(evlist, sample.id);
			if (evsel == NULL) {
				printf("Unknown tp ID %" PRIu64 ", skipping...\n", sample.id);
				continue;
			}

			id = perf_evsel__intval(evsel, &sample, "id");
			if (id < 0) {
				printf("Invalid syscall %d id, skipping...\n", id);
				continue;
			}

			if ((id > trace->syscalls.max || trace->syscalls.table[id].name == NULL) &&
			    trace__read_syscall_info(trace, id))
				continue;

			if ((id > trace->syscalls.max || trace->syscalls.table[id].name == NULL))
				continue;

			sc = &trace->syscalls.table[id];

			if (evlist->threads->map[0] == -1 || evlist->threads->nr > 1)
				printf("%d ", sample.tid);

			if (evsel == evsel_enter) {
				void *args = perf_evsel__rawptr(evsel, &sample, "args");

				printf("%s(", sc->name);
				syscall__fprintf_args(sc, args, stdout);
			} else if (evsel == evsel_exit) {
				int ret = perf_evsel__intval(evsel, &sample, "ret");

				if (ret < 0 && sc->fmt && sc->fmt->errmsg) {
					char bf[256];
					const char *emsg = strerror_r(-ret, bf, sizeof(bf)),
						   *e = audit_errno_to_name(-ret);

					printf(") = -1 %s %s", e, emsg);
				} else if (ret == 0 && sc->fmt && sc->fmt->timeout)
					printf(") = 0 Timeout");
				else
					printf(") = %d", ret);

				putchar('\n');
			}
		}
	}

	if (nr_events == before)
		poll(evlist->pollfd, evlist->nr_fds, -1);

	goto again;

out_delete_evlist:
	perf_evlist__delete(evlist);
out:
	return err;
}

int cmd_trace(int argc, const char **argv, const char *prefix __maybe_unused)
{
	const char * const trace_usage[] = {
		"perf trace [<options>]",
		NULL
	};
	struct trace trace = {
		.audit_machine = audit_detect_machine(),
		.syscalls = {
			. max = -1,
		},
		.opts = {
			.target = {
				.uid	   = UINT_MAX,
				.uses_mmap = true,
			},
			.user_freq     = UINT_MAX,
			.user_interval = ULLONG_MAX,
			.no_delay      = true,
			.mmap_pages    = 1024,
		},
	};
	const struct option trace_options[] = {
	OPT_STRING('p', "pid", &trace.opts.target.pid, "pid",
		    "trace events on existing process id"),
	OPT_STRING(0, "tid", &trace.opts.target.tid, "tid",
		    "trace events on existing thread id"),
	OPT_BOOLEAN(0, "all-cpus", &trace.opts.target.system_wide,
		    "system-wide collection from all CPUs"),
	OPT_STRING(0, "cpu", &trace.opts.target.cpu_list, "cpu",
		    "list of cpus to monitor"),
	OPT_BOOLEAN(0, "no-inherit", &trace.opts.no_inherit,
		    "child tasks do not inherit counters"),
	OPT_UINTEGER(0, "mmap-pages", &trace.opts.mmap_pages,
		     "number of mmap data pages"),
	OPT_STRING(0, "uid", &trace.opts.target.uid_str, "user",
		   "user to profile"),
	OPT_END()
	};
	int err;

	argc = parse_options(argc, argv, trace_options, trace_usage, 0);
	if (argc)
		usage_with_options(trace_usage, trace_options);

	err = perf_target__parse_uid(&trace.opts.target);
	if (err) {
		char bf[BUFSIZ];
		perf_target__strerror(&trace.opts.target, err, bf, sizeof(bf));
		printf("%s", bf);
		return err;
	}

	return trace__run(&trace);
}
