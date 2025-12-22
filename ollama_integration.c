#include "ollama_integration.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <math.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include <ctype.h>

// Constants
#define RIPPLE_RL_BUFSIZE 1024
#define RIPPLE_TOK_BUFSIZE 64
#define RIPPLE_TOK_DELIM " \t\r\n\a"
#define RIPPLE_VERSION "1.0.0"
#define OLLAMA_API_URL "http://localhost:11434/api/generate"

typedef struct {
    const char *name;
    const char *summary;
    const char *what;
    const char *usage;
    const char *examples; // multiline
    const char *common;   // multiline (tables/bullets)
    const char *related;  // comma-separated
} BuiltinHelp;

typedef struct {
    const char *flag;
    const char *desc;
} FlagHelp;

static const FlagHelp GCC_FLAGS[] = {
    {"--help", "Show help"},
    {"--version", "Print GCC version"},
    {"-v", "Verbose compiler output"},
    {"-Wall", "Enable common warnings"},
    {"-Wextra", "Enable extra warnings"},
    {"-Werror", "Treat warnings as errors"},
    {"-g", "Include debug symbols"},
    {"-O0", "No optimization"},
    {"-O1", "Optimize"},
    {"-O2", "More optimization"},
    {"-O3", "Max optimization"},
    {"-c", "Compile only (produce .o, do not link)"},
    {"-o", "Set output file name (next arg)"},
    {"-I", "Add include directory (next arg)"},
    {"-L", "Add library directory (next arg)"},
    {"-l", "Link library (next arg, e.g. -lm)"},
    {"-std=c11", "Use C11 standard"},
    {"-std=c17", "Use C17 standard"},
    {"-E", "Preprocess only"},
    {"-S", "Compile to assembly (.s)"},
    {"-MMD", "Generate dependency file for make"},
    {"-MP", "Add phony targets for deps"},
    {"-fsanitize=address", "Address sanitizer (debug memory bugs)"},
    {"-pthread", "Enable pthreads (compile+link)"},
};

