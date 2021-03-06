/*
 * timer1.c
 *
 * Author: Thuan Song Teoh
 * 
 * We setup timer1 to generate an interrupt every 100ms
 * We update 2 global clock tick variables - whose value
 * can be retrieved using the get_lap_timer() and get_timer1_clock_ticks()
 * functions.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "timer1.h"
#include "terminalio.h"
#include "project.h"

/* Our internal clock tick counter - incremented every 100
 * milliseconds. clock_ticks will overflow every ~49 days. */
static volatile uint32_t clock_ticks;

/* Our internal clock tick counter for lap timer - incremented every 100
 * milliseconds. lap_clock_ticks will overflow every ~1.8 hours. Difference with
 * clock_tick is that lap_clock_ticks will get reset. */
static volatile uint16_t lap_clock_ticks;

// Counter toggle for lap timer (0 off, 1 on)
static volatile uint8_t lap_timer = 0;

/* Set up timer 1 to generate an interrupt every 100ms. 
 * We will divide the clock by 64 and count up to 12499.
 * We will therefore get an interrupt every 64 x 12499
 * clock cycles, i.e. every 100 milliseconds with an 8MHz
 * clock. 
 * The counter will be reset to 0 when it reaches it's
 * output compare value.
 */
void init_timer1(void) {
	/* Reset clock ticks count */
	lap_clock_ticks = 0;
	clock_ticks = 0L;

	/* Clear the timer */
	TCNT1 = 0;

	/* Set the output compare value to be 12499 */
	OCR1A = 12499;

	/* Set the timer to clear on compare match (CTC mode)
	 * and to divide the clock by 64. This starts the timer
	 * running.
	 */
	TCCR1A = 0;
	TCCR1B = (1<<WGM12)|(1<<CS11)|(1<<CS10);

	/* Enable an interrupt on output compare match. 
	 * Note that interrupts have to be enabled globally
	 * before the interrupts will fire.
	 */
	TIMSK1 |= (1<<OCIE1A);

	/* Make sure the interrupt flag is cleared by writing a 
	 * 1 to it.
	 */
	TIFR1 &= (1<<OCF1A);
}

uint16_t get_lap_timer(void) {
	uint16_t return_value;

	/* Disable interrupts so we can be sure that the interrupt
	 * doesn't fire when we've copied just a couple of bytes
	 * of the value. Interrupts are re-enabled if they were
	 * enabled at the start.
	 */
	uint8_t interrupts_on = bit_is_set(SREG, SREG_I);
	cli();
	return_value = lap_clock_ticks;
	if(interrupts_on) {
		sei();
	}
	return return_value;
}

uint32_t get_timer1_clock_ticks(void) {
	uint32_t return_value;

	/* Disable interrupts so we can be sure that the interrupt
	 * doesn't fire when we've copied just a couple of bytes
	 * of the value. Interrupts are re-enabled if they were
	 * enabled at the start.
	 */
	uint8_t interrupts_on = bit_is_set(SREG, SREG_I);
	cli();
	return_value = clock_ticks;
	if(interrupts_on) {
		sei();
	}
	return return_value;
}

void start_lap_timer(void) {
	lap_clock_ticks = 0;
	lap_timer = 1;
}

void stop_lap_timer(void) {
	lap_timer = 0;
}

ISR(TIMER1_COMPA_vect) {
	/* Increment our clock tick counters if timer started and not paused */
	if(!is_paused() && lap_timer) {
		lap_clock_ticks++;
	}
	if(!is_paused()) {
		clock_ticks++;
	}
}