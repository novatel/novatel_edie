/* Build options collection tool for pclp_config
   Copyright Gimpel Software LLC 2017-2019. All rights reserved.
   Confidential and proprietary. No part of this file may be redistributed
   without express written permission of Gimpel Software LLC.
   This file is provided by Gimpel Software LLC (https://www.gimpel.com) for
   use exclusively with PC-lint Plus. Redistribution to and use by licensed
   users is permitted. Any such redistribution must preserve this notice and,
   if the redistributed file has been modified, provide notice that the file
   has been modified from the original.
*/

/* Imposter Program

The imposter program is used to extract build commands from a build system for
further processing by pclp_config, the PC-lint Plus configuration tool.
To obtain the information it needs, the build system is configured to call the
imposter program instead of the actual compiler.  The imposter program logs
the arguments of each invocation and then optionally invokes the compiler (this
may or may not be necessary depending on the build system and project being
configured).

In order to be a drop-in replacement for the compiler, communication with the
imposter program is performed via environment variables instead of command-line
options.  The supported environment variables are:

  IMPOSTER_EXIT_SUCCESS
  The value with which the imposter should exit when not invoking a compiler
  when no error occurs.  This should be the same value your compiler exits
  with when there is no compilation error or the value your build system
  expects for a successful compilation.  If no value is supplied, 0 is used.

  IMPOSTER_EXIT_FAILURE
  The exit code used when an error is encountered before invoking the compiler.
  This value is used if the log file cannot be written to, if we run out of
  memory, or if the compiler fails to invoke.  If the compiler is successfully
  invoked, the imposter returns the value that the compiler returned.  If no
  value is supplied, the default of 1 is used.

  IMPOSTER_LOG
  The name of the log file to write invocations to.  If an absolute path is not
  provided, the path is relative the directory in which the imposter is invoked.
  If no value is provided, output is written to stdout.

  IMPOSTER_COMPILER
  The full path of the compiler to invoke after logging invocation information.
  If no value is provided, no compiler is invoked (unless IMPOSTER_COMPILER_ARG1
  is set as described below) and the imposter exits with IMPOSTER_EXIT_SUCCESS
  on success.

  IMPOSTER_COMPILER_ARG1
  If this variable is set to any value (including an empty value) and
  IMPOSTER_COMPILER is NOT set, the compiler is expected to be provided as the
  first argument to the imposter program and will be executed with the remaining
  argument list.  If this variable is set and there is no first argument, the
  imposter will exit with the failure exit code.  This functionality is useful
  when the imposter needs to stand in for multiple compilers during the build
  process, such as both a C and C++ compiler.

  IMPOSTER_NO_RSP_EXPAND
  If an argument is received that starts with '@', this is treated as a response
  file and the argument is replaced with the parsed arguments contained within
  the response file (the name left after removing the '@').  If the file cannot
  be opened, or IMPOSTER_NO_RSP_EXPAND is set, the entire argument is logged as
  is.  Note that in all cases, if the compiler is invoked, it receives the
  unexpanded argument, the expansion is limited to the logging process.

  IMPOSTER_PRE_2008_PARAMS
  When parsing response files, the parameters it contains are processed using
  the Windows command line parameter parsing rules.  There is a subtle and
  undocumented difference between the way handling of consecutive quotes was
  handled prior to 2008 and after 2008.  By default, the post-2008 rules are
  employed.  If this variable is set, the pre-2008 rules are instead employed.

  IMPOSTER_INCLUDE_VARS
  A semi-colon separated list of environment variable to extract header include
  information from.  If this variable is not set, the default list of:
    INCLUDE;CPATH;C_INCLUDE_PATH;CPLUS_INCLUDE_PATH
  is used.  Each environment variable processed is expected to contain a list
  of paths which are emitted as -I options in the log file.  The delimiter used
  and how to change it are described below in 'IMPOSTER_INCLUDE_DELIM'.
  Environment variables are processed in the order provided.  Environment
  variables included in this list that are not set are ignored.
  Setting IMPOSTER_INCLUDE_VARS to an empty value will disable this processing.

  IMPOSTER_INCLUDE_DELIM
  The character(s) recognized as delimiters when parsing include directories
  specified from environment variables (such as those specified by IMPOSTER_-
  INCLUDE_VARS). By default, this is ';' on Windows and ':' on other platforms.
  Setting this variable will override the default.  Each character appearing
  in the value of this variable will be considered to be a delimiter.  For
  example, setting this variable to '|!:' will cause any of these characters
  to separate directories appearing in the include variables.

  IMPOSTER_AUTO_RSP_FILE
  If this variable is set, the value is interpreted to be a response file that
  should be processed and logged to the beginning of the invocation.

  IMPOSTER_RSP_INTRO_ARG
  If this variable is set and is encountered while processing the arguments
  of the compiler invocation, the next argument is assumed to be the name of
  a response file whose contents are parsed and logged instead of the option
  itself.  If there is no next argument or the next argument is not the name
  of a file that can be opened, the option and following argument are logged
  as usual.

  IMPOSTER_MODULES_IN_WORKING_DIR
  If this variable is set, the working directory will be preprended to each
  argument which appears to be the relative path of a module. Apparent
  options, absolute paths, or arguments lacking one of the extensions defined
  in 'IMPOSTER_MODULE_EXTENSIONS' will be ignored.

  IMPOSTER_MODULE_EXTENSIONS
  If this variable is set, its value replaces the default module extension
  list: ".c;.C;.cpp;.CPP;.cc;.CC". The value is a case-sensitive list of
  extensions (including the dot) delimited by semicolons.

  IMPOSTER_PATH_ARGUMENT_RELATIVE_TO_WORKING_DIR_OPTION_INTRODUCERS
  If this variable is set, any argument that initially matches one of the
  provided semicolon-delimited strings will have the working directory
  inserted after the option introducer if the remainder of the option does
  not appear to be an absolute path.
*/

