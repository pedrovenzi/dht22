#include <msp430.h>
#include <stdint.h>

volatile int vetor_tempos[41];
volatile int vetor_bits[41];

volatile uint16_t int_temp;
volatile uint16_t int_umi;
volatile float float_temp;
volatile float float_umi;
volatile uint8_t tempH;
volatile uint8_t tempL;
volatile uint8_t umiH;
volatile uint8_t umiL;
volatile uint8_t checksum;
volatile uint8_t sum;

int traduzir_dados(){
    checksum = 0;
    int_umi = 0;
    int_temp = 0;

    unsigned int i;
    for (i = 1; i < 41; i++){
        if (vetor_tempos[i] > 100){
            vetor_bits[i] = 1;
        } else {
            vetor_bits[i] = 0;
        }
    }

    for (i = 1; i <= 16; i++){
        int_umi += vetor_bits[i]; //crazy coder
        if (i != 16)
            int_umi = int_umi << 1; // int_umi << 1;
    }
    umiH = int_umi/256; // umiH = int_umi; -> umiH >> 8;
    umiL = (int_umi*256)/256; // umiL = int_umi; -> umiL << 8; -> umiL
    float_umi = int_umi;
    float_umi = float_umi/10;

    for (i = 17; i <= 32; i++){
        int_temp += vetor_bits[i];
        if (i != 32)
            int_temp = int_temp << 1; // int_temp << 1;
    }
    tempH = int_temp/256; // tempH = int_temp; -> tempH >> 8;
    tempL = (int_temp*256)/256; // tempL = int_temp; -> temL << 8; -> tempL
    float_temp = int_temp;
    float_temp = float_temp/10;


    for (i = 33; i <= 40; i++){
        checksum += vetor_bits[i];
        if (i != 40)
            checksum = checksum << 1;
    }
    sum = tempH + tempL + umiH + umiL;
    if (sum == checksum){
        return 0;
    } else {
        return 1;
    }
}

void config_timer_start(){
    TA1CTL = TASSEL__SMCLK | MC__UP | TACLR;
    TA1CCR0 = 1100;
}

void config_timer_captura(){
    TA1CTL = TASSEL__SMCLK | MC__CONTINUOUS | TACLR;
    TA1CCTL1 = CM_2 | CAP;
    P2SEL |= BIT0;
}

void receber_dados(){
    config_timer_captura();

    unsigned int i;
    for (i = 0; i < 41; i++){
        while ((TA1CCTL1 & CCIFG) == 0); // POLLING
        vetor_tempos[i] = TA1CCR1;
        TA1CTL |= TACLR;
        TA1CCTL1 &= ~CCIFG;
    }
}

void start_dht22(){
    P2DIR |= BIT0;
    P2OUT |= BIT0;
    P2SEL &= ~BIT0;

    config_timer_start();
    P2OUT &= ~BIT0;

    while((TA1CCTL0 & CCIFG) == 0);
    //TA1CCTL0 &= ~CCIFG;
    TA1CTL = 0;
    TA1CCTL0 = 0;

    P2OUT |= BIT0;
    P2DIR &= ~BIT0;
    P2REN &= ~BIT0;

    while ((P2IN & BIT0) == 1);
}

void config_led_verde(){
    P4DIR |= BIT7;
    P4OUT &= ~BIT7;
}

void config_led_vermelho(){
    P1DIR |= BIT0;
    P1OUT |= BIT0;
}


/**
 * main.c
 */

volatile int status;

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    config_led_verde();
    config_led_vermelho();

    while(1) {
        P4OUT &= ~BIT7;
        P1OUT |= BIT0;
        TA1CTL = TACLR;
        TA1CCTL0 = 0;
        start_dht22();
        receber_dados();
        status = traduzir_dados();
        P4OUT |= BIT7;
        if (status == 0) P1OUT &= ~BIT0;      // apaga vermelho
        __delay_cycles(10000000);             // 10seg
    }

    return 0;
}
