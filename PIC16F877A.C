#define _XTAL_FREQ 20000000
#define TEMP_LIMIT 30          /* alarm limit in deg C */
#include <htc.h>

__CONFIG(FOSC_HS & WDTE_OFF & PWRTE_ON & BOREN_OFF & LVP_OFF & CPD_OFF & WRT_OFF & CP_OFF);

/* ---------- outputs ---------- */
#define LED_ON()   (PORTC |=  0x04)    /* RC2 - pin 17 */
#define LED_OFF()  (PORTC &= ~0x04)
#define BUZ_ON()   (PORTA |=  0x04)    /* RA2 - pin 4  */
#define BUZ_OFF()  (PORTA &= ~0x04)
#define FAN_ON()   (PORTD |=  0x08)    /* RD3 - pin 22 */
#define FAN_OFF()  (PORTD &= ~0x08)

/* ---------- LCD control pins ---------- */
#define RS_HIGH()  (PORTB |=  0x01)    /* RB0 */
#define RS_LOW()   (PORTB &= ~0x01)
#define EN_HIGH()  (PORTB |=  0x02)    /* RB1 */
#define EN_LOW()   (PORTB &= ~0x02)

void delay_ms(unsigned int ms)
{
    while (ms--) __delay_ms(1);
}

/* ---------------- LCD ---------------- */
void LCD_Nibble(unsigned char n)
{
    PORTB = (PORTB & 0x0F) | (n & 0xF0);
    EN_HIGH();
    __delay_us(2);
    EN_LOW();
    __delay_us(50);
}

void LCD_Send(unsigned char val, unsigned char rs)
{
    if (rs) RS_HIGH(); else RS_LOW();
    LCD_Nibble(val & 0xF0);
    LCD_Nibble((val << 4) & 0xF0);
}

void LCD_Cmd(unsigned char c)
{
    LCD_Send(c, 0);
    if (c < 4) delay_ms(2);
}

void LCD_Char(char c) { LCD_Send((unsigned char)c, 1); }

void LCD_String(const char *s)
{
    while (*s) LCD_Char(*s++);
}

void LCD_SetCursor(unsigned char row, unsigned char col)
{
    LCD_Cmd((row == 1 ? 0x80 : 0xC0) + col);
}

void LCD_Init(void)
{
    delay_ms(20);
    RS_LOW();
    LCD_Nibble(0x30); delay_ms(5);
    LCD_Nibble(0x30); delay_ms(1);
    LCD_Nibble(0x30);
    LCD_Nibble(0x20);
    LCD_Cmd(0x28);
    LCD_Cmd(0x0C);
    LCD_Cmd(0x06);
    LCD_Cmd(0x01);
}

void LCD_Num3(unsigned int n)
{
    LCD_Char(n >= 100 ? '0' + (n / 100) : ' ');
    LCD_Char(n >= 10  ? '0' + ((n / 10) % 10) : ' ');
    LCD_Char('0' + (n % 10));
}

/* ---------------- ADC ---------------- */
void ADC_Init(void)
{
    ADCON1 = 0x8E;   /* only AN0 analog, RA1-RA5 digital */
    TRISA  = 0xFB;   /* RA0 input (LM35), RA2 output (buzzer) */
    ADCON0 = 0x81;
}

unsigned int ADC_Read(unsigned char ch)
{
    ADCON0 = 0x81 | (ch << 3);
    __delay_us(20);
    ADCON0 |= 0x04;
    while (ADCON0 & 0x04);
    return ((unsigned int)ADRESH << 8) | ADRESL;
}

/* ---------------- MAIN ---------------- */
void main(void)
{
    unsigned int adc, temp10;

    TRISB = 0x00;  PORTB = 0x00;   /* LCD */
    TRISC = 0x00;  PORTC = 0x00;   /* LED */
    TRISD = 0x00;  PORTD = 0x00;   /* fan */

    ADC_Init();
    PORTA = 0x00;                  /* buzzer off */
    LCD_Init();

    LCD_SetCursor(1, 0); LCD_String(" DIGITAL TEMP   ");
    LCD_SetCursor(2, 0); LCD_String(" MONITOR SYSTEM ");
    delay_ms(1500);
    LCD_Cmd(0x01);

    while (1)
    {
        adc    = ADC_Read(0);
        temp10 = (unsigned int)(((unsigned long)adc * 5000UL) / 1023UL);

        LCD_SetCursor(1, 0);
        LCD_String("Temp: ");
        LCD_Num3(temp10 / 10);
        LCD_Char('.');
        LCD_Char('0' + (temp10 % 10));
        LCD_Char((char)0xDF);
        LCD_String("C   ");

        LCD_SetCursor(2, 0);
        if (temp10 > (TEMP_LIMIT * 10))    /* above 30 C */
        {
            LED_ON();
            BUZ_ON();
            FAN_ON();
            LCD_String("FAN ON   ALERT! ");
        }
        else                               /* 30 C or below */
        {
            LED_ON();
            BUZ_OFF();
            FAN_OFF();
            LCD_String("FAN OFF  NORMAL ");
        }

        delay_ms(300);
    }
}