/*lint -e820*/

#if defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS STDC
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#include <direct.h>
#endif

#define MAX_PATH_SIZE 4096
#define MAX_ENV_VAR_NAME 256

void *xmalloc(size_t size) {
    void *p = malloc(size);
    if (!p) {
        fputs("Ran out of memory", stderr);
        abort();
    }
    return p;
}

void *increase_buffer(void *buffer, size_t *buffer_size) {
    /* Increase the size of the provided buffer, returning the new buffer.
       The size of the new buffer is stored in current_size.  If the buffer
       cannot be resized, terminate. */
    if (*buffer_size < 1024) { *buffer_size = 1024; }
    if (((size_t)-1) / 2 < *buffer_size) {
        fputs("Ran out of memory", stderr);
        abort();
    }
    *buffer_size *= 2;
    buffer = realloc(buffer, *buffer_size);
    if (!buffer) {
        fputs("Ran out of memory", stderr);
        abort();
    }
    return buffer;
}

int run_compiler(char *argv[]);

/* The exit code that indicates success */
static int success_exit_code = 0;

/* The exit code that indicates failure */
static int failure_exit_code = 1;

/* The name of the file to log invocations to */
static const char *log_filename;

/* The full path of the compiler to invoke, if any, and not provided as
   the first argument to the imposter. */
char *compiler;

/* Whether the compiler is expected to appear in the first argument */
int compiler_arg1 = 0;

static int modules_in_working_dir = 0;
static char* rel_opt_intros = "";
static char working_dir_buffer[MAX_PATH_SIZE];
static const char* module_extensions = ".c;.C;.cpp;.CPP;.cc;.CC";

static int expand_response_files = 1;

static int pre_2008_params = 0;

/* The name of the auto response file to process */
static const char * auto_rsp_file;

/* The list of INCLUDE-like environment variables */
static const char * include_var_list = "INCLUDE;CPATH;C_INCLUDE_PATH;CPLUS_INCLUDE_PATH";

/* The compiler option that indicates the next argument is a response file */
static const char * rsp_intro_arg;

#if defined(__unix__) || defined(__APPLE__)
static const char * include_delims = ":";
#elif defined(_WIN32)
static const char * include_delims = ";";
#endif

