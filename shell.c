#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h> // mkdir을 위해 필요한 헤더

#define MAX_LEN 1024

// 신호 처리 핸들러
void handle_signal(int signo) {
    if (signo == SIGINT) {
        printf("\nSIGINT(Ctrl+C) 시그널이 감지되었습니다. 'exit'을 입력하여 종료하세요.\n");
    } else if (signo == SIGQUIT) {
        printf("\nSIGQUIT(Ctrl+\\) 시그널이 감지되었습니다.\n");
    }
}

// ls 명령 처리 함수
void execute_ls() {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(".");
    if (dir == NULL) {
        perror("ls 실행 중 오류 발생");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        printf("%s  ", entry->d_name);
    }
    printf("\n");
    closedir(dir);
}

// pwd 명령 처리 함수
void execute_pwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("현재 디렉토리: %s\n", cwd);
    } else {
        perror("pwd 실행 중 오류 발생");
    }
}

// cd 명령 처리 함수
void execute_cd(char *path) {
    if (path == NULL || strcmp(path, "") == 0) {
        char *home = getenv("HOME");
        if (home == NULL) {
            fprintf(stderr, "HOME 환경 변수가 설정되어 있지 않습니다.\n");
            return;
        }
        if (chdir(home) != 0) {
            perror("cd 실행 오류");
        }
    } else {
        if (chdir(path) != 0) {
            perror("cd 실행 오류");
        }
    }
}

// mkdir 명령 처리 함수
void execute_mkdir(char *dir_name) {
    if (mkdir(dir_name, 0777) == 0) {
        printf("디렉토리 '%s' 생성 성공\n", dir_name);
    } else {
        perror("mkdir 실행 중 오류 발생");
    }
}

// rmdir 명령 처리 함수
void my_rmdir(const char *dir_name) {
    if (rmdir(dir_name) == 0) {
        printf("디렉토리 '%s' 삭제 성공\n", dir_name);
    } else {
        perror("rmdir 실행 중 오류 발생");
    }
}

// ln 명령 처리 함수
void my_ln(const char *src, const char *dest) {
    if (link(src, dest) == 0) {
        printf("'%s'에서 '%s'로 하드 링크 생성 성공\n", src, dest);
    } else {
        perror("ln 실행 중 오류 발생");
    }
}

// cp 명령 처리 함수
void my_cp(const char *src, const char *dest) {
    FILE *source, *destination;
    char buffer[1024];
    size_t bytes;

    source = fopen(src, "rb");
    if (source == NULL) {
        perror("cp 실행 중 오류 발생 (원본 파일 열기 실패)");
        return;
    }

    destination = fopen(dest, "wb");
    if (destination == NULL) {
        perror("cp 실행 중 오류 발생 (대상 파일 열기 실패)");
        fclose(source);
        return;
    }

    while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        fwrite(buffer, 1, bytes, destination);
    }

    printf("파일 '%s'을(를) '%s'로 복사 성공\n", src, dest);

    fclose(source);
    fclose(destination);
}

// rm 명령 처리 함수
void rm_command(const char *file) {
    if (unlink(file) == 0) {
        printf("파일 '%s' 삭제 성공\n", file);
    } else {
        perror("rm 실행 중 오류 발생");
    }
}

// mv 명령 처리 함수
void mv_command(const char *source, const char *destination) {
    if (rename(source, destination) == 0) {
        printf("파일 '%s'을(를) '%s'로 이동/이름 변경 성공\n", source, destination);
    } else {
        perror("mv 실행 중 오류 발생");
    }
}

// cat 명령 처리 함수
void cat_command(const char *file) {
    FILE *f = fopen(file, "r");
    if (f == NULL) {
        perror("cat 실행 중 오류 발생");
        return;
    }

    char buffer[1024];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        fwrite(buffer, 1, bytesRead, stdout);
    }

    fclose(f);
}