static const BuiltinHelp BUILTIN_HELP[] = {
    {
        "cd",
        "Change directory (move between folders)",
        "The cd command stands for Change Directory.\nIt changes your current working directory in the shell (moves you from one folder to another).",
        "cd <directory>\ncd\ncd ~",
        "Examples:\n"
        "  cd Documents\n"
        "  cd ..\n"
        "  cd /tmp\n",
        "Common cd forms:\n"
        "  cd /        - Go to the root directory\n"
        "  cd ~ or cd  - Go to your home directory\n"
        "  cd ..       - Move one level up (parent directory)\n"
        "  cd ../..    - Move two levels up\n"
        "  cd -        - Go back to the previous directory (note: not implemented in this shell)\n",
        "pwd, ls, tree"
    },
    {
        "help",
        "Show available built-in commands",
        "Prints a list of the built-in commands supported by this shell.",
        "help",
        "Examples:\n"
        "  help\n",
        "Tip:\n"
        "  Use TAB on a command name (e.g., 'cd' then TAB) to see detailed help.\n",
        "version, history, pwd"
    },
    {
        "exit",
        "Exit the shell",
        "Exits the shell program and returns you to your normal terminal.",
        "exit",
        "Examples:\n"
        "  exit\n",
        "Notes:\n"
        "  Any running background processes are not managed by this shell.\n",
        "help"
    },
    {
        "bg",
        "Run a command in the background",
        "Runs an external command in the background (does not block the shell).",
        "bg <command> [args...]",
        "Examples:\n"
        "  bg sleep 5\n"
        "  bg python3 script.py\n",
        "Notes:\n"
        "  This runs external programs using execvp().\n"
        "  Output and job control are minimal.\n",
        "history, help"
    },
    {
        "history",
        "Show command history",
        "Shows previously executed commands in this shell session.",
        "history",
        "Examples:\n"
        "  history\n",
        "Notes:\n"
        "  History is kept only for the current session.\n",
        "help"
    },
    {
        "clear",
        "Clear the screen",
        "Clears the terminal screen using ANSI escape codes.",
        "clear",
        "Examples:\n"
        "  clear\n",
        "Tip:\n"
        "  If your terminal scrollback is messy, clear can help.\n",
        "help"
    },
    {
        "echo",
        "Print text",
        "Prints the given text to the terminal.",
        "echo <text...>",
        "Examples:\n"
        "  echo hello\n"
        "  echo \"hello world\"\n",
        "Notes:\n"
        "  This is a simple echo; it does not support advanced flags.\n",
        "pwd, ls"
    },
    {
        "pwd",
        "Print working directory",
        "Prints the current working directory path.",
        "pwd",
        "Examples:\n"
        "  pwd\n",
        "Related:\n"
        "  cd changes directories; pwd shows where you are.\n",
        "cd, ls, tree"
    },
    {
        "ls",
        "List directory contents",
        "Lists files and folders in a directory (non-hidden entries only).",
        "ls\nls <path>",
        "Examples:\n"
        "  ls\n"
        "  ls ..\n",
        "Notes:\n"
        "  This built-in ls is minimal (not the full GNU/BSD ls).\n",
        "tree, count, find"
    },
    {
        "version",
        "Show shell version",
        "Displays the version of this AI Automated Custom Shell.",
        "version",
        "Examples:\n"
        "  version\n",
        "Common version flags (general, not implemented here):\n"
        "  --version  - Version info (many tools)\n"
        "  -v         - Short version output\n"
        "  -V         - Detailed version output\n"
        "  about      - App info (some CLIs)\n",
        "help, whoami, pwd"
    },
    {
        "calc",
        "Simple calculator",
        "Evaluates a simple binary arithmetic expression.",
        "calc <number> <operator> <number>\nOperators: +  -  *  /  %  ^",
        "Examples:\n"
        "  calc 10 + 5\n"
        "  calc 2 ^ 8\n",
        "Notes:\n"
        "  Division by zero is checked.\n",
        "echo, datetime"
    },
    {
        "datetime",
        "Show date and time",
        "Prints the current date and time in a friendly format.",
        "datetime",
        "Examples:\n"
        "  datetime\n",
        "Related:\n"
        "  Useful for timestamps and quick checks.\n",
        "version, pwd"
    },
    {
        "count",
        "Count files and directories",
        "Counts items in a directory and prints totals (directories vs files).",
        "count\ncount <path>",
        "Examples:\n"
        "  count\n"
        "  count ..\n",
        "Notes:\n"
        "  Counting is based on whether opendir() succeeds on an entry.\n",
        "ls, tree, find"
    },
    {
        "find",
        "Find files by name pattern",
        "Recursively searches for files matching a glob pattern (e.g., *.c).",
        "find <pattern>",
        "Examples:\n"
        "  find \"*.c\"\n"
        "  find \"*.txt\"\n",
        "Notes:\n"
        "  Pattern matching uses fnmatch().\n",
        "ls, tree, count"
    },
    {
        "cat",
        "Print a file to the screen",
        "Displays the contents of a file.",
        "cat <filename>",
        "Examples:\n"
        "  cat README.md\n",
        "Notes:\n"
        "  This reads and prints the file as text.\n",
        "ls, find"
    },
    {
        "tree",
        "Show directory tree",
        "Displays a tree view of a directory (folders and files).",
        "tree\ntree <path>",
        "Examples:\n"
        "  tree\n"
        "  tree ..\n",
        "Related:\n"
        "  Use ls for a flat list; tree for structure.\n",
        "ls, count, find"
    },
    {
        "mkdir",
        "Create a directory",
        "Creates a new directory.",
        "mkdir <directory_name>",
        "Examples:\n"
        "  mkdir test_folder\n",
        "Notes:\n"
        "  Permissions are set to 0755.\n",
        "touch, rm, ls"
    },
    {
        "touch",
        "Create an empty file",
        "Creates a file if it doesn't exist (or updates its timestamp).",
        "touch <filename>",
        "Examples:\n"
        "  touch notes.txt\n",
        "Notes:\n"
        "  This uses fopen(..., \"a\") to create/update.\n",
        "cat, ls, rm"
    },
    {
        "rm",
        "Remove a file",
        "Removes (deletes) a file.",
        "rm <filename>",
        "Examples:\n"
        "  rm notes.txt\n",
        "Warning:\n"
        "  This permanently deletes the file (no trash).\n",
        "ls, touch, mkdir"
    },
    {
        "whoami",
        "Show current user",
        "Prints the current username (from the USER environment variable).",
        "whoami",
        "Examples:\n"
        "  whoami\n",
        "Notes:\n"
        "  If USER is not set, it prints 'Unknown user'.\n",
        "pwd, version, help"
    }
};