void init(void) {
    const char *success_retval_str = getenv("IMPOSTER_EXIT_SUCCESS");
    const char *failure_retval_str = getenv("IMPOSTER_EXIT_FAILURE");
    const char *compiler_arg1_str  = getenv("IMPOSTER_COMPILER_ARG1");
    const char *no_rsp_expand_str  = getenv("IMPOSTER_NO_RSP_EXPAND");
    const char *pre_2008_params_str= getenv("IMPOSTER_PRE_2008_PARAMS");
    const char *include_vars_str   = getenv("IMPOSTER_INCLUDE_VARS");
    const char *include_delim_str  = getenv("IMPOSTER_INCLUDE_DELIM");
    const char *rsp_intro_arg_str  = getenv("IMPOSTER_RSP_INTRO_ARG");
    const char *modules_in_working_dir_str = getenv("IMPOSTER_MODULES_IN_WORKING_DIR");
    const char* module_extensions_str = getenv("IMPOSTER_MODULE_EXTENSIONS");
    log_filename = getenv("IMPOSTER_LOG");
    compiler = getenv("IMPOSTER_COMPILER");
    auto_rsp_file = getenv("IMPOSTER_AUTO_RSP_FILE");
    rel_opt_intros = getenv("IMPOSTER_PATH_ARGUMENT_RELATIVE_TO_WORKING_DIR_OPTION_INTRODUCERS");

    if (success_retval_str) { success_exit_code = atoi(success_retval_str); }
    if (failure_retval_str) { failure_exit_code = atoi(failure_retval_str); }
    if (compiler_arg1_str) { compiler_arg1 = 1; }
    if (no_rsp_expand_str) { expand_response_files = 0; }
    if (pre_2008_params_str) { pre_2008_params = 1; }
    if (include_vars_str) { include_var_list = include_vars_str; }
    if (include_delim_str) { include_delims = include_delim_str; }
    if (rsp_intro_arg_str) { rsp_intro_arg = rsp_intro_arg_str; }
    if (module_extensions_str) { module_extensions = module_extensions_str; }

    if (modules_in_working_dir_str) {
        modules_in_working_dir = 1;
    }

    if (modules_in_working_dir || rel_opt_intros) {
#if defined(__unix__) || defined(__APPLE__)
        if (!getcwd(working_dir_buffer, sizeof(working_dir_buffer))) {
            fputs("failed to obtain working directory", stderr);
            abort();
        }
#elif defined(_WIN32)
        if (!_getcwd(working_dir_buffer, sizeof(working_dir_buffer))) {
            fputs("failed to obtain working directory", stderr);
            abort();
        }
#endif
    }
}

void log_args_yaml(FILE *fd, char * const argv[]);
const char * parse_cmndline_arg(const char *cmndline, char **buffer, int pre_2008);

void log_args(int argc, char * const argv[]) {
    FILE *fd;
    if (argc > compiler_arg1 + 1) {
        /* If there are any arguments, record them */
        fd = log_filename ? fopen(log_filename, "a") : stdout;
        if (!fd) {
            fprintf(stderr, "Failed to open '%s' for writing: %s\n", log_filename, strerror(errno));
            exit(failure_exit_code);
        }
        log_args_yaml(fd, &argv[compiler_arg1+1]);
        if (log_filename) { fclose(fd); }
    }
}

enum stream_type {
    NEW,
    UTF8,
    UTF16LE,
    UTF16BE
};

int is_empty_rsp_line(const char *line) {
    while (*line == ' ' || *line == '\t' || *line == '\r' || *line == '\n') { ++line; }
    return (*line == '\0' || *line == '#');
}

int get_next_utf16_pair(FILE *fp, enum stream_type st) {
    int upper, lower;
    int result;
    if (st == UTF16LE) {
        lower = fgetc(fp);
        upper = fgetc(fp);
    }
    else {
        upper = fgetc(fp);
        lower = fgetc(fp);
    }
    if (upper == EOF || lower == EOF) { return EOF; }
    result = ((unsigned char) upper << 8) + (unsigned char) lower;
    return result;
}

