#include <stdint.h>
#include <stdio.h>
#include "system.h"
#include "io.h"

/* ============================================================
   REGISTRES CAPTEURS
   ============================================================ */
#define REG_READY_VECT   0x00
#define REG_NIVEAU       0x01

/* ============================================================
   REGISTRES PWM
   ============================================================ */
#define PWM_RIGHT         0x00
#define PWM_LEFT          0x04

/* ============================================================
   PARAMETRES PWM
   ============================================================ */
#define PWM_GO            (1 << 13)
#define PWM_DIR           (1 << 12)

#define FORWARD           0
#define BACKWARD          1

/* Directions physiques adaptées à ton robot */
#define RIGHT_FORWARD_DIR 0
#define RIGHT_BACKWARD_DIR 1

#define LEFT_FORWARD_DIR  1
#define LEFT_BACKWARD_DIR 0

/* Duty uniquement, pas commande complète */
#define SPEED_MIN         0x08C0
#define BASE_SPEED        0x08FF
#define SEARCH_SPEED      0x08C0
#define SPEED_MAX         0x0935

/* ============================================================
   PID simple
   ============================================================ */
#define KP                50
#define KD                5

/* ============================================================
   SEUIL CAPTEUR
   ============================================================ */
#define NIVEAU            0x6C


void delay_ms(uint32_t ms)
{
    volatile uint32_t i;

    while (ms > 0)
    {
        for (i = 0; i < 5000; i++);
        ms--;
    }
}


uint32_t pwm_cmd(int dir_bit, uint32_t speed)
{
    if (speed > SPEED_MAX)
        speed = SPEED_MAX;

    return PWM_GO | (dir_bit ? PWM_DIR : 0) | (speed & 0x0FFF);
}


void set_moteur_droit(int physical_dir, uint32_t speed)
{
    int dir_bit;

    if (physical_dir == FORWARD)
        dir_bit = RIGHT_FORWARD_DIR;
    else
        dir_bit = RIGHT_BACKWARD_DIR;

    IOWR_32DIRECT(
        PWM_GENERATION_AVALON_INTERFACE_0_BASE,
        PWM_RIGHT,
        pwm_cmd(dir_bit, speed)
    );
}


void set_moteur_gauche(int physical_dir, uint32_t speed)
{
    int dir_bit;

    if (physical_dir == FORWARD)
        dir_bit = LEFT_FORWARD_DIR;
    else
        dir_bit = LEFT_BACKWARD_DIR;

    IOWR_32DIRECT(
        PWM_GENERATION_AVALON_INTERFACE_0_BASE,
        PWM_LEFT,
        pwm_cmd(dir_bit, speed)
    );
}


void stop_moteurs(void)
{
    IOWR_32DIRECT(PWM_GENERATION_AVALON_INTERFACE_0_BASE, PWM_RIGHT, 0x00000000);
    IOWR_32DIRECT(PWM_GENERATION_AVALON_INTERFACE_0_BASE, PWM_LEFT,  0x00000000);
}


/* Pivot droite : roue droite arrière, roue gauche avant */
void chercher_droite(void)
{
    set_moteur_droit(BACKWARD, SEARCH_SPEED);
    set_moteur_gauche(FORWARD,  SEARCH_SPEED);
}


/* Pivot gauche : roue droite avant, roue gauche arrière */
void chercher_gauche(void)
{
    set_moteur_droit(FORWARD,  SEARCH_SPEED);
    set_moteur_gauche(BACKWARD, SEARCH_SPEED);
}


void avancer(uint32_t right_speed, uint32_t left_speed)
{
    set_moteur_droit(FORWARD, right_speed);
    set_moteur_gauche(FORWARD, left_speed);
}


void print_vect(uint8_t vect)
{
    int i;

    for (i = 6; i >= 0; i--)
    {
        if (vect & (1 << i))
            printf("1");
        else
            printf("0");
    }
}


uint8_t read_vect_capt(void)
{
    uint8_t ready_vect;
    uint8_t vect;

    ready_vect = IORD_8DIRECT(
        CAPTEURS_SOL_SEUIL_AVALON_0_BASE,
        REG_READY_VECT
    );

    vect = ready_vect & 0x7F;

    return vect;
}


int main(void)
{
    uint8_t vect_capt;

    int sum_position;
    int count;
    int error;
    int last_error;
    int derivative;
    int output;
    int last_known_error;

    int left_speed;
    int right_speed;

    last_error = 0;
    last_known_error = 0;

    printf("CUTECAR line follower adapted\n");

    IOWR_8DIRECT(
        CAPTEURS_SOL_SEUIL_AVALON_0_BASE,
        REG_NIVEAU,
        NIVEAU
    );

    delay_ms(100);

    while (1)
    {
        delay_ms(5);

        vect_capt = read_vect_capt();

        printf("vect = ");
        print_vect(vect_capt);
        printf("\n");

        /* Ligne perdue */
        if (vect_capt == 0x00)
        {
            last_error = 0;

            if (last_known_error > 0)
            {
                chercher_gauche();
            }
            else if (last_known_error < 0)
            {
                chercher_droite();
            }
            else
            {
                stop_moteurs();
            }

            continue;
        }

        /* Calcul position */
        sum_position = 0;
        count = 0;

        if (vect_capt & (1 << 0)) { sum_position += -3; count++; }
        if (vect_capt & (1 << 1)) { sum_position += -2; count++; }
        if (vect_capt & (1 << 2)) { sum_position += -1; count++; }
        if (vect_capt & (1 << 3)) { sum_position +=  0; count++; }
        if (vect_capt & (1 << 4)) { sum_position +=  1; count++; }
        if (vect_capt & (1 << 5)) { sum_position +=  2; count++; }
        if (vect_capt & (1 << 6)) { sum_position +=  3; count++; }

        error = sum_position / count;
        last_known_error = error;

        derivative = error - last_error;
        output = (KP * error) + (KD * derivative);
        last_error = error;

        /*
           Si le robot corrige dans le mauvais sens,
           inverse les deux lignes suivantes.
        */
        left_speed  = BASE_SPEED - output;
        right_speed = BASE_SPEED + output;

        if (left_speed > SPEED_MAX)
            left_speed = SPEED_MAX;

        if (right_speed > SPEED_MAX)
            right_speed = SPEED_MAX;

        if (left_speed < SPEED_MIN)
            left_speed = SPEED_MIN;

        if (right_speed < SPEED_MIN)
            right_speed = SPEED_MIN;

        avancer((uint32_t)right_speed, (uint32_t)left_speed);
    }

    return 0;
}