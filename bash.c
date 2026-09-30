#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/signal.h>

#define BUFFER_SIZE 1024
#define MAX_TOKENS 35
#define REDIRECT_LENGTH 256
#define COMMAND_LENGTH 128
#define COMMAND_NUM 256
#define DIRECTORY_MAX_LENGTH 128
#define MAX_JOBS 256

char tokens[MAX_TOKENS][BUFFER_SIZE];
//tokeni
char buffer[BUFFER_SIZE];
//buffer - basically input string
int debug_level = 0;
//debug spremenljivka
char* prompt_def = "mysh";
//default ime za prompt funkcijo
int return_status = 0;
//return status funkcij
bool inBackground = false;
bool redirectIn = false;
bool redirectOut = false;

pid_t live_jobs[MAX_JOBS];
int live_jobs_count = 0;

char currentWD[DIRECTORY_MAX_LENGTH];
char currentPD[DIRECTORY_MAX_LENGTH] = "/proc";
//globalne spremenljivke za redirect in background delovanje

typedef int (*builtin_func)(int arg_num);

//kazalec na funkcijo

typedef struct {
    char* name;
    builtin_func func;
} Builtin;

//struct za funkcije

int builtin_debug(int arg_num);
int builtin_prompt(int arg_num);
int builtin_status(int arg_num);
int builtin_exit(int arg_num);
int builtin_help(int arg_num);
int builtin_print(int arg_num);
int builtin_echo(int arg_num);
int builtin_len(int arg_num);
int builtin_sum(int arg_num);
int builtin_calc(int arg_num);
int builtin_basename(int arg_num);
int builtin_dirname(int arg_num);
int builtin_dirch(int arg_num);
int builtin_dirwd(int arg_num);
int builtin_dirmk(int arg_num);
int builtin_dirrm(int arg_num);
int builtin_dirls(int arg_num);
int builtin_rename(int arg_num);
int builtin_unlink(int arg_num);
int builtin_remove(int arg_num);
int builtin_linkhard(int arg_num);
int builtin_linksoft(int arg_num);
int builtin_linkread(int arg_num);
int builtin_linklist(int arg_num);
int builtin_cpcat(int arg_num);
int builtin_pid(int arg_num);
int builtin_ppid(int arg_num);
int builtin_uid(int arg_num);
int builtin_euid(int arg_num);
int builtin_gid(int arg_num);
int builtin_egid(int arg_num);
int builtin_sysinfo(int arg_num);
int builtin_proc(int arg_num);
int builtin_pids(int arg_num);
int builtin_pinfo(int arg_num);
int waitone(int arg_num);
int waitall(int arg_num);

int external_exec(int arg_num);



Builtin builtins[] = {
    {"debug", builtin_debug},
    {"prompt", builtin_prompt},
    {"status", builtin_status},
    {"exit", builtin_exit},
    {"help", builtin_help},
    {"print", builtin_print},
    {"echo", builtin_echo},
    {"len", builtin_len},
    {"sum", builtin_sum},
    {"calc", builtin_calc},
    {"basename", builtin_basename},
    {"dirname", builtin_dirname},
    {"dirch", builtin_dirch},
    {"dirwd", builtin_dirwd},
    {"dirmk", builtin_dirmk},
    {"dirrm", builtin_dirrm},
    {"dirls", builtin_dirls},
    {"rename", builtin_rename},
    {"unlink", builtin_unlink},
    {"linkhard", builtin_linkhard},
    {"linksoft", builtin_linksoft},
    {"linkread", builtin_linkread},
    {"linklist", builtin_linklist},
    {"cpcat", builtin_cpcat},
    {"pid", builtin_pid},
    {"ppid", builtin_ppid},
    {"uid", builtin_uid},
    {"euid", builtin_euid},
    {"gid", builtin_gid},
    {"egid", builtin_egid},
    {"sysinfo", builtin_sysinfo},
    {"proc", builtin_proc},
    {"pids", builtin_pids},
    {"pinfo", builtin_pinfo},
    {"waitone", waitone},
    {"waitall", waitall},
    {NULL, NULL},
};   

//tabela z funkcijami, z imeni in pointerji na funkcije

builtin_func find_builtin(char* name){
    for(int i = 0; builtins[i].name != NULL; i++){
        if(strcmp(builtins[i].name, name) == 0){
            return builtins[i].func;
        }
    }
    return NULL;
}