int get_next_utf16_codepoint(FILE *fp, enum stream_type st) {
    int upper_pair, lower_pair;
    upper_pair = get_next_utf16_pair(fp, st);
    if (upper_pair >= 0xD800 && upper_pair <= 0xDBFF) {
        lower_pair = get_next_utf16_pair(fp, st);
        if (lower_pair == EOF) { return EOF; }
        return ((unsigned short)(upper_pair & 0x3FF) << 10) + ((unsigned short)(lower_pair & 0x03FF)) + 0x10000;
    }
    return upper_pair;
}

char *get_next_utf8_line(FILE *fp, enum stream_type *st) {
    /* Get the next line in UTF-8 encoding, assumes fp was opened in binary mode. */
    size_t buffer_size = 0;
    size_t buffer_pos = 0;
    char *buffer = increase_buffer(NULL, &buffer_size);
    if (*st == NEW) {
        /* Need to determine encoding, look for BOM. */
        int c1 = fgetc(fp);
        if (c1 == EOF) { free(buffer); return 0; }
        if (c1 == 0xFF) { (void)fgetc(fp); *st = UTF16LE; }
        else if (c1 == 0xFE) { (void)fgetc(fp); *st = UTF16BE; }
        else if ( c1 == 0xEF) { (void)fgetc(fp); (void)fgetc(fp); *st = UTF8; }
        else { (void)ungetc(c1, fp); *st = UTF8; }
    }

    if (*st == UTF8) {
        int c;
        while ((c = fgetc(fp)) != EOF) {
            if (buffer_pos > buffer_size - 4)
                buffer = increase_buffer(buffer, &buffer_size);
            buffer[buffer_pos++] = c;
            if (c == '\r' || c == '\n') {
                buffer[buffer_pos] = '\0';
                return buffer;
            }
        }
        if (buffer_pos == 0) { free(buffer); return 0; }
        if (buffer_pos > buffer_size - 2)
            buffer = increase_buffer(buffer, &buffer_size);
        buffer[buffer_pos] = '\0';
        return buffer;
    }

    if (*st == UTF16LE || *st == UTF16BE) {
        for (;;) {
            int codepoint = get_next_utf16_codepoint(fp, *st);
            if (buffer_pos > buffer_size - 6)
                buffer = increase_buffer(buffer, &buffer_size);
            if (codepoint == EOF) {
                if (buffer_pos == 0) { free(buffer); return 0; }
                buffer[buffer_pos] = '\0';
                return buffer;
            }
            if (codepoint < 0x80) {
                buffer[buffer_pos++] = codepoint & 0x7F;
                if ((char) (codepoint & 0x7F) == '\r' || (char) (codepoint & 0x7F) == '\n') {
                    buffer[buffer_pos] = '\0';
                    return buffer;
                }
            }
            else if (codepoint < 0x0800) {
                buffer[buffer_pos++] = ((codepoint >> 6) & 0x1F) | 0xC0;
                buffer[buffer_pos++] = (codepoint & 0x3F) | 0x80;
            }
            else if (codepoint < 0x010000) {
                buffer[buffer_pos++] = ((codepoint >> 12) & 0x0F) | 0xE0;
                buffer[buffer_pos++] = ((codepoint >> 6) & 0x3F) | 0x80;
                buffer[buffer_pos++] = (codepoint & 0x3F) | 0x80;
            }
            else {
                buffer[buffer_pos++] = ((codepoint >> 18) & 0x07) | 0xF0;
                buffer[buffer_pos++] = ((codepoint >> 12) & 0x3F) | 0x80;
                buffer[buffer_pos++] = ((codepoint >> 6) & 0x3F) | 0x80;
                buffer[buffer_pos++] = (codepoint & 0x3F) | 0x80;
            }
        }
    }

    free(buffer);
    return 0;
}

int has_module_extension(const char* arg) {
    const char* ext = module_extensions;
    while (ext && *ext) {
        size_t arg_len = strlen(arg);
        size_t ext_len;
        const char* ext_end = ext;
        const char* next_ext = 0;
        while (*ext_end && *ext_end != ';') { ++ext_end; }
        if (*ext_end) {
            next_ext = ext_end + 1;
        }
        ext_len = ext_end - ext;
        if (!ext_len || arg_len <= ext_len) {
            ext = next_ext;
            continue;
        }
        if (!memcmp(arg + arg_len - ext_len, ext, ext_len)) {
            return 1;
        }
        ext = next_ext;
    }
    return 0;
}