static int starts_with_icase(const char *s, const char *prefix) {
    if (!s || !prefix) return 0;
    while (*prefix) {
        if (!*s) return 0;
        if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix)) return 0;
        s++;
        prefix++;
    }
    return 1;
}

static const BuiltinHelp* find_help_exact_icase(const char *name) {
    if (!name) return NULL;
    for (size_t i = 0; i < sizeof(BUILTIN_HELP)/sizeof(BUILTIN_HELP[0]); i++) {
        const char *n = BUILTIN_HELP[i].name;
        if (n && strlen(n) == strlen(name) && starts_with_icase(n, name)) {
            return &BUILTIN_HELP[i];
        }
    }
    return NULL;
}

static void print_help_page(const BuiltinHelp *h) {
    if (!h) return;
    printf("Command: %s\n", h->name);
    printf("Summary: %s\n\n", h->summary);
    printf("What it does:\n%s\n\n", h->what);
    printf("Basic usage:\n%s\n\n", h->usage);
    if (h->examples && strlen(h->examples) > 0) {
        printf("%s\n", h->examples);
    }
    if (h->common && strlen(h->common) > 0) {
        printf("%s\n", h->common);
    }
    if (h->related && strlen(h->related) > 0) {
        printf("Related:\n  %s\n", h->related);
    }
}

static void print_match_list(const char *partial) {
    printf("Possible commands for '%s':\n", partial && *partial ? partial : "");
    for (size_t i = 0; i < sizeof(BUILTIN_HELP)/sizeof(BUILTIN_HELP[0]); i++) {
        if (!partial || !*partial || starts_with_icase(BUILTIN_HELP[i].name, partial)) {
            printf("  %-8s - %s\n", BUILTIN_HELP[i].name, BUILTIN_HELP[i].summary);
        }
    }
    printf("\nTip: keep typing to narrow it down, then press TAB again for detailed help.\n");
}

static int is_duplicate_name(const char *name, char names[][256], int count) {
    for (int i = 0; i < count; i++) {
        if (strcmp(names[i], name) == 0) return 1;
    }
    return 0;
}

static int collect_path_executables_with_prefix(const char *prefix, char names[][256], int max_names) {
    if (!prefix || !*prefix) return 0;

    const char *path_env = getenv("PATH");
    if (!path_env || !*path_env) return 0;

    char *path_copy = strdup(path_env);
    if (!path_copy) return 0;

    int count = 0;
    char *saveptr = NULL;
    char *dir_path = strtok_r(path_copy, ":", &saveptr);
    while (dir_path && count < max_names) {
        DIR *d = opendir(dir_path);
        if (d) {
            struct dirent *ent;
            while ((ent = readdir(d)) != NULL && count < max_names) {
                const char *name = ent->d_name;
                if (!name || name[0] == '.') continue;
                if (!starts_with_icase(name, prefix)) continue;

                // Build full path and check executable bit
                char full[1024];
                snprintf(full, sizeof(full), "%s/%s", dir_path, name);
                if (access(full, X_OK) != 0) continue;

                // Avoid duplicates across PATH dirs
                if (is_duplicate_name(name, names, count)) continue;

                snprintf(names[count], 256, "%s", name);
                count++;
            }
            closedir(d);
        }
        dir_path = strtok_r(NULL, ":", &saveptr);
    }

    free(path_copy);
    return count;
}

static int has_hyphen_or_digit(const char *s) {
    for (const char *p = s; p && *p; p++) {
        if (*p == '-' || (*p >= '0' && *p <= '9')) return 1;
    }
    return 0;
}

// Pick a "best" candidate deterministically for common cases like:
// gc -> gcc (prefer short, no hyphen/digits).
// Returns index of best candidate, or -1 if ambiguous/tied.
static int pick_best_external_candidate(char names[][256], int n) {
    if (n <= 0) return -1;

    int best = 0;
    int best_bad = has_hyphen_or_digit(names[0]);
    size_t best_len = strlen(names[0]);
    int tie = 0;

    for (int i = 1; i < n; i++) {
        int bad = has_hyphen_or_digit(names[i]);
        size_t len = strlen(names[i]);

        if (bad < best_bad) {
            best = i; best_bad = bad; best_len = len; tie = 0;
        } else if (bad == best_bad) {
            if (len < best_len) {
                best = i; best_len = len; tie = 0;
            } else if (len == best_len) {
                // Same "quality" and same length -> ambiguous
                tie = 1;
            }
        }
    }

    return tie ? -1 : best;
}