void print_token(int token_count){
    printf("Input line: '%s'\n", buffer);
    
    for(int l = 0; l < token_count; l++){
        printf("Token %d: '%s'\n", l, tokens[l]);
    }

}

void print_redirect(char* vhod, char* izhod){


    

    if(redirectIn){
        printf("Input redirect: '%s'\n", vhod);
    }

    if(redirectOut){
        printf("Output redirect: '%s'\n", izhod);

    }

    if(inBackground){
        printf("Background: 1\n");
    }
}


void print_external(int token_count, char* vhod, char* izhod){
    char* stringExternal = calloc(2048, sizeof(char)); 
    bool prvi = true;


    for(int i = 0; i < token_count; i++){
        if(strcmp(tokens[i] + 1, vhod) == 0){
            break;
        }else if(strcmp(tokens[i] + 1, izhod) == 0){
            break;
        }else{
            if(prvi){
                strcat(stringExternal, tokens[i]);
                prvi = false;
            }else{
                strcat(stringExternal, " ");
                strcat(stringExternal, tokens[i]);
            }
        }
    }

    printf("External command '%s'\n", stringExternal);
}


void execute_builtin(builtin_func exec_func, int arg_num, char* vhod, char* izhod){
    fflush(stdin);
    fflush(stdout);
    fflush(stderr);
    if(inBackground){
        pid_t pid = fork();
        if(pid < 0){
            return_status = errno;
            perror("fork");
            return;
        }
        if(pid == 0){
            exec_func(arg_num);
            fflush(stdin);
            fflush(stdout);
            fflush(stderr);
            print_redirect(vhod, izhod);
            _exit(return_status);
            fflush(stdin);
            fflush(stdout);
            fflush(stderr);
            
        }

        live_jobs[live_jobs_count++] = pid;
    }else{
        exec_func(arg_num);
        print_redirect(vhod, izhod);
    }
    fflush(stdin);
    fflush(stdout);
    fflush(stderr);

    // exec_func(arg_num);
    // print_redirect(vhod, izhod);
}


void execute_external(int token_count, char* vhod, char* izhod){

    external_exec(token_count);
}


int builtin_debug(int arg_num){
    if(debug_level <= 0){
        if(arg_num < 2){
            printf("%d\n", debug_level);
        }else{
            if(atoi(tokens[1]) == 0){
                debug_level = 0;
            }else{
                debug_level = atoi(tokens[1]);
            }
        }
    }else{
        if(arg_num < 2){
            print_token(arg_num);
            printf("Executing builtin 'debug' in foreground\n");
            printf("%d\n", debug_level);
        }else{
            if(atoi(tokens[1]) == 0){
                debug_level = 0;
                print_token(arg_num);
            }else{
                debug_level = atoi(tokens[1]);
                // print_token(1);
            }
            printf("Executing builtin 'debug' in foreground\n");
        }
    }
    return_status = 0;
    return 0;
}

int builtin_prompt(int arg_num){
    if(arg_num < 2){
        return_status = 0;
        printf("%s\n", prompt_def);
    }else{
        if(strlen(tokens[1]) > 8){
            return_status = 1;
        }else{
            prompt_def = tokens[1];
            return_status = 0;
        }
    }
    return 0;
}

int builtin_status(int arg_num){
    printf("%d\n", return_status);
}

int builtin_exit(int arg_num){

    if(arg_num >= 2){
        return_status = atoi(tokens[1]);
    }

    if(inBackground){
        return return_status;
    }

    printf("Exit status: %d\n", return_status);
    
}

int builtin_help(int arg_num){
    printf("test");
    return_status = 0;
    return 0;
}

int builtin_print(int arg_num){

    for(int i = 1; i < arg_num; i++){
        if(i == arg_num - 1){
            printf("%s", tokens[i]);
        }else{
            printf("%s ", tokens[i]);
        }
    }
    
    return_status = 0;
}

int builtin_echo(int arg_num){
    for(int i = 1; i < arg_num; i++){
        if(i == arg_num - 1){
            printf("%s", tokens[i]);
        }else{
            printf("%s ", tokens[i]);
        }
    }
    printf("\n");
    return_status = 0;
}

