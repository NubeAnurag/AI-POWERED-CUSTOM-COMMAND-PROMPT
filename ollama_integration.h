#ifndef OLLAMA_INTEGRATION_H
#define OLLAMA_INTEGRATION_H

#include <stddef.h>

// Function declarations
char* get_ollama_completion(const char* prompt);
void suggest_command(const char* partial_cmd);
int complete_builtin_command(const char* partial_cmd, char* out, size_t out_sz);
int complete_external_command(const char* partial_cmd, char* out, size_t out_sz);
void suggest_external_args(const char* cmd, const char* partial_arg);
int complete_external_arg(const char* cmd, const char* partial_arg, char* out, size_t out_sz);
char* ripple_read_line(void);
char** ripple_split_line(char* line);

// Constants
#define RIPPLE_RL_BUFSIZE 1024
#define RIPPLE_TOK_BUFSIZE 64
#define RIPPLE_TOK_DELIM " \t\r\n\a"
#define RIPPLE_VERSION "1.0.0"
#define OLLAMA_API_URL "http://localhost:11434/api/generate"

#endif // OLLAMA_INTEGRATION_H 