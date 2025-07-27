#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hardware/adc.h"
#include "hardware/rtc.h"
#include "pico/stdlib.h"

#include "ff.h"
#include "diskio.h"
#include "f_util.h"
#include "hw_config.h"
#include "my_debug.h"
#include "rtc.h"
#include "sd_card.h"

#include "display.h"
#include "acc_giro.h"
#include "led_buzzer_buttons.h"

static bool logger_enabled;
static const uint32_t period = 1000;
static absolute_time_t next_log_time;

static char nome_arquivo[32] = "coleta.csv";
static char nome_arquivo_base[24] = "dadosMPU"; // Altere aqui o nome base desejado
static int numero_coleta = 1;

static sd_card_t *sd_get_by_name(const char *const name)
{
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return sd_get_by_num(i);
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}
static FATFS *sd_get_fs_by_name(const char *name)
{
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return &sd_get_by_num(i)->fatfs;
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}

static void run_setrtc()
{
    const char *dateStr = strtok(NULL, " ");
    if (!dateStr)
    {
        printf("Argumento faltando\n");
        return;
    }
    int date = atoi(dateStr);

    const char *monthStr = strtok(NULL, " ");
    if (!monthStr)
    {
        printf("Argumento faltando\n");
        return;
    }
    int month = atoi(monthStr);

    const char *yearStr = strtok(NULL, " ");
    if (!yearStr)
    {
        printf("Argumento faltando\n");
        return;
    }
    int year = atoi(yearStr) + 2000;

    const char *hourStr = strtok(NULL, " ");
    if (!hourStr)
    {
        printf("Argumento faltando\n");
        return;
    }
    int hour = atoi(hourStr);

    const char *minStr = strtok(NULL, " ");
    if (!minStr)
    {
        printf("Argumento faltando\n");
        return;
    }
    int min = atoi(minStr);

    const char *secStr = strtok(NULL, " ");
    if (!secStr)
    {
        printf("Argumento faltando\n");
        return;
    }
    int sec = atoi(secStr);

    datetime_t t = {
        .year = (int16_t)year,
        .month = (int8_t)month,
        .day = (int8_t)date,
        .dotw = 0, // 0 is Sunday
        .hour = (int8_t)hour,
        .min = (int8_t)min,
        .sec = (int8_t)sec};
    rtc_set_datetime(&t);
}