int builtin_len(int arg_num){
    int sum_len = 0;
    if(arg_num < 2){
        printf("0\n");
    }else{

        for(int i = 1; i < arg_num; i++){
            sum_len += strlen(tokens[i]);
        }
        printf("%d\n", sum_len);
    }
    
    return_status = 0;
}

int builtin_sum(int arg_num){
    int sum = 0;
    for(int i = 1; i < arg_num; i++){
        sum += atoi(tokens[i]);
    }
    printf("%d\n", sum);
    return_status = 0;
}

int builtin_calc(int arg_num){
    if(strcmp(tokens[2], "+") == 0){
        printf("%d\n", atoi(tokens[1]) + atoi(tokens[3]));
    }else if(strcmp(tokens[2], "-") == 0){
        printf("%d\n", atoi(tokens[1]) - atoi(tokens[3]));
    }else if(strcmp(tokens[2], "*") == 0){
        printf("%d\n", atoi(tokens[1]) * atoi(tokens[3]));
    }else if(strcmp(tokens[2], "/") == 0){
        printf("%d\n", atoi(tokens[1]) / atoi(tokens[3]));
    }else if(strcmp(tokens[2], "%") == 0){
        printf("%d\n", atoi(tokens[1]) % atoi(tokens[3]));
    }
    return_status = 0;
}

int builtin_basename(int arg_num){
    if(arg_num < 2){
        return_status = 1;
    }else{
        char* last = strrchr(tokens[1], '/');
        last++;
        while(last <= &tokens[1][strlen(tokens[1]) - 1]){
            putchar(*last);
            last++;
        }
        printf("\n");
    return_status = 0;
    }
}

int builtin_dirname(int arg_num){
    if(arg_num < 2){
        return_status = 1;
    }else{
        char* last = strrchr(tokens[1], '/');
        last--;
        char* start = tokens[1];
        while(start <= last){
            putchar(*start);
            start++;
        }
    printf("\n");
    return_status = 0;
    }
}

int builtin_dirch(int arg_num){
    if(arg_num < 2){
        chdir("/");
    }else{
        if(chdir(tokens[1]) != 0){
            fflush(stdout);
            return_status = errno;
            perror("dirch");
            return -1;
        }
    return_status = 0;
    }
}

int builtin_dirwd(int arg_num){
    char* mode;
    getcwd(currentWD, DIRECTORY_MAX_LENGTH);
    char* last = strrchr(currentWD, '/');
    last++;
    if(arg_num < 2){
        if(strlen(currentWD) == 1){
            printf("%s\n", currentWD);
        }else{
            while(last <= &currentWD[strlen(currentWD) - 1]){
                putchar(*last);
                last++;
            }
            printf("\n");
        }
    }else{
        if(strcmp(tokens[1], "base") == 0){
            if(strlen(currentWD) == 1){
                printf("%s\n", currentWD);
            }else{
                while(last <= &currentWD[strlen(currentWD) - 1]){
                    putchar(*last);
                    last++;
                }
            printf("\n");
            }
        
        }else if(strcmp(tokens[1], "full") == 0){
            printf("%s\n", currentWD);
        }
    }
    return_status = 0;
}

int builtin_dirmk(int arg_num){

    if(mkdir(tokens[1], 0700) != 0){
        fflush(stdout);
        return_status = errno;
        perror("dirmk");
        return -1;
    }
}

int builtin_dirrm(int arg_num){
    if(rmdir(tokens[1]) != 0){
        fflush(stdout);
        return_status = errno;
        perror("dirrm");
        return -1;
    }
}

int builtin_dirls(int arg_num){
    char* dir_path = (arg_num < 2) ? "." : tokens[1]; 

    bool first = true;
    DIR* dir = opendir(dir_path);
    struct dirent * entry;
    while ( (entry = readdir(dir)) != 0) {
        if(!first){
            printf("  ");
        }
        printf("%s", entry->d_name);
        first = false;
    }
    printf("\n");
    closedir(dir);
    return_status = 0;
}

int builtin_rename(int arg_num){
    if(rename(tokens[1], tokens[2]) != 0){
        fflush(stdout);
        return_status = errno;
        perror("rename");
    }else{
        return_status = 0;
    }
}