// Returns:
//  0 = no external command match in PATH
//  1 = unique completion found (written to out)
//  2 = multiple matches (no completion written)
int complete_external_command(const char* partial_cmd, char* out, size_t out_sz) {
    if (!out || out_sz == 0) return 0;
    out[0] = '\0';
    if (!partial_cmd || !*partial_cmd) return 0;

    // Only complete the first token (no spaces)
    for (const char *p = partial_cmd; *p; p++) {
        if (*p == ' ' || *p == '\t') return 0;
    }

    char names[32][256];
    int n = collect_path_executables_with_prefix(partial_cmd, names, 32);
    if (n == 1) {
        snprintf(out, out_sz, "%s", names[0]);
        return 1;
    }
    if (n > 1) {
        int best = pick_best_external_candidate(names, n);
        if (best >= 0) {
            snprintf(out, out_sz, "%s", names[best]);
            return 1;
        }
        return 2;
    }
    return 0;
}

// Returns:
//  0 = no built-in match
//  1 = unique completion found (written to out)
//  2 = multiple matches (no completion written)
int complete_builtin_command(const char* partial_cmd, char* out, size_t out_sz) {
    if (!out || out_sz == 0) return 0;
    out[0] = '\0';
    if (!partial_cmd || !*partial_cmd) return 0;

    const BuiltinHelp *exact = find_help_exact_icase(partial_cmd);
    if (exact) {
        snprintf(out, out_sz, "%s", exact->name);
        return 1;
    }

    int match_count = 0;
    const BuiltinHelp *single = NULL;
    for (size_t i = 0; i < sizeof(BUILTIN_HELP)/sizeof(BUILTIN_HELP[0]); i++) {
        if (starts_with_icase(BUILTIN_HELP[i].name, partial_cmd)) {
            match_count++;
            single = &BUILTIN_HELP[i];
        }
    }

    if (match_count == 1 && single) {
        snprintf(out, out_sz, "%s", single->name);
        return 1;
    }
    if (match_count > 1) return 2;
    return 0;
}

static int starts_with(const char *s, const char *prefix) {
    if (!s || !prefix) return 0;
    while (*prefix) {
        if (!*s) return 0;
        if (*s != *prefix) return 0;
        s++; prefix++;
    }
    return 1;
}

static int collect_flag_matches(const FlagHelp *flags, int flags_n, const char *partial, const FlagHelp **out, int max_out) {
    if (!out || max_out <= 0) return 0;
    int n = 0;
    for (int i = 0; i < flags_n && n < max_out; i++) {
        if (!partial || !*partial || starts_with(flags[i].flag, partial)) {
            out[n++] = &flags[i];
        }
    }
    return n;
}

int complete_external_arg(const char* cmd, const char* partial_arg, char* out, size_t out_sz) {
    if (!out || out_sz == 0) return 0;
    out[0] = '\0';
    if (!cmd || !*cmd) return 0;
    if (!partial_arg) partial_arg = "";

    // Only implement deterministic flag completion for gcc for now.
    if (strcmp(cmd, "gcc") != 0) return 0;

    const FlagHelp *matches[32];
    int n = collect_flag_matches(GCC_FLAGS, (int)(sizeof(GCC_FLAGS)/sizeof(GCC_FLAGS[0])), partial_arg, matches, 32);
    if (n == 1) {
        snprintf(out, out_sz, "%s", matches[0]->flag);
        return 1;
    }
    if (n > 1) return 2;
    return 0;
}