// 파일 재지향 처리
void execute_command_with_redirect(char *input) {
    char *args[100];
    char *file = NULL;

    if (strstr(input, ">")) {
        file = strtok(input, ">");
        char *file_name = strtok(NULL, " ");
        if (file_name == NULL) {
            fprintf(stderr, "출력 파일을 지정하지 않았습니다.\n");
            return;
        }
        int fd = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd < 0) {
            perror("파일 열기 실패");
            return;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
        execlp(file, file, NULL);
        perror("명령 실행 실패");
        exit(1);
    }
}

// 파이프 처리 함수
void execute_pipe_command(char *cmd1, char *cmd2) {
    int pipefd[2];
    pipe(pipefd);

    if (fork() == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execlp(cmd1, cmd1, NULL);
        perror("명령 실행 실패");
        exit(1);
    }

    if (fork() == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execlp(cmd2, cmd2, NULL);
        perror("명령 실행 실패");
        exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    wait(NULL);
    wait(NULL);
}

// 명령 실행 함수
void execute_command(char *command) {
    int is_background = 0;
    if (command[strlen(command) - 1] == '&') {
        is_background = 1; // 백그라운드 실행 여부 확인
        command[strlen(command) - 1] = '\0';
    }

    if (strstr(command, "|")) {
        char *cmd1 = strtok(command, "|");
        char *cmd2 = strtok(NULL, "|");
        if (cmd1 && cmd2) {
            execute_pipe_command(cmd1, cmd2); // 파이프 처리
        }
        return;
    }

    if (strstr(command, ">") || strstr(command, "<")) {
        execute_command_with_redirect(command); // 파일 재지향 처리
        return;
    }

    char *token = strtok(command, " ");
    if (token == NULL) return;

    if (strcmp(token, "ls") == 0) {
        execute_ls();
    } else if (strcmp(token, "pwd") == 0) {
        execute_pwd();
    } else if (strcmp(token, "cd") == 0) {
        token = strtok(NULL, " ");
        execute_cd(token);
    } else if (strcmp(token, "mkdir") == 0) {
        token = strtok(NULL, " ");
        if (token != NULL) {
            execute_mkdir(token);
        } else {
            printf("디렉토리 이름을 입력하세요.\n");
        }
    } else if (strcmp(token, "rmdir") == 0) {
        token = strtok(NULL, " ");
        if (token != NULL) {
            my_rmdir(token);
        } else {
            printf("디렉토리 이름을 입력하세요.\n");
        }
    } else if (strcmp(token, "ln") == 0) {
        char *src = strtok(NULL, " ");
        char *dest = strtok(NULL, " ");
        if (src && dest) my_ln(src, dest);
        else printf("사용법: ln <원본> <대상>\n");
    } else if (strcmp(token, "cp") == 0) {
        char *src = strtok(NULL, " ");
        char *dest = strtok(NULL, " ");
        if (src && dest) my_cp(src, dest);
        else printf("사용법: cp <원본> <대상>\n");
    } else if (strcmp(token, "rm") == 0) {
        token = strtok(NULL, " ");
        if (token != NULL) {
            rm_command(token);
        } else {
            printf("파일 이름을 입력하세요.\n");
        }
    } else if (strcmp(token, "mv") == 0) {
        char *src = strtok(NULL, " ");
        char *dest = strtok(NULL, " ");
        if (src && dest) mv_command(src, dest);
        else printf("사용법: mv <원본> <대상>\n");
    } else if (strcmp(token, "cat") == 0) {
        token = strtok(NULL, " ");
        if (token != NULL) {
            cat_command(token);
        } else {
            printf("파일 이름을 입력하세요.\n");
        }
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            execlp(token, token, NULL);
            perror("명령 실행 실패");
            exit(1);
        } else if (pid > 0) {
            if (!is_background) { // 백그라운드 실행 여부 확인
                wait(NULL);
            }
        } else {
            perror("fork 실패");
        }
    }
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGQUIT, handle_signal);

    char input[MAX_LEN];
    while (1) {
        printf("myshell> ");
        if (fgets(input, MAX_LEN, stdin) == NULL) {
            break;
        }
        input[strcspn(input, "\n")] = '\0';
        if (strcmp(input, "exit") == 0) {
            break;
        }
        execute_command(input);
    }

    printf("myshell 종료\n");
    return 0;
}