int builtin_unlink(int arg_num){
    if(unlink(tokens[1]) != 0){
        fflush(stdout);
        return_status = errno;
        perror("unlink");
    }else{
        return_status = 0;
    }
}

int builtin_remove(int arg_num){

}

int builtin_linkhard(int arg_num){
    if(link(tokens[1], tokens[2]) != 0){
        fflush(stdout);
        return_status = errno;
        perror("linkhard");
    }else{
        return_status = 0;
    }
}

int builtin_linksoft(int arg_num){
    if(symlink(tokens[1], tokens[2]) != 0){
        fflush(stdout);
        return_status = errno;
        perror("linksoft");
    }else{
        return_status = 0;
    }
}

int builtin_linkread(int arg_num){
    char* linkread_buffer = malloc(DIRECTORY_MAX_LENGTH * sizeof(char));
    if(readlink(tokens[1], linkread_buffer, DIRECTORY_MAX_LENGTH) < 0){
        fflush(stdout);
        return_status = errno;
        perror("linkread");
    }else{  
        printf("%s\n", linkread_buffer);
        return_status = 0;
    }
    
}

int builtin_linklist(int arg_num){
    struct stat st;
    int inode = stat(tokens[1], &st);
    // printf("%d\n", inode);
    
    bool first = true;

    struct dirent *entry;
    DIR* dir = opendir(currentWD);
    while((entry = readdir(dir)) != 0){
        struct stat test;
        stat(entry->d_name, &test);
        if(test.st_ino == st.st_ino){
            if(!first){
                printf("  %s", entry->d_name);
            }else{
                printf("%s", entry->d_name);
                first = false;
            }
        }
    }
    printf("\n");
}