void write_yaml_escaped(FILE* fd, const char* arg, size_t max_length) {
    char c;
    while (max_length-- != 0 && (c = *(arg++))) {
        if (c == '\'') { fputc('\'', fd); }
        fputc(c, fd);
    }
}

void log_yaml_arg(FILE *fd, const char *arg) {
    static int last_arg_was_exact_rel_opt_intro = 0;
    int added_working_directory = 0;
    fputc('\'', fd);
    if (modules_in_working_dir || last_arg_was_exact_rel_opt_intro) {
        if (*arg && *arg != '-' && *arg != '/' && *arg != '\\' && !(isalpha(*arg) && *(arg + 1) == ':') && (has_module_extension(arg) || last_arg_was_exact_rel_opt_intro)) {
            write_yaml_escaped(fd, working_dir_buffer, -1);
            fputc('/', fd);
            added_working_directory = 1;
        }
    }
    last_arg_was_exact_rel_opt_intro = 0;
    if (rel_opt_intros && !added_working_directory) {
        char* opt_intro = rel_opt_intros;
        const char* argz = arg;
        while (*opt_intro && !added_working_directory) {
            while (*opt_intro && *argz && *opt_intro == *argz) {
                ++opt_intro;
                ++argz;
            }
            if (!*opt_intro || *opt_intro == ';') {
                if (!*argz) {
                    last_arg_was_exact_rel_opt_intro = 1;
                    break;
                }
                if (*argz == '/' || *argz == '\\' || (isalpha(*argz) && *(argz + 1) == ':')) {
                    break;
                }
                write_yaml_escaped(fd, arg, argz - arg);
                write_yaml_escaped(fd, working_dir_buffer, -1);
                fputc('/', fd);
                arg = argz;
                added_working_directory = 1;
                last_arg_was_exact_rel_opt_intro = 0;
                break;
            } else {
                argz = arg;
                while (*opt_intro && *opt_intro++ != ';') { }
            }
        }
    }
    write_yaml_escaped(fd, arg, -1);
    fputc('\'', fd);
}

size_t get_next_delimited_string(const char *src, const char *delims, char *dst, size_t dst_size, size_t *size_needed) {
    /* Store the next region delimited by a character in delims from src into
       dst if the substring is smaller than dst_size.  The size needed to store
       the substring is stored in size_needed if non-null.  Returns the number
       of characters consumed from the source string. */
    size_t substr_len;
    size_t bytes_needed;
    size_t chars_processed;
    if (*src == '\0') { return 0; }

    substr_len = strcspn(src, delims);
    bytes_needed = substr_len + 1;
    chars_processed = substr_len + (src[substr_len] == '\0' ? 0 : 1);

    if (size_needed) { *size_needed = bytes_needed; }
    if (substr_len && src[substr_len] != '\0' && src[substr_len + 1] == '\0') {
        /* If there is a trailing delimiter at the end of the string, make sure
           this gets processed separately. */
        chars_processed--;
    }
    if (bytes_needed > dst_size) { return chars_processed; }

    memcpy(dst, src, substr_len);
    dst[substr_len] = '\0';
    return chars_processed;
}

