// Sammy Mohamed - EECE 3624 final project
// ZumoBot Co-Pilot: ReadyAVR (ATmega128A) joystick controller

#ifndef F_CPU
#define F_CPU 7372800UL
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#define BAUD 9600UL
#define UBRR0_VALUE ((F_CPU / (16UL * BAUD)) - 1UL)

// joystick on PORTB, active low (PB4 is the center push, not used)
#define JOY_PIN    PINB
#define JOY_PORT   PORTB
#define JOY_DDR    DDRB
#define JOY_RIGHT  0
#define JOY_DOWN   1
#define JOY_LEFT   2
#define JOY_UP     3
#define JOY_MASK   ((1 << JOY_UP) | (1 << JOY_DOWN) | (1 << JOY_LEFT) | (1 << JOY_RIGHT))

// LEDs 0-3 on PORTC, active low
#define LED_PORT   PORTC
#define LED_DDR    DDRC
#define LED_MASK   0x0F

#define SEND_PERIOD_MS 50

// last status from the zumo: M = moving, E = edge
volatile char status = 'M';

// USART0 (PE0/PE1) to the master HC-05, 9600 8N1
void bt_init(void)
{
    UBRR0H = (uint8_t)(UBRR0_VALUE >> 8);
    UBRR0L = (uint8_t)UBRR0_VALUE;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void bt_send(char c)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = (uint8_t)c;
}

// status from the zumo
ISR(USART0_RX_vect)
{
    char c = (char)UDR0;
    if (c == 'M' || c == 'E')
        status = c;
}

// F, B, L, R, or S when released
char read_direction(void)
{
    uint8_t p = (uint8_t)~JOY_PIN;
    if (p & (1 << JOY_UP))    return 'F';
    if (p & (1 << JOY_DOWN))  return 'B';
    if (p & (1 << JOY_LEFT))  return 'L';
    if (p & (1 << JOY_RIGHT)) return 'R';
    return 'S';
}

// LED0 = ok, LED1 = driving, LED3 = edge
void show_status(char s, char cmd)
{
    uint8_t pattern = 0x01;
    if (s == 'E') pattern = 0x08;
    if (cmd != 'S') pattern |= 0x02;
    LED_PORT = (uint8_t)((LED_PORT & ~LED_MASK) | (~pattern & LED_MASK));
}

int main(void)
{
    JOY_DDR  &= (uint8_t)~JOY_MASK;   // inputs
    JOY_PORT |= JOY_MASK;             // pull-ups
    LED_DDR  |= LED_MASK;

    bt_init();
    sei();

    // send a letter every 50 ms so the zumo's watchdog stays happy
    while (1) {
        char cmd = read_direction();
        bt_send(cmd);
        show_status(status, cmd);
        _delay_ms(SEND_PERIOD_MS);
    }
}