void suggest_external_args(const char* cmd, const char* partial_arg) {
    if (!cmd || !*cmd) return;
    if (!partial_arg) partial_arg = "";

    if (strcmp(cmd, "gcc") != 0) {
        printf("No argument suggestions for '%s' yet.\n", cmd);
        printf("Tip: try '%s --help'\n", cmd);
        return;
    }

    printf("gcc format:\n");
    printf("  gcc <source.c> -o <output>\n");
    printf("Examples:\n");
    printf("  gcc hello.c -o hello\n");
    printf("  gcc -Wall -Wextra -Werror -g hello.c -o hello\n\n");

    const FlagHelp *matches[32];
    int n = collect_flag_matches(GCC_FLAGS, (int)(sizeof(GCC_FLAGS)/sizeof(GCC_FLAGS[0])), partial_arg, matches, 32);

    if (partial_arg && *partial_arg) {
        printf("gcc flags matching '%s':\n", partial_arg);
    } else {
        printf("Common gcc flags:\n");
    }

    for (int i = 0; i < n && i < 12; i++) {
        printf("  %-18s - %s\n", matches[i]->flag, matches[i]->desc);
    }
    if (n > 12) {
        printf("  ... (%d more)\n", n - 12);
    }
}

// Struct for curl response
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Function to escape JSON string
static char* escape_json_string(const char* input) {
    if (!input) return NULL;
    
    size_t len = strlen(input);
    char* escaped = malloc(len * 2 + 1); // Worst case: every char needs escaping
    if (!escaped) return NULL;
    
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        switch (input[i]) {
            case '\n':
                escaped[j++] = '\\';
                escaped[j++] = 'n';
                break;
            case '\r':
                escaped[j++] = '\\';
                escaped[j++] = 'r';
                break;
            case '\t':
                escaped[j++] = '\\';
                escaped[j++] = 't';
                break;
            case '\\':
                escaped[j++] = '\\';
                escaped[j++] = '\\';
                break;
            case '"':
                escaped[j++] = '\\';
                escaped[j++] = '"';
                break;
            default:
                escaped[j++] = input[i];
        }
    }
    escaped[j] = '\0';
    return escaped;
}

// Function to handle memory allocation for curl response
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(!ptr) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

// Function to get AI-based command completion using Ollama API
char* get_ollama_completion(const char* prompt) {
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    if (!chunk.memory) {
        return NULL;
    }
    chunk.memory[0] = '\0';
    
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    
    if(curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        // Using tinyllama for quick responses
        char json_data[4096]; // Increased buffer size for larger prompts
        const char* prompt_template;
        
        // Special handling for cd command
        if (strncmp(prompt, "cd", 2) == 0) {
            prompt_template = "User typed: '%s'\n\n"
                            "Complete Command: cd [directory]\n"
                            "What it does: Changes the current working directory\n\n"
                            "Suggested completions:\n"
                            "1. cd ~ (Go to home directory)\n"
                            "2. cd .. (Go up one directory)\n"
                            "3. cd /path/to/directory (Go to specific path)";
        } else {
            // Simplified, direct prompt
            prompt_template = "Complete the command '%s'. Available commands: version, calc, datetime, ls, pwd, whoami, help, tree, find, cat, count, mkdir, touch, rm, clear, echo, cd, exit, history, bg.\n\n"
                            "Reply in this exact format (3 lines only):\n"
                            "Complete: [full command]\n"
                            "Does: [one short sentence]\n"
                            "Similar: [command1], [command2], [command3]";
        }
        
        // Create the full prompt
        char full_prompt[2048];
        snprintf(full_prompt, sizeof(full_prompt), prompt_template, prompt);
        
        // Escape the prompt for JSON
        char* escaped_prompt = escape_json_string(full_prompt);
        if (!escaped_prompt) {
            fprintf(stderr, "Failed to escape prompt\n");
            free(chunk.memory);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return NULL;
        }
        
        // Create the JSON request with stricter parameters for concise output
        snprintf(json_data, sizeof(json_data), 
                "{\"model\": \"tinyllama\", \"prompt\": \"%s\", \"stream\": false, \"temperature\": 0.1, \"top_p\": 0.5, \"top_k\": 20, \"num_predict\": 100}", 
                escaped_prompt);
        
        free(escaped_prompt);
        
        curl_easy_setopt(curl, CURLOPT_URL, OLLAMA_API_URL);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        
        res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            free(chunk.memory);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return NULL;
        }
        
        // Parse the response
        struct json_object *parsed_json = json_tokener_parse(chunk.memory);
        if (!parsed_json) {
            fprintf(stderr, "Failed to parse JSON response\n");
            free(chunk.memory);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return NULL;
        }
        
        struct json_object *response_obj;
        if (json_object_object_get_ex(parsed_json, "response", &response_obj)) {
            const char* response_text = json_object_get_string(response_obj);
            char* result = strdup(response_text);
            
            json_object_put(parsed_json);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            free(chunk.memory);
            curl_global_cleanup();
            return result;
        }
        
        json_object_put(parsed_json);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        free(chunk.memory);
        curl_global_cleanup();
        return NULL;
    }
    
    free(chunk.memory);
    curl_global_cleanup();
    return NULL;
}