int builtin_cpcat(int arg_num){
    int input_fd, output_fd, bytes_read, bytes_written;

    char bufferCPCAT[256];

    fflush(stdout);
    //ce je st argumentov 1 (ker arg 1 je keyword)
    if(arg_num == 2){
        input_fd = open(tokens[1], O_RDONLY);
        if(input_fd < 0){
            return_status = errno;
            perror("cpcat");
            return return_status;
        }
        output_fd = STDOUT_FILENO;
        while((bytes_read = read(input_fd, bufferCPCAT, 256)) > 0){
            if(bytes_read < 0){
                return_status = errno;
                perror("cpcat");
                return return_status;
            }
            bytes_written = write(output_fd, bufferCPCAT, bytes_read);
            if(bytes_written < 0){
                return_status = errno;
                perror("cpcat");
                return return_status;
            }
        }
        close(input_fd);
    }else if(arg_num == 3){
        input_fd = open(tokens[1], O_RDONLY);
        output_fd = open(tokens[2], O_WRONLY | O_CREAT | O_TRUNC,
                            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if(input_fd < 0){
            return_status = errno;
            perror("cpcat");
            return return_status;
        }
        if(output_fd < 0){
            return_status = errno;
            perror("cpcat");
            return return_status;
        }
        while((bytes_read = read(input_fd, bufferCPCAT, 256)) > 0){
            if(bytes_read < 0){
                return_status = errno;
                perror("cpcat");
                return return_status;
            }
            bytes_written = write(output_fd, bufferCPCAT, bytes_read);
            if(bytes_written < 0){
                return_status = errno;
                perror("cpcat");
                return return_status;
            }        }
        close(input_fd);
        close(output_fd);
        
    }
    fflush(stdout);
    fflush(stderr);
    return return_status;
}

int builtin_pid(int arg_num){
    pid_t pid = getpid();
    printf("%d\n", pid);
}

int builtin_ppid(int arg_num){
    pid_t ppid = getppid();
    printf("%d\n", ppid);
}

int builtin_uid(int arg_num){
    uid_t uid = getuid();
    printf("%d\n", uid);
}

int builtin_euid(int arg_num){
    uid_t euid = geteuid();
    printf("%d\n", euid);
}

int builtin_gid(int arg_num){
    // if(arg_num >= 2){
    //     printf("Input error");
    // }else{
    gid_t gid = getgid();
    printf("%d\n", gid);
    // }
}

int builtin_egid(int arg_num){
    gid_t egid = getegid();
    printf("%d\n", egid);
}

int builtin_sysinfo(int arg_num){
    struct utsname mysystem;
    if(uname(&mysystem) < 0){
        fflush(stdout);
        return_status = errno;
        perror("sysinfo");
    }

    printf("Sysname: %s\n", mysystem.sysname);
    printf("Nodename: %s\n", mysystem.nodename);
    printf("Release: %s\n", mysystem.release);
    printf("Version: %s\n", mysystem.version);
    printf("Machine: %s\n", mysystem.machine);

}

int builtin_proc(int arg_num){
    if(arg_num < 2){
        printf("%s\n", currentPD);
        return_status = 0;
    }else{
        if(chdir(tokens[1]) != 0){
            // fflush(stdout);
            // perror("proc");
            return_status = 1;
        }else{
            getcwd(currentPD, DIRECTORY_MAX_LENGTH);
            // printf("%s\n", currentPD);
        return_status = 0;
        }
    }    
}

int compare(const void* a, const void* b){
    return (*(int*) a - *(int*) b);
}

int builtin_pids(int arg_num){
    fflush(stdin);
    fflush(stdout);
    fflush(stderr);

    struct dirent *process;
    DIR* proc = opendir(currentPD);
    int* tabelaProcesov = malloc(256 * sizeof(int));
    int procNum = 0;

    while((process = readdir(proc)) != 0){
        if(atoi(process->d_name) != 0){
            tabelaProcesov[procNum]= atoi(process->d_name);
            procNum++;
        }
    }
    qsort(tabelaProcesov, procNum, sizeof(int), compare);
    
    for(int i = 0; i < procNum; i++){
            printf("%d\n", tabelaProcesov[i]);
    }
    // fflush(stdin);
    // fflush(stdout);
    // fflush(stderr);

}

int builtin_pinfo(int arg_num){
    struct dirent *process;
    DIR* proc = opendir(currentPD);
    int* tabelaProcesov = malloc(256 * sizeof(int));
    int procNum = 0;
    FILE *fp;
    char path[266];
    int pid;
    int ppid;
    char state;
    char name[128];



    while((process = readdir(proc)) != 0){
        if(atoi(process->d_name) != 0){
            tabelaProcesov[procNum]= atoi(process->d_name);
            procNum++;
        }
    }
    qsort(tabelaProcesov, procNum, sizeof(int), compare);
    
    printf("%5s %5s %6s %s\n", "PID", "PPID", "STANJE", "IME");
    for(int i = 0; i < procNum; i++){
        snprintf(path, sizeof(path), "/%s/%d/stat", currentPD, tabelaProcesov[i]);
        fp = fopen(path, "r");

        fscanf(fp, "%d (%[^)]) %c %d",
            &pid, &name, &state, &ppid
        );
        //tist weird ta drug removea oklepaje z obeh strani besede
        printf("%5d %5d %6c %s\n", pid, ppid, state, name);
    }
    fclose(fp);

}

int external_exec(int arg_num) {
    fflush(stdin);
    fflush(stdout);
    fflush(stderr);

    pid_t pid = fork();



    if (pid < 0) {
        return_status = errno;
        perror("fork");
        return return_status;
    }
    if (pid == 0) {
        // CHILD: build argv and exec
        char **args = malloc((arg_num + 1) * sizeof(char *));
        for (int i = 0; i < arg_num; i++) {
            args[i] = strdup(tokens[i]);
        }
        args[arg_num] = NULL;

        execvp(tokens[0], args);
        // if execvp returns, it failed:
        perror("exec");
        // free before exiting
        for (int i = 0; i < arg_num; i++) {
            free(args[i]);
        }
        free(args);
        _exit(127);
    }

    // PARENT
    if (inBackground) {
        // stash for later reaping, don’t wait
        if (live_jobs_count < MAX_JOBS) {
            live_jobs[live_jobs_count++] = pid;
        }
        return_status = 0;
    } else {
        int status;
        if (waitpid(pid, &status, 0) < 0) {
            return_status = errno;
            perror("waitpid");
        } else if (WIFEXITED(status)) {
            return_status = WEXITSTATUS(status);
        } else {
            // e.g. killed by signal
            return_status = 128 + WTERMSIG(status);
        }
    }
    fflush(stdin);
    fflush(stdout);
    fflush(stderr);
    return return_status;
}



int remove_job(pid_t pid){

    for(int i = 0; i < live_jobs_count; i++){
        if(live_jobs[i] == pid){
            live_jobs[i] = live_jobs[--live_jobs_count];
        }
    }

}

int waitone(int arg_num){
    
    int status;
    pid_t child_pid;

    if(arg_num == 2){
        child_pid = atoi(tokens[1]);   


        waitpid(child_pid, &status, 0);

    }else{
        child_pid = wait(&status);
    
    }
    
    if(WIFEXITED(status)){
        return_status = WEXITSTATUS(status);
    }else{
        return_status = 0;
    }
    remove_job(child_pid);
    return return_status;
}

int waitall(int arg_num){
    int status;
    pid_t pid;

    while(live_jobs_count > 0){
        pid = wait(&status);
        if(pid < 0){
            if(errno == ECHILD) break;
            return_status = errno;
            perror("waitall");
        }
        remove_job(pid);
    }

    return_status = 0;
    return return_status;
}

int tokenize(){

    int i = 0;
    int token_count = 0;
    int internalCounter = 0;
    
    buffer[strlen(buffer) - 1] = '\0';


    // beremo ukaz in ko pridemo ali do konca vnosov '\0' ali do konca vrstice '\n' ali pa do presledka gremo v novo vrstico
    // bele znake preskocimo 
    // nize obdane z narekovaji prepisemo vse kar je znotraj


    while (buffer[i] != '\0') {
        while (isspace(buffer[i])) {
            i++;
        }

        if (buffer[i] == '\0' || buffer[i] == '\n') break;

        if (buffer[i] == '#') {
            break; 
        }

        if (buffer[i] == '"') {
            i++; 
            while (buffer[i] != '"' && buffer[i] != '\0') {
                tokens[token_count][internalCounter++] = buffer[i++];
            }
            if (buffer[i] == '"') i++; 
        } else {
            while (!isspace(buffer[i]) && buffer[i] != '\0') {
                tokens[token_count][internalCounter++] = buffer[i++];
            }
        }

        tokens[token_count][internalCounter] = '\0';
        token_count++;
        internalCounter = 0;
    }




    return token_count;
}   


void parse(int token_count){


    inBackground = false;
    redirectIn = false;
    redirectOut = false;
    // printf("%s", tokens[0]);

    char* izhod = calloc(REDIRECT_LENGTH, sizeof(char));
    char* vhod = calloc(REDIRECT_LENGTH, sizeof(char));


    for(int i = token_count - 1; i >= 0; i--){
        // printf("%c", tokens[i][0]);
        if(tokens[i][0] == '&'){
            // printf("flagB");
            inBackground = true;
            i--;
        }
        if(tokens[i][0] == '>'){
            strcpy(izhod, tokens[i]);
            // printf("flagI");
            izhod = izhod+1;
            redirectOut = true;
            i--;
        }
        if(tokens[i][0] == '<'){
            strcpy(vhod, tokens[i]);
            // printf("flagO");
            vhod = vhod+1;
            redirectIn = true;    
            i--;    
        }
    }

    if(inBackground){
        token_count--;
    }
    if(redirectIn){
        token_count--;
    }
    if(redirectOut){
        token_count--;
    }

    builtin_func searched_builtin_func = find_builtin(tokens[0]);
    if(searched_builtin_func == NULL){
        execute_external(token_count, vhod, izhod);
    }else{
        execute_builtin(searched_builtin_func, token_count, vhod, izhod);
    }


}

void sigchld_handler(){
    int st, saved_errno = errno;
    pid_t pid;


    while((pid = waitpid(-1, &st, WNOHANG)) > 0){
        remove_job(pid);
    }
    errno = saved_errno;
}


int main(int argc, char* argv[]){
    
    int terminal_mode = isatty(STDIN_FILENO);

    // char* buffer = calloc(BUFFER_SIZE, sizeof(char));
    // char** tokens = calloc(MAX_TOKENS, sizeof(char*));
    // for(int j = 0; j < MAX_TOKENS; j++){
    //     tokens[j] = calloc(BUFFER_SIZE, sizeof(char));
    // }
    // to je zdej globalna tabela tako da ni treba tega vec
    

    sigchld_handler();

    while(fgets(buffer, BUFFER_SIZE, stdin) != NULL){
        // printf("%s", buffer);
        int token_count = tokenize();

        if(token_count == 0) continue;
        parse(token_count);
    }
    


    return return_status;
}