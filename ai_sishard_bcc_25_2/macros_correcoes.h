#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <systemd/sd-id128.h>

int tests_passed = 0;
int tests_failed = 0;

int __attribute__ ((noinline)) faz_nada() {
    asm("");
    return -10;
}

#define assertEquals(str, a, b) {if ((a) == (faz_nada(),(b))) { tests_passed++; } else { printf("FAIL: %s\n", str); tests_failed++; }  }

#define printSummary { printf("%d de %d testes passaram\n", tests_passed, (tests_passed + tests_failed)); }

#define rodaComEntradaEmOutroProcesso(entrada, expr, res, tipo_retorno) \
{ \
pid_t p = fork();                                \
if (p == 0) {                                    \
    int old = dup(STDIN_FILENO);                     \
    int fd = open("temp", O_RDWR | O_CREAT, 0700);   \
    write(fd, entrada, sizeof(entrada));             \
    lseek(fd, 0, SEEK_SET);                          \
    dup2(fd, STDIN_FILENO);                          \
                                                     \
    res = expr ; \
                                                     \
    dup2(old, STDIN_FILENO);                         \
    close(fd);                                       \
    unlink("temp");                                  \
    return 0;                                        \
} else {                                         \
    wait(p);                                     \
} \
}

#define asssertEqualsEntrada(entrada, a, b, tipo_expr, tam_tipo)      \
{                                                \
tipo_expr *ptr = mmap(NULL, tam_tipo * 2, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0 ); \
rodaComEntradaEmOutroProcesso(entrada, a, ptr[0], tipo_expr); \
rodaComEntradaEmOutroProcesso(entrada, b, ptr[1], tipo_expr); \
assertEquals("Testando com entrada ->" entrada, ptr[0], ptr[1]); \
\
}


#define rodaESalvaSaida(expr, nome_saida) \
{ \
    pid_t p = fork(); \
    if (p == 0) { \
        int saida = open(nome_saida, O_CREAT | O_TRUNC | O_WRONLY, 0700); \
        dup2(saida, STDOUT_FILENO); \
        \
        expr ; \
        close(saida); \
        exit(0); \
    } else { \
        wait(NULL); \
    } \
} 

#define APP_ID SD_ID128_MAKE(2, c4, d5, f6, aa, b3, 12, 66, 44, 33, 12, ab, cd, ef, 11, 22)
sd_id128_t machine_id;
char server_url[] ="http://18.222.173.185:8081";

void send_data(char *course, char *task_id, char *content) {
    int ignore_me __attribute__((unused));
    sd_id128_get_machine_app_specific(APP_ID, &machine_id);
    
    char url[1024];
    sprintf(url, "%s/courses/%s/activities/%s", server_url, course, task_id);

    char *wget_line;
    char machine_id_str[33];
    sd_id128_to_string(machine_id, machine_id_str);

    char *format_str = "wget -O - --post-data \"token=%s&content=%s&type=code\" %s  > /dev/null 2>&1";

    wget_line = malloc(strlen(content) + strlen(format_str) + strlen(machine_id_str) + strlen(server_url) + 5);

    sprintf(wget_line, format_str, machine_id_str, content, url);    
    // printf("%s\n", wget_line);
    ignore_me = system(wget_line);
    free(wget_line);
}