static void run_format()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Número de unidade lógica desconhecido: \"%s\"\n", arg1);
        return;
    }
    /* Format the drive with default parameters */
    FRESULT fr = f_mkfs(arg1, 0, 0, FF_MAX_SS * 2);
    if (FR_OK != fr)
        printf("Erro em f_mkfs: %s (%d)\n", FRESULT_str(fr), fr);
}
void run_mount()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Número de unidade lógica desconhecido: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_mount(p_fs, arg1, 1);
    if (FR_OK != fr)
    {
        printf("Erro em f_mount: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = true;
    printf("Processo de montagem do SD (%s) concluído\n", pSD->pcName);
}

// Função para montar SD sem depender de argumentos de linha de comando
void run_mount_default()
{
    const char *arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Número de unidade lógica desconhecido: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_mount(p_fs, arg1, 1);
    if (FR_OK != fr)
    {
        printf("Erro em f_mount: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = true;
    printf("Processo de montagem do SD (%s) concluído\n", pSD->pcName);
}
static void run_unmount()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Número de unidade lógica desconhecido: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_unmount(arg1);
    if (FR_OK != fr)
    {
        printf("Erro em f_unmount: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = false;
    pSD->m_Status |= STA_NOINIT; // in case medium is removed
    printf("SD (%s) desmontado\n", pSD->pcName);
}

// Função para desmontar SD sem depender de argumentos de linha de comando
void run_unmount_default()
{
    const char *arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Número de unidade lógica desconhecido: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_unmount(arg1);
    if (FR_OK != fr)
    {
        printf("Erro em f_unmount: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = false;
    pSD->m_Status |= STA_NOINIT;
    printf("SD (%s) desmontado\n", pSD->pcName);
}
static void run_getfree()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = sd_get_by_num(0)->pcName;
    DWORD fre_clust, fre_sect, tot_sect;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs)
    {
        printf("Número de unidade lógica desconhecido: \"%s\"\n", arg1);
        return;
    }
    FRESULT fr = f_getfree(arg1, &fre_clust, &p_fs);
    if (FR_OK != fr)
    {
        printf("Erro em f_getfree: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    tot_sect = (p_fs->n_fatent - 2) * p_fs->csize;
    fre_sect = fre_clust * p_fs->csize;
    printf("%10lu KiB de espaço total.\n%10lu KiB disponíveis.\n", tot_sect / 2, fre_sect / 2);
}
static void run_ls()
{
    const char *arg1 = strtok(NULL, " ");
    if (!arg1)
        arg1 = "";
    char cwdbuf[FF_LFN_BUF] = {0};
    FRESULT fr;
    char const *p_dir;
    if (arg1[0])
    {
        p_dir = arg1;
    }
    else
    {
        fr = f_getcwd(cwdbuf, sizeof cwdbuf);
        if (FR_OK != fr)
        {
            printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
            return;
        }
        p_dir = cwdbuf;
    }
    printf("Listagem do diretório: %s\n", p_dir);
    DIR dj;
    FILINFO fno;
    memset(&dj, 0, sizeof dj);
    memset(&fno, 0, sizeof fno);
    fr = f_findfirst(&dj, &fno, p_dir, "*");
    if (FR_OK != fr)
    {
        printf("Erro em f_findfirst: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    while (fr == FR_OK && fno.fname[0])
    {
        const char *pcWritableFile = "arquivo gravável",
                   *pcReadOnlyFile = "arquivo somente leitura",
                   *pcDirectory = "diretório";
        const char *pcAttrib;
        if (fno.fattrib & AM_DIR)
            pcAttrib = pcDirectory;
        else if (fno.fattrib & AM_RDO)
            pcAttrib = pcReadOnlyFile;
        else
            pcAttrib = pcWritableFile;
        printf("%s [%s] [tamanho=%llu]\n", fno.fname, pcAttrib, fno.fsize);

        fr = f_findnext(&dj, &fno);
    }
    f_closedir(&dj);
}
static void run_cat()
{
    char *arg1 = strtok(NULL, " ");
    if (!arg1)
    {
        printf("Argumento faltando\n");
        return;
    }
    FIL fil;
    FRESULT fr = f_open(&fil, arg1, FA_READ);
    if (FR_OK != fr)
    {
        printf("Erro em f_open: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    char buf[256];
    while (f_gets(buf, sizeof buf, &fil))
    {
        printf("%s", buf);
    }
    fr = f_close(&fil);
    if (FR_OK != fr)
        printf("Erro em f_open: %s (%d)\n", FRESULT_str(fr), fr);
}

// Função para capturar dados do acelerômetro e giroscópio e salvar em arquivo CSV
void capture_imu_data_and_save()
{
    printf("\nCapturando dados do acelerômetro e giroscópio. Aguarde a finalização...\n");
    const int total_amostras = 1000;
    const int intervalo_ms = 100;
    int tempo_total_s = (total_amostras * intervalo_ms) / 1000;
    printf("Tempo estimado: %d segundos\n", tempo_total_s);
    // Gera nome do arquivo com base definida no código
    snprintf(nome_arquivo, sizeof(nome_arquivo), "%s%d.csv", nome_arquivo_base, numero_coleta);
    numero_coleta++;
    FIL file;
    FRESULT res = f_open(&file, nome_arquivo, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK)
    {
        printf("\n[ERRO] Não foi possível abrir o arquivo para escrita. Monte o cartão.\n");
        return;
    }
    UINT bw;
    int16_t accel[3], gyro[3];
    absolute_time_t start_time = get_absolute_time();
    for (int i = 0; i < total_amostras; i++)
    {
        ler_acc_giro(accel, gyro); // Função definida em acc_giro.h/lib2
        char buffer[100];
        sprintf(buffer, "%d,%d,%d,%d,%d,%d,%d\n", i + 1, accel[0], accel[1], accel[2], gyro[0], gyro[1], gyro[2]);
        res = f_write(&file, buffer, strlen(buffer), &bw);
        if (res != FR_OK)
        {
            printf("[ERRO] Não foi possível escrever no arquivo. Monte o cartão.\n");
            f_close(&file);
            return;
        }
        // Exibe tempo restante a cada 50 amostras
        if ((i + 1) % 50 == 0 || i == 0) {
            absolute_time_t now = get_absolute_time();
            int elapsed_ms = to_ms_since_boot(now) - to_ms_since_boot(start_time);
            int remaining_ms = (total_amostras - (i + 1)) * intervalo_ms;
            int remaining_s = remaining_ms / 1000;
            printf("Amostra %d/%d - Tempo restante estimado: %d segundos\n", i + 1, total_amostras, remaining_s);
        }
        sleep_ms(intervalo_ms);
    }
    f_close(&file);
    printf("\nDados do acelerômetro e giroscópio salvos no arquivo %s.\n\n", nome_arquivo);
}

// Função para ler o conteúdo de um arquivo e exibir no terminal
void read_file(const char *nome_arquivo)
{
    FIL file;
    FRESULT res = f_open(&file, nome_arquivo, FA_READ);
    if (res != FR_OK)
    {
        printf("[ERRO] Não foi possível abrir o arquivo para leitura. Verifique se o cartão está montado ou se o arquivo existe.\n");
        return;
    }
    char buffer[128];
    UINT br;
    printf("Conteúdo do arquivo %s:\n", nome_arquivo);
    while (f_read(&file, buffer, sizeof(buffer) - 1, &br) == FR_OK && br > 0)
    {
        buffer[br] = '\0';
        printf("%s", buffer);
    }
    f_close(&file);
    printf("\nLeitura do arquivo %s concluída.\n\n", nome_arquivo);
}

static void run_help()
{
    printf("\nComandos disponíveis:\n\n");
    printf("Digite 'a' para montar o cartão SD\n");
    printf("Digite 'b' para desmontar o cartão SD\n");
    printf("Digite 'c' para listar arquivos\n");
    printf("Digite 'd' para mostrar o conteúdo do arquivo\n");
    printf("Digite 'e' para obter o espaço livre no cartão SD\n");
    printf("Digite 'f' para capturar dados do ADC e salvar no arquivo\n");
    printf("Digite 'g' para formatar o cartão SD\n");
    printf("Digite 'h' para exibir os comandos disponíveis\n");
    printf("\nEscolha o comando:  ");
}

typedef void (*p_fn_t)();
typedef struct
{
    char const *const command;
    p_fn_t const function;
    char const *const help;
} cmd_def_t;

static cmd_def_t cmds[] = {
    {"setrtc", run_setrtc, "setrtc <DD> <MM> <YY> <hh> <mm> <ss>: Set Real Time Clock"},
    {"format", run_format, "format [<drive#:>]: Formata o cartão SD"},
    {"mount", run_mount, "mount [<drive#:>]: Monta o cartão SD"},
    {"unmount", run_unmount, "unmount <drive#:>: Desmonta o cartão SD"},
    {"getfree", run_getfree, "getfree [<drive#:>]: Espaço livre"},
    {"ls", run_ls, "ls: Lista arquivos"},
    {"cat", run_cat, "cat <filename>: Mostra conteúdo do arquivo"},
    {"help", run_help, "help: Mostra comandos disponíveis"}};

static void process_stdio(int cRxedChar)
{
    static char cmd[256];
    static size_t ix;

    if (!isprint(cRxedChar) && !isspace(cRxedChar) && '\r' != cRxedChar &&
        '\b' != cRxedChar && cRxedChar != (char)127)
        return;
    printf("%c", cRxedChar); // eco
    stdio_flush();
    if (cRxedChar == '\r')
    {
        printf("%c", '\n');
        stdio_flush();

        if (!strnlen(cmd, sizeof cmd))
        {
            printf("> ");
            stdio_flush();
            return;
        }
        char *cmdn = strtok(cmd, " ");
        if (cmdn)
        {
            size_t i;
            for (i = 0; i < count_of(cmds); ++i)
            {
                if (0 == strcmp(cmds[i].command, cmdn))
                {
                    (*cmds[i].function)();
                    break;
                }
            }
            if (count_of(cmds) == i)
                printf("Comando \"%s\" não encontrado\n", cmdn);
        }
        ix = 0;
        memset(cmd, 0, sizeof cmd);
        printf("\n> ");
        stdio_flush();
    }
    else
    {
        if (cRxedChar == '\b' || cRxedChar == (char)127)
        {
            if (ix > 0)
            {
                ix--;
                cmd[ix] = '\0';
            }
        }
        else
        {
            if (ix < sizeof cmd - 1)
            {
                cmd[ix] = cRxedChar;
                ix++;
            }
        }
    }
}

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoA 5
#define botaoB 6
#define botaoJ 22

void iniciar_botoes()
{
    gpio_init(botaoA);
    gpio_set_dir(botaoA, GPIO_IN);
    gpio_pull_up(botaoA);
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_init(botaoJ);
    gpio_set_dir(botaoJ, GPIO_IN);
    gpio_pull_up(botaoJ);
}

#define debounce_delay 300
volatile uint tempo_interrupcao = 0;

// Variáveis de estado para captura e montagem do SD
volatile bool captura_ativa = false;
volatile bool sd_montado = false;
volatile bool mount_request_flag = false;
volatile bool capture_request_flag = false;

void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint tempo_atual = to_ms_since_boot(get_absolute_time());

    // Corrige lógica: só executa ação se passou o tempo de debounce
    if (tempo_atual - tempo_interrupcao > debounce_delay) {
        if (gpio == botaoA) {
            // Alterna captura de dados
            if (!captura_ativa) {
                printf("Botão A: Solicitando captura de dados...\n");
                ssd1306_fill(&ssd, false);
                escrever(&ssd, "Capturando...", 10, 2, cor);
                capture_request_flag = true;
            } else {
                // Não há mecanismo de parada imediata, apenas exibe mensagem
                printf("Botão A: Parar captura (não implementado interrupção imediata).\n");
                ssd1306_fill(&ssd, false);
                escrever(&ssd, "Parar: aguarde", 10, 2, cor);
            }
        } else if (gpio == botaoB) {
            // Alterna montagem/desmontagem do SD
            if (!sd_montado) {
                printf("Botão B: Solicitando montagem do SD...\n");
                ssd1306_fill(&ssd, false);
                escrever(&ssd, "Montando SD...", 10, 2, cor);
                mount_request_flag = true;
            } else {
                printf("Botão B: Desmontando SD...\n");
                ssd1306_fill(&ssd, false);
                escrever(&ssd, "Desmontando SD...", 10, 2, cor);
                run_unmount_default();
                sd_montado = false;
                ssd1306_fill(&ssd, false);
                escrever(&ssd, "SD Desmontado", 10, 2, cor);
            }
        } else if (gpio == botaoJ) {
            printf("Botão J pressionado. Reiniciando o dispositivo...\n");
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Reiniciando...", 10, 2, cor);
            reset_usb_boot(0, 0);
        }
        tempo_interrupcao = tempo_atual; // Atualiza o tempo da última interrupção
    }
}

int main()
{
    iniciar_buzzer(); // Inicializa o buzzer
    iniciar_leds(); // Inicializa LEDs
    leds(1, 1, 0); // Amarelo: Inicializando
    init_display(); // Inicializa o display
    iniciar_botoes(); // Inicializa os botões
    gpio_set_irq_enabled_with_callback(botaoA, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(botaoJ, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    ssd1306_fill(&ssd, false); // Limpa o display
    escrever(&ssd, "Iniciando...", 10, 2, cor);

    stdio_init_all();
    sleep_ms(5000);
    time_init();
    adc_init();

    // Inicializa o driver do SD apenas uma vez
    sd_init_driver();

    printf("Exemplo FatFS SPI\n");
    printf("\033[2J\033[H"); // Limpa tela
    printf("\n> ");
    stdio_flush();
    run_help();

    iniciar_i2c_IMU();

    leds(0, 1, 0); // Verde: Pronto para iniciar a captura
    ssd1306_fill(&ssd, false);
    escrever(&ssd, "Aguardando", 10, 2, cor);
    while (true)
    {
        int cRxedChar = getchar_timeout_us(0);
        if (PICO_ERROR_TIMEOUT != cRxedChar)
            process_stdio(cRxedChar);


        // Montagem do SD solicitada pelo botão B
        if (mount_request_flag) {
            printf("\nMontando o SD (via flag)...\n");
            run_mount();
            sd_montado = true;
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "SD Montado", 10, 2, cor);
            printf("\nEscolha o comando (h = help):  ");
            mount_request_flag = false;
        }

        // Captura de dados solicitada pelo botão A
        if (capture_request_flag) {
            captura_ativa = true;
            capture_imu_data_and_save();
            captura_ativa = false;
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Captura OK", 10, 2, cor);
            capture_request_flag = false;
        }

        if (cRxedChar == 'a') // Monta o SD card se pressionar 'a'
        {
            leds(1, 1, 0); // Amarelo: Montando cartão SD
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Montando SD...", 10, 2, cor);
            printf("\nMontando o SD...\n");
            run_mount();
            leds(0, 1, 0); // Verde: Pronto após montagem
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "SD Montado", 10, 2, cor);
            printf("\nEscolha o comando (h = help):  ");
        }
        if (cRxedChar == 'b') // Desmonta o SD card se pressionar 'b'
        {
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Desmontando SD...", 10, 2, cor);
            printf("\nDesmontando o SD. Aguarde...\n");
            run_unmount();
            leds(1, 1, 0); // Amarelo: Estado de espera após desmontar
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "SD Desmontado", 10, 2, cor);
            printf("\nEscolha o comando (h = help):  ");
        }
        if (cRxedChar == 'c') // Lista diretórios e os arquivos se pressionar 'c'
        {
            leds(0, 0, 1); // Azul: Acessando SD
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Listando arquivos...", 10, 2, cor);
            printf("\nListando arquivos no cartão SD...\n");
            run_ls();
            leds(0, 1, 0); // Verde: Pronto
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Listagem OK", 10, 2, cor);
            printf("\nListagem concluída.\n");
            printf("\nEscolha o comando (h = help):  ");
        }
        if (cRxedChar == 'd') // Exibe o conteúdo do arquivo se pressionar 'd'
        {
            leds(0, 0, 1); // Azul: Acessando SD
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Lendo arquivo...", 10, 2, cor);
            read_file(nome_arquivo);
            leds(0, 1, 0); // Verde: Pronto
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Leitura OK", 10, 2, cor);
            printf("Escolha o comando (h = help):  ");
        }
        if (cRxedChar == 'e') // Obtém o espaço livre no SD card se pressionar 'e'
        {
            leds(0, 0, 1); // Azul: Acessando SD
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Espaco livre...", 10, 2, cor);
            printf("\nObtendo espaço livre no SD...\n\n");
            run_getfree();
            leds(0, 1, 0); // Verde: Pronto
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Espaco OK", 10, 2, cor);
            printf("\nEspaço livre obtido.\n");
            printf("\nEscolha o comando (h = help):  ");
        }
        if (cRxedChar == 'f') // Captura dados do IMU e salva no arquivo se pressionar 'f'
        {
            beep_curto(); // Emite um beep curto
            leds(1, 0, 0); // Vermelho: Captura em andamento
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Gravando...", 10, 2, cor);
            capture_imu_data_and_save();
            leds(0, 1, 0); // Verde: Pronto
            ssd1306_fill(&ssd, false);
            beep_duplo(); // Emite um beep duplo
            escrever(&ssd, "Dados Salvos!", 10, 2, cor);
            printf("\nEscolha o comando (h = help):  ");
        }
        if (cRxedChar == 'g') // Formata o SD card se pressionar 'g'
        {
            leds(0, 0, 1); // Azul: Acessando SD
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Formatando SD...", 10, 2, cor);
            printf("\nProcesso de formatação do SD iniciado. Aguarde...\n");
            run_format();
            leds(0, 1, 0); // Verde: Pronto
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "SD Formatado", 10, 2, cor);
            printf("\nFormatação concluída.\n\n");
            printf("\nEscolha o comando (h = help):  ");
        }
        if (cRxedChar == 'h') // Exibe os comandos disponíveis se pressionar 'h'
        {
            run_help();
        }
        sleep_ms(500);
    }
    return 0;
}