void log_args_yaml(FILE *fd, char * const argv[]) {
    int argno = 0;
    const char *p;
    FILE *rsp;
    char env_var_buf[MAX_ENV_VAR_NAME];
    const char *vars_ptr = include_var_list;
    size_t bytes_processed;
    size_t string_size;

    fputc('[', fd);

    /* Process INCLUDE environment variables */
    while ((bytes_processed = get_next_delimited_string(vars_ptr, ";", env_var_buf, MAX_ENV_VAR_NAME, &string_size))) {
        char include_buffer[MAX_PATH_SIZE];
        const char *include_ptr;
        vars_ptr += bytes_processed;
        if (string_size > MAX_ENV_VAR_NAME) { continue; }

        include_ptr = getenv(env_var_buf);
        if (!include_ptr) { continue; }

        while ((bytes_processed = get_next_delimited_string(include_ptr, include_delims, include_buffer, MAX_PATH_SIZE, &string_size))) {
            include_ptr += bytes_processed;
            if (string_size > MAX_PATH_SIZE) { continue; }
            if (argno++) { fputc(',', fd); }
            log_yaml_arg(fd, "-I");
            if (argno++) { fputc(',', fd); }
            log_yaml_arg(fd, string_size <= 1 ? "." : include_buffer);
            if (!bytes_processed) { break; }
        }
    }

    /* Process auto response file */
    if (auto_rsp_file && (rsp = fopen(auto_rsp_file, "r"))) {
        enum stream_type st = NEW;
        char *line_buffer;
        char *arg_buffer;
        while ((line_buffer = get_next_utf8_line(rsp, &st))) {
            const char *line_ptr = line_buffer;
            if (is_empty_rsp_line(line_buffer)) { continue; }
            while ((line_ptr = parse_cmndline_arg(line_ptr, &arg_buffer, pre_2008_params))) {
                if (argno++) { fputc(',', fd); }
                log_yaml_arg(fd, arg_buffer);
                free(arg_buffer);
            }
            free(line_buffer);
        }
        fclose(rsp);
    }

    /* Process arguments */
    while((p = *argv)) {
        if ((*p == '@' && expand_response_files && (rsp = fopen(&p[1], "r"))) ||
            (rsp_intro_arg && strcmp(rsp_intro_arg, p) == 0 && argv[1] && (rsp = fopen(argv[1], "r")))) {
            /* This argument is a response file, process its contents */
            enum stream_type st = NEW;
            char *line_buffer;
            char *arg_buffer;
            while ((line_buffer = get_next_utf8_line(rsp, &st))) {
                const char *p2 = line_buffer;
                if (is_empty_rsp_line(line_buffer)) { continue; }
                while ((p2 = parse_cmndline_arg(p2, &arg_buffer, pre_2008_params))) {
                    if (argno++) { fputc(',', fd); }
                    log_yaml_arg(fd, arg_buffer);
                    free(arg_buffer);
                }
                free(line_buffer);
            }
            fclose(rsp);
            if (*p != '@') { ++argv; /* Skip response file argument for rsp_intro_arg */ }
        }
        else {
            if (argno++) { fputc(',', fd); }
            log_yaml_arg(fd, p);
        }
        ++argv;
    }
    fputc(']', fd);
    fputc('\n', fd);
}


const char * parse_cmndline_arg(const char *cmndline, char **arg_buffer, int pre_2008) {
    /* Parse the next Windows commandline parameter from cmndline and store it in buffer.
       cmndline is expected to be UTF8 encoded at this point.
       The rules involved here are complicated, for a detailed discussion see:
       http://daviddeley.com/autohotkey/parameters/parameters.htm */
    int quoted = 0;
    char c;
    int i;
    int num_backslashes;
    size_t buffer_idx = 0;
    size_t buffer_size = 0;
    char *buffer;

    while (*cmndline == ' ' || *cmndline == '\t' || *cmndline == '\r' || *cmndline == '\n') { ++cmndline; }
    if (!*cmndline) { return 0; }
    buffer = increase_buffer(NULL, &buffer_size);

    while ((c = *cmndline)) {
        num_backslashes = 0;
        while (c == '\\') {
            num_backslashes++;
            ++cmndline;
            c = *cmndline;
        }
        if (buffer_idx > buffer_size - 4)
            buffer = increase_buffer(buffer, &buffer_size);
        switch (c) {
            case '"':
                for (i = 0; i < num_backslashes / 2; ++i) { buffer[buffer_idx++] = '\\'; }
                if (num_backslashes % 2) {
                    buffer[buffer_idx++] = '"';
                }
                else {
                    if (quoted && cmndline[1] == '"') {
                        buffer[buffer_idx++] = '"';
                        ++cmndline;
                        if (pre_2008) { quoted = 0; }
                    }
                    else {
                        quoted = !quoted;
                    }
                }
                break;
            case '\0': case ' ': case '\t': case '\r': case '\n':
                for (i = 0; i < num_backslashes; ++i) { buffer[buffer_idx++] = '\\'; }
                if (c && quoted) { buffer[buffer_idx++] = c; }
                else { buffer[buffer_idx++] = '\0'; *arg_buffer = buffer; return cmndline; }
                break;
            default:
                for (i = 0; i < num_backslashes; ++i) { buffer[buffer_idx++] = '\\'; }
                buffer[buffer_idx++] = c;
                break;
        }
        ++cmndline;
    }
    buffer[buffer_idx] = '\0';
    *arg_buffer = buffer;
    return cmndline;
}

