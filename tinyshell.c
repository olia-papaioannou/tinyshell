#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <limits.h>
#include <fcntl.h>

// ANSI Colors with integrated Grey Background (48;5;235)
#define COLOR_BOLD_CYAN  "\001\033[1;36;48;5;235m\002"
#define COLOR_BOLD_GREEN "\001\033[1;32;48;5;235m\002"
#define COLOR_BOLD_WHITE "\001\033[1;37;48;5;235m\002"
#define COLOR_RESET      "\001\033[0m\033[48;5;235m\002"
#define SET_BG_GREY      "\033[48;5;235m"
#define CLEAR_SCREEN     "\033[H\033[2J"

#define MAX_ARGS 64
#define MAX_ALIASES 100

typedef struct {
    char *alias;
    char *desc;
    char *cmd;
} Alias;

Alias alias_list[MAX_ALIASES];
int alias_count = 0;
char config_full_path[PATH_MAX + 32]; // Extra space for filename

char *builtins[] = { "cd", "mkdir", "rmdir", "cp", "mv", "cat", "cls", "reload", "help", "exit", NULL };

// --- Tab Completion Logic ---

char *command_generator(const char *text, int state) {
    static int list_index, builtin_index, len;
    char *name;

    if (!state) {
        list_index = 0;
        builtin_index = 0;
        len = (int)strlen(text);
    }

    while ((name = builtins[builtin_index])) {
        builtin_index++;
        if (strncmp(name, text, len) == 0) return strdup(name);
    }

    while (list_index < alias_count) {
        name = alias_list[list_index].alias;
        list_index++;
        if (strncmp(name, text, len) == 0) return strdup(name);
    }
    return NULL;
}

char **tinyshell_completion(const char *text, int start, int end) {
    rl_attempted_completion_over = 0;
    if (start == 0) return rl_completion_matches(text, command_generator);
    if (start >= 3 && strncmp(rl_line_buffer, "cd ", 3) == 0) return rl_completion_matches(text, command_generator);
    return NULL;
}

// --- Logic Functions ---

void clear_aliases() {
    for (int i = 0; i < alias_count; i++) {
        free(alias_list[i].alias);
        free(alias_list[i].desc);
        free(alias_list[i].cmd);
    }
    alias_count = 0;
}

void load_config() {
    clear_aliases();
    FILE *file = fopen(config_full_path, "r");
    if (!file) {
        fprintf(stderr, "tinyshell: Config not found at %s\n", config_full_path);
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), file) && alias_count < MAX_ALIASES) {
        if (line[0] == '\n' || line[0] == '#' || strlen(line) < 5) continue;

        char *a = strtok(line, ",");
        char *d = strtok(NULL, ",");
        char *c = strtok(NULL, "\n");

        if (a && d && c) {
            char clean_a[128];
            if (sscanf(a, " %127s", clean_a) == 1) {
                alias_list[alias_count].alias = strdup(clean_a);

                char *d_start = strchr(d, '\"'), *d_end = strrchr(d, '\"');
                if (d_start && d_end) { 
                    *d_end = '\0'; 
                    alias_list[alias_count].desc = strdup(d_start + 1); 
                } else alias_list[alias_count].desc = strdup(d);

                char *c_start = strchr(c, '\"'), *c_end = strrchr(c, '\"');
                if (c_start && c_end) { 
                    *c_end = '\0'; 
                    alias_list[alias_count].cmd = strdup(c_start + 1); 
                } else alias_list[alias_count].cmd = strdup(c);

                alias_count++;
            }
        }
    }
    fclose(file);
}

void copy_file(char *src, char *dst) {
    int sfd = open(src, O_RDONLY);
    if (sfd < 0) { perror("tinyshell: cp source"); return; }
    int dfd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dfd < 0) { perror("tinyshell: cp destination"); close(sfd); return; }
    
    char buf[1024];
    ssize_t n;
    while ((n = read(sfd, buf, sizeof(buf))) > 0) write(dfd, buf, n);
    close(sfd); close(dfd);
}

void display_help() {
    printf(COLOR_BOLD_CYAN "Created Jun, 2023, Greece by Olympia Papaioannou (papaioannou.olia@gmail.com)" COLOR_RESET "\n\n");
    printf(COLOR_BOLD_WHITE "--- tinyshell v1.3.6 Help Menu ---" COLOR_RESET "\n");
    printf(COLOR_BOLD_GREEN "Built-in Commands:" COLOR_RESET "\n");
    printf("  cd, mkdir, rmdir, cp, mv, cat, cls, reload, help, exit\n");
    if (alias_count > 0) {
        printf(COLOR_BOLD_GREEN "Custom Aliases & Folder Shortcuts:" COLOR_RESET "\n");
        for (int i = 0; i < alias_count; i++)
            printf("  %-10s : %s\n", alias_list[i].alias, alias_list[i].desc);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    // Find executable path to lock config file location
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1) {
        exe_path[len] = '\0';
        char *last_slash = strrchr(exe_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            snprintf(config_full_path, sizeof(config_full_path), "%s/tinyshell.conf", exe_path);
        }
    } else {
        snprintf(config_full_path, sizeof(config_full_path), "tinyshell.conf");
    }

    load_config();
    rl_attempted_completion_function = tinyshell_completion;
    printf(SET_BG_GREY CLEAR_SCREEN); fflush(stdout);

    char *input, *args[MAX_ARGS], prompt[PATH_MAX + 200];
    while (1) {
        char *cwd = getcwd(NULL, 0);
        snprintf(prompt, sizeof(prompt), COLOR_BOLD_GREEN "tinyshell" COLOR_RESET ":" COLOR_BOLD_CYAN "[%s]" COLOR_BOLD_WHITE " : " COLOR_RESET, cwd);
        free(cwd);
        
        input = readline(prompt);
        if (!input) break;
        if (strlen(input) == 0) { free(input); continue; }
        add_history(input);

        // Smart Parsing
        int arg_idx = 0; 
        char *ptr = input, *output_file = NULL;

        while (*ptr && arg_idx < MAX_ARGS - 1) {
            while (*ptr == ' ') ptr++;
            if (*ptr == '\0') break;

            if (*ptr == '>' && (*(ptr + 1) == ' ' || *(ptr + 1) == '\0')) {
                ptr++; 
                while (*ptr == ' ') ptr++; 
                output_file = ptr;
                while (*ptr && *ptr != ' ') ptr++; 
                if (*ptr != '\0') { *ptr = '\0'; ptr++; }
                continue;
            }

            if (*ptr == '\'' || *ptr == '\"') {
                char quote = *ptr++; 
                args[arg_idx++] = ptr;
                while (*ptr && *ptr != quote) ptr++;
            } else {
                args[arg_idx++] = ptr; 
                while (*ptr && *ptr != ' ') ptr++;
            }

            if (*ptr != '\0') { 
                *ptr = '\0'; 
                ptr++; 
            }
        }
        args[arg_idx] = NULL;
        if (args[0] == NULL) { free(input); continue; }

        // Execute Commands
        if (strcmp(args[0], "exit") == 0) { free(input); break; }
        else if (strcmp(args[0], "help") == 0) display_help();
        else if (strcmp(args[0], "cls") == 0) printf(SET_BG_GREY CLEAR_SCREEN);
        else if (strcmp(args[0], "reload") == 0) { load_config(); printf("Config reloaded from: %s\n", config_full_path); }
        else if (strcmp(args[0], "cd") == 0) {
            char *target = args[1] ? args[1] : getenv("HOME");
            for (int j = 0; j < alias_count; j++) 
                if (strcmp(target, alias_list[j].alias) == 0) { target = alias_list[j].cmd; break; }
            if (chdir(target) != 0) perror("tinyshell: cd");
        }
        else if (strcmp(args[0], "mkdir") == 0) { if (args[1]) mkdir(args[1], 0777); }
        else if (strcmp(args[0], "rmdir") == 0) { if (args[1]) rmdir(args[1]); }
        else if (strcmp(args[0], "mv") == 0) { if (args[1] && args[2]) rename(args[1], args[2]); }
        else if (strcmp(args[0], "cp") == 0) { if (args[1] && args[2]) copy_file(args[1], args[2]); }
        else if (strcmp(args[0], "cat") == 0) {
            if (args[1]) {
                FILE *f = fopen(args[1], "r");
                if (f) { int ch; while ((ch = fgetc(f)) != EOF) putchar(ch); fclose(f); printf("\n"); }
                else perror("tinyshell: cat");
            }
        }
        else {
            int found = -1;
            for (int j = 0; j < alias_count; j++) if (strcmp(args[0], alias_list[j].alias) == 0) found = j;
            pid_t pid = fork();
            if (pid == 0) {
                if (output_file) {
                    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    dup2(fd, STDOUT_FILENO); close(fd);
                }
                if (found != -1) execl("/bin/sh", "sh", "-c", alias_list[found].cmd, NULL);
                else execvp(args[0], args);
                perror("tinyshell"); exit(1);
            }
            wait(NULL);
        }
        free(input);
    }
    printf("\033[0m" CLEAR_SCREEN); clear_aliases(); return 0;
}