// Ask Ollama to describe an external command (short, practical).
static char* get_ollama_command_description(const char* cmd) {
    if (!cmd || !*cmd) return NULL;

    // Deterministic descriptions for common commands (prevents hallucinations)
    if (strcmp(cmd, "gcc") == 0 || starts_with(cmd, "gcc-") || starts_with(cmd, "gcc")) {
        const char *fixed =
            "Does: Compiles C/C++ source files into executables or object files.\n"
            "Example: gcc hello.c -o hello\n"
            "Example: gcc -Wall -Wextra -g hello.c -o hello\n";
        return strdup(fixed);
    }

    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    if (!chunk.memory) return NULL;
    chunk.memory[0] = '\0';

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (!curl) {
        free(chunk.memory);
        curl_global_cleanup();
        return NULL;
    }

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    // Collect a small help snippet to ground the model (reduces hallucinations).
    // We intentionally limit the amount of text.
    char help_snippet[1200];
    help_snippet[0] = '\0';
    {
        char cmdline[512];
        snprintf(cmdline, sizeof(cmdline), "%s --help 2>&1", cmd);
        FILE *fp = popen(cmdline, "r");
        if (fp) {
            size_t total = 0;
            while (total < sizeof(help_snippet) - 1) {
                int ch = fgetc(fp);
                if (ch == EOF) break;
                help_snippet[total++] = (char)ch;
            }
            help_snippet[total] = '\0';
            pclose(fp);
        }
    }

    // Keep it extremely constrained so tinyllama behaves.
    char full_prompt[1800];
    snprintf(full_prompt, sizeof(full_prompt),
             "You are helping a user in a terminal.\n"
             "Command: %s\n"
             "Help output (may be incomplete):\n"
             "-----\n"
             "%s\n"
             "-----\n"
             "Using ONLY the help output above, reply in EXACTLY 3 lines:\n"
             "Does: <one short sentence>\n"
             "Example: <one realistic example>\n"
             "Example: <one realistic example>\n"
             "Do not add any other lines.\n",
             cmd,
             (help_snippet[0] ? help_snippet : "(no help output)"));

    char *escaped_prompt = escape_json_string(full_prompt);
    if (!escaped_prompt) {
        free(chunk.memory);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return NULL;
    }

    char json_data[2048];
    snprintf(json_data, sizeof(json_data),
             "{\"model\": \"tinyllama\", \"prompt\": \"%s\", \"stream\": false, "
             "\"temperature\": 0.1, \"top_p\": 0.5, \"top_k\": 20, \"num_predict\": 80}",
             escaped_prompt);
    free(escaped_prompt);

    curl_easy_setopt(curl, CURLOPT_URL, OLLAMA_API_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        free(chunk.memory);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return NULL;
    }

    struct json_object *parsed_json = json_tokener_parse(chunk.memory);
    if (!parsed_json) {
        free(chunk.memory);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return NULL;
    }

    struct json_object *response_obj;
    char *result = NULL;
    if (json_object_object_get_ex(parsed_json, "response", &response_obj)) {
        const char *response_text = json_object_get_string(response_obj);
        result = strdup(response_text);
    }

    json_object_put(parsed_json);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(chunk.memory);
    curl_global_cleanup();
    return result;
}

static void print_first_n_nonempty_lines(const char *text, int n) {
    if (!text || n <= 0) return;
    int printed = 0;
    const char *p = text;
    while (*p && printed < n) {
        // Skip leading newlines
        while (*p == '\n' || *p == '\r') p++;
        if (!*p) break;

        // Find end of line
        const char *e = p;
        while (*e && *e != '\n' && *e != '\r') e++;

        // Trim whitespace
        while (p < e && (*p == ' ' || *p == '\t')) p++;
        const char *t = e;
        while (t > p && (*(t - 1) == ' ' || *(t - 1) == '\t')) t--;

        if (t > p) {
            printf("%.*s\n", (int)(t - p), p);
            printed++;
        }
        p = e;
    }
}

