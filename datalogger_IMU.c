#include "funcoes_projeto.h"

char nome_arquivo[32] = "tarefa.csv";
int numero_coleta = 1;

const int total_amostras = 1000;
const int intervalo_ms = 10;

int main()
{
    iniciar_buzzer(); // Inicializa o buzzer
    iniciar_leds(); // Inicializa LEDs
    leds(1, 1, 0); // Amarelo: Inicializando (vermelho + verde)
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

    leds(0, 1, 0); // Verde: Sistema pronto
    ssd1306_fill(&ssd, false);
    escrever(&ssd, "Aguardando", 10, 2, cor);
    while (true)
    {
        int cRxedChar = getchar_timeout_us(0);
        if (PICO_ERROR_TIMEOUT != cRxedChar)
            process_stdio(cRxedChar);


        // Montagem do SD solicitada pelo botão B
        if (mount_request_flag) {
            leds(1, 1, 0); // Amarelo: Montando cartão SD
            printf("\nMontando o SD (via flag)...\n");
            run_mount();
            if (sd_get_by_num(0)->mounted) {
                leds(0, 1, 0); // Verde: Pronto após montagem
                ssd1306_fill(&ssd, false);
                escrever(&ssd, "SD Montado", 10, 2, cor);
            } else {
                // Erro ao montar SD
                for (int i = 0; i < 6; i++) {
                    leds(1, 0, 1); // Roxo: Vermelho + Azul piscando
                    sleep_ms(150);
                    leds(0, 0, 0);
                    sleep_ms(150);
                }
                ssd1306_fill(&ssd, false);
                escrever(&ssd, "ERRO SD", 10, 2, cor);
                printf("ERRO SD\n");
            }
            printf("\nEscolha o comando (h = help):  ");
            mount_request_flag = false;
        }

        // Captura de dados solicitada pelo botão A
        if (capture_request_flag) {
            captura_ativa = true;
            leds(1, 0, 0); // Vermelho: Captura em andamento
            beep_curto(); // Beep curto ao iniciar captura (fora da interrupção)
            capture_imu_data_and_save();
            captura_ativa = false;
            leds(0, 1, 0); // Verde: Pronto após captura
            beep_duplo(); // Dois beeps curtos ao finalizar captura (fora da interrupção)
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
            if (sd_get_by_num(0)->mounted) {
                leds(0, 1, 0); // Verde: Pronto após montagem
                ssd1306_fill(&ssd, false);
                escrever(&ssd, "SD Montado", 10, 2, cor);
            } else {
                // Erro ao montar SD
                for (int i = 0; i < 6; i++) {
                    leds(1, 0, 1); // Roxo: Vermelho + Azul piscando
                    sleep_ms(150);
                    leds(0, 0, 0);
                    sleep_ms(150);
                }
                ssd1306_fill(&ssd, false);
                escrever(&ssd, "ERRO SD", 10, 2, cor);
                printf("ERRO SD\n");
            }
            printf("\nEscolha o comando (h = help):  ");
        }
        if (cRxedChar == 'b') // Desmonta o SD card se pressionar 'b'
        {
            leds(1, 1, 0); // Amarelo: Estado de espera após desmontar
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Desmontando SD...", 10, 2, cor);
            printf("\nDesmontando o SD. Aguarde...\n");
            run_unmount();
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "SD Desmontado", 10, 2, cor);
            printf("\nEscolha o comando (h = help):  ");
        }
        if (cRxedChar == 'c') // Lista diretórios e os arquivos se pressionar 'c'
        {
            // Azul piscando: Acessando SD
            for (int i = 0; i < 4; i++) {
                leds(0, 0, 1);
                sleep_ms(120);
                leds(0, 0, 0);
                sleep_ms(120);
            }
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
            // Azul piscando: Acessando SD
            for (int i = 0; i < 4; i++) {
                leds(0, 0, 1);
                sleep_ms(120);
                leds(0, 0, 0);
                sleep_ms(120);
            }
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
            // Azul piscando: Acessando SD
            for (int i = 0; i < 4; i++) {
                leds(0, 0, 1);
                sleep_ms(120);
                leds(0, 0, 0);
                sleep_ms(120);
            }
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
            beep_curto(); // Beep curto ao iniciar captura
            leds(1, 0, 0); // Vermelho: Captura em andamento
            ssd1306_fill(&ssd, false);
            escrever(&ssd, "Gravando...", 10, 2, cor);
            capture_imu_data_and_save();
            leds(0, 1, 0); // Verde: Pronto
            ssd1306_fill(&ssd, false);
            beep_duplo(); // Dois beeps curtos ao finalizar captura
            escrever(&ssd, "Dados Salvos!", 10, 2, cor);
            printf("\nEscolha o comando (h = help):  ");
        }
        if (cRxedChar == 'g') // Formata o SD card se pressionar 'g'
        {
            // Azul piscando: Acessando SD
            for (int i = 0; i < 4; i++) {
                leds(0, 0, 1);
                sleep_ms(120);
                leds(0, 0, 0);
                sleep_ms(120);
            }
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
        sleep_ms(100);
    }
    return 0;
}