int main(int argc, char *argv[]) {
    init();
    log_args(argc, argv);
    
    /* If the IMPOSTER_COMPILER environment variable is set, call the actual compiler */
    if (!compiler && !compiler_arg1) { return success_exit_code; }
    return run_compiler(argv);
}

#if defined(__unix__) || defined(__APPLE__)
int run_compiler(char *argv[]) {
    if (compiler) {
        argv[0] = compiler;
        (void) execv(compiler, argv);
    }
    else {
        (void) execvp(argv[1], &argv[1]);
    }
    /* If we got this far, exec failed, probably because the wrong compiler command
       was provided.  Return a failure code. */
    fprintf(stderr, "Failed to launch compiler '%s': %s\n", compiler ? compiler : argv[1], strerror(errno));
    return failure_exit_code;
}
#elif defined(_WIN32)
char * quote_argument(const char *arg) {
    char *quoted = xmalloc((strlen(arg) + 1) * 3);
    char *p = quoted;
    assert(quoted && "failed to allocate memory for argument");
    *(p++) = '"';
    while (*arg) {
        int num_backslashes = 0;
        while (*arg == '\\') {
            ++num_backslashes;
            ++arg;
        }
        if (*arg == '"') { num_backslashes = num_backslashes * 2 + 1; }
        else if (!*arg) { num_backslashes *= 2; }
        memset(p, '\\', num_backslashes);
        p += num_backslashes;
        if (*arg) { *(p++) = *(arg++); }
    }
    *(p++) = '"';
    *p = '\0';
    return quoted;
}

size_t buffer_size_for_argv(char *argv[]) {
    size_t len = 0;
    while (*argv) {
        len += (strlen(*argv) + 1) * 3 + 2;
        ++argv;
    }
    return len;
}

char * argv_to_cmndline(char *argv[]) {
    int argno = 0;
    char *cmndline = xmalloc(buffer_size_for_argv(argv));
    assert(cmndline && "failed to allocate memory for command line");
    *cmndline = '\0';
    while (*argv) {
        char *quoted_arg = quote_argument(*argv);
        if (argno++ != 0) { strcat(cmndline, " "); }
        strcat(cmndline, quoted_arg);
        free(quoted_arg);
        ++argv;
    }
    return cmndline;
}

int run_compiler(char *argv[]) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    DWORD exit_code;
    char * cmndline;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (compiler) {
        argv[0] = compiler;
        cmndline = argv_to_cmndline(&argv[0]);
    }
    else {
        cmndline = argv_to_cmndline(&argv[1]);
    }
    if (!CreateProcessA(NULL,   /* No module name (use command line) */
                       cmndline,/* Command line */
                       NULL,    /* Process handle not inheritable */
                       NULL,    /* Thread handle not inheritable */
                       FALSE,   /* Set handle inheritance to FALSE */
                       0,       /* No creation flags */
                       NULL,    /* Use parent's environment block */
                       NULL,    /* Use parent's starting directory */
                       &si,     /* Pointer to STARTUPINFO structure */
                       &pi)     /* Pointer to PROCESS_INFORMATION structure */
        ) {
        LPVOID lpMsgBuf;
        DWORD dw = GetLastError();
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&lpMsgBuf,
            0, NULL);
        fprintf(stderr, "Failed to launch compiler '%s': %s\n", compiler ? compiler : argv[1], (LPCSTR)lpMsgBuf);
        exit(failure_exit_code);
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    if (!GetExitCodeProcess(pi.hProcess, &exit_code)) { exit(failure_exit_code); }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    exit(exit_code);
}
#endif