// Function to suggest next command based on prompt
void suggest_command(const char* partial_cmd) {
    printf("\n");
    printf("\033[1;93m╔══════════════════════════════════════════════════════════════════╗\033[0m\n");
    printf("\033[1;93m║\033[0m  \033[1;96m⚡ AI Suggestions for:\033[0m \033[1;95m'%s'\033[0m\033[1;93m                              ║\033[0m\n", partial_cmd ? partial_cmd : "");
    printf("\033[1;93m╚══════════════════════════════════════════════════════════════════╝\033[0m\n\n");

    // If empty input, show the list of available commands
    if (!partial_cmd || !*partial_cmd) {
        print_match_list("");
        printf("\n");
        return;
    }

    // Built-in match logic (deterministic)
    const BuiltinHelp *exact = find_help_exact_icase(partial_cmd);
    if (exact) {
        print_help_page(exact);
        printf("\n");
        return;
    }

    // Count prefix matches
    int match_count = 0;
    const BuiltinHelp *single = NULL;
    for (size_t i = 0; i < sizeof(BUILTIN_HELP)/sizeof(BUILTIN_HELP[0]); i++) {
        if (starts_with_icase(BUILTIN_HELP[i].name, partial_cmd)) {
            match_count++;
            single = &BUILTIN_HELP[i];
        }
    }

    if (match_count == 1 && single) {
        print_help_page(single);
        printf("\n");
        return;
    }

    if (match_count > 1) {
        print_match_list(partial_cmd);
        printf("\n");
        return;
    }

    // External commands from PATH (deterministic)
    char ext_names[32][256];
    int ext_n = collect_path_executables_with_prefix(partial_cmd, ext_names, 32);
    if (ext_n == 1) {
        printf("External command: %s\n", ext_names[0]);
        char *desc = get_ollama_command_description(ext_names[0]);
        if (desc) {
            print_first_n_nonempty_lines(desc, 3);
            free(desc);
        }
        printf("\nTip: type more arguments after it, e.g. \"%s --help\"\n\n", ext_names[0]);
        return;
    }
    if (ext_n > 1) {
        int best = pick_best_external_candidate(ext_names, ext_n);
        if (best >= 0) {
            printf("Best match: %s\n\n", ext_names[best]);
            char *desc = get_ollama_command_description(ext_names[best]);
            if (desc) {
                print_first_n_nonempty_lines(desc, 3);
                printf("\n");
                free(desc);
            }
        }
        printf("Possible external commands for '%s':\n", partial_cmd);
        for (int i = 0; i < ext_n && i < 15; i++) {
            printf("  %s\n", ext_names[i]);
        }
        if (ext_n > 15) {
            printf("  ... (%d more)\n", ext_n - 15);
        }
        printf("\nTip: keep typing to narrow it down.\n\n");
        return;
    }

    // Fallback to Ollama only when no built-in or PATH matches exist
    printf("No built-in or PATH match for '%s'. Asking Ollama...\n\n", partial_cmd);
    char* ai_suggestion = get_ollama_completion(partial_cmd);
    if (ai_suggestion) {
        printf("%s\n\n", ai_suggestion);
        free(ai_suggestion);
    } else {
        printf("Unable to get AI suggestions. Is Ollama running?\n");
        printf("Try: ollama serve\n");
        printf("Ensure model is installed: ollama pull tinyllama\n\n");
    }
}

// Split a line into tokens
char **ripple_split_line(char *line) {
    int bufsize = RIPPLE_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if (!tokens) {
        fprintf(stderr, "ripple: allocation error\n");
        return NULL;
    }

    token = strtok(line, RIPPLE_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += RIPPLE_TOK_BUFSIZE;
            char **temp = realloc(tokens, bufsize * sizeof(char *));
            if (!temp) {
                fprintf(stderr, "ripple: allocation error\n");
                free(tokens);
                return NULL;
            }
            tokens = temp;
        }

        token = strtok(NULL, RIPPLE_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
} 