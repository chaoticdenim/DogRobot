#include<p18f2520.h> 
#include"MI2C.h"

int compteur_adc = 500;
int compteur_IR = 2;
int compteur_chaine = 10;

int flag_chaine = 0;
int flag_batterie = 0;
int flag_capteur = 0;
int flag_telecommande = 0;
int flag_roule = 0;
int flag_marche = 0;

int capteur_gauche = 0;
int capteur_droit = 0;


void HighISR(void);
volatile unsigned int UBAT = 1023;

#pragma code HighVector=0x08
void IntHighVector(void)
{
    _asm goto HighISR _endasm
}
#pragma code

#pragma interrupt HighISR 
void HighISR(void)
{
    if(PIR1bits.TMR2IF)
    {
        PIR1bits.TMR2IF = 0;
        if(compteur_chaine==0)
        {
            flag_chaine = 1;
        }
        else
        {
            compteur_chaine--;
        }
        if(compteur_adc==0)
        {
            flag_batterie=1;
        }
        else
        {
            compteur_adc--;
        }
        if(compteur_IR)
        {
            flag_capteur=1;
        }
        else
        {
            compteur_IR--;
        }
    }
    
    if(INTCONbits.INT0IF)
        {
            INTCONbits.INT0IF = 0;
            flag_telecommande = 1;
        }
}

void initialisation(void)
{
    //Horloge à 8MHz
    OSCCONbits.IRCF = 7;
    
    //PWM
    TRISCbits.RC1 = 0;
    TRISCbits.RC2 = 0;
    TRISAbits.RA6 = 0; //DIRD
    TRISAbits.RA7 = 0;  //DIRG
    
    
    //Timer0 en mode compteur 
    T0CONbits.TMR0ON = 1;
    T0CONbits.T08BIT = 0; //16bits
    T0CONbits.T0CS = 1; //Mode compteur
    T0CONbits.T0SE = 0; //Low to high counteur
    T0CONbits.PSA = 1;
    TMR0H = 0;
    TMR0L = 0;
    
    //TMR1 en mode compteur
    T1CONbits.RD16 = 1; //16bits
    T1CONbits.T1RUN = 0; //Source exterieure
    T1CONbits.T1CKPS = 0;
    T1CONbits.TMR1CS = 1;
    T1CONbits.TMR1ON = 1;
    TMR1H = 0;
    TMR1L = 0;
    
    //TMR2 PWM
    T2CONbits.T2OUTPS = 9; //POST 10
    T2CONbits.TMR2ON = 1; 
    T2CONbits.T2CKPS1 = 1; //PRE 16
    
    PR2 = 124;
    CCPR1L = 0;
    CCPR2L = 0;
    CCP1CONbits.DC1B = 0;
    CCP2CONbits.DC2B = 0;
    CCP1CONbits.CCP1M3 = 1;
    CCP1CONbits.CCP1M2 = 1;
    CCP2CONbits.CCP2M3 = 1;
    CCP2CONbits.CCP2M2 = 1;
    
    //CAN
    TRISBbits.RB5 = 0; //LED
    ADCON0bits.CHS0 = 0; //Choix entrée AN2 analogique
	ADCON0bits.CHS1 = 1;
	ADCON0bits.CHS2 = 0;
	ADCON0bits.CHS3 = 0;
    ADCON0bits.ADON = 1;
    
    ADCON1bits.VCFG0 = 0; //choix des références de tension
	ADCON1bits.VCFG1 = 0;
    ADCON1bits.PCFG0 = 0; //Choix des entrées analogiques 
	ADCON1bits.PCFG1 = 0;
	ADCON1bits.PCFG2 = 1;
	ADCON1bits.PCFG3 = 1;
    
    ADCON2bits.ADFM = 0; //justification à gauche 
    ADCON2bits.ADCS = 1;
    ADCON2bits.ACQT = 3;
    
    PIE1bits.ADIE = 1;
    
    //Initialisation IR
    TRISAbits.RA0 = 1;
    TRISAbits.RA1 = 1;
    TRISBbits.RB1 = 0;
    PORTBbits.RB1 = 0;
    
    //Initialisation interruption0
    INTCONbits.PEIE = 1;
    INTCONbits.INT0IE = 1;
    INTCON2bits.INTEDG0 = 1;
    PIE1bits.TXIE = 1;
    PIE1bits.ADIE = 1;
    PIE1bits.TMR2IE = 1;
    
    //Initialisaiton I2C
    MI2CInit();
    TRISCbits.RC3 = 1;
    TRISCbits.RC4 = 1;
    
    //Init capteur chaine
    TRISAbits.RA4 = 1;
    TRISCbits.RC0 = 1;
    
    //Init UART
    BAUDCONbits.BRG16 = 0;
    TXSTAbits.BRGH = 0;
	TXSTAbits.SYNC = 0;
    TXSTAbits.TXEN = 1;
    RCSTAbits.SPEN = 1;
    SPBRG = 12;
    TRISCbits.RC6 = 1;
    
    //Validation interruptions
    INTCONbits.GIE = 1;
}

void capteur_IR(void)
{
    selectionner_voie(0);
    ADCON0bits.GO = 1;
    while(ADCON0bits.NOT_DONE);
    capteur_droit = ADRESH;
    
    selectionner_voie(1);
    ADCON0bits.GO = 1;
    while(ADCON0bits.NOT_DONE);
    capteur_gauche = ADRESH;
    
    selectionner_voie(2);
}

void selectionner_voie(int voie)
{
    if(voie==0)
    {
        ADCON0bits.CHS = 0;
    }
    if(voie==1)
    {
        ADCON0bits.CHS = 1;
    }
    if(voie==2)
    {
        ADCON0bits.CHS = 2;
    }
}

void test_batterie(void)
{
    ADCON0bits.GO = 1;
    while(ADCON0bits.NOT_DONE);
    UBAT = ADRESH;
    if(UBAT<260)
    {
        arret();
        PORTBbits.RB5 = 1;
    }
    else
    {
        PORTBbits.RB5 = 0;
    }
}

int main(void)
{
    initialisation();
    while(1)
    {
        if(flag_batterie==1)
        {
            test_batterie();
            flag_batterie=0;
            compteur_adc = 500;
        }
        if(flag_telecommande==1)
        {
            marche_arret();
            flag_telecommande = 0;
        }
        if(flag_chaine)
        {
            compteur_chaine=10;
            flag_chaine=0;
            if(flag_roule)
            {
                asservissement_chaine();
            }
        }
        if(flag_capteur)
        {
            flag_capteur=0;
            compteur_IR=2;
            if(flag_marche)
            {
                capteur_IR();
                if(capteur_droit>0x45 || capteur_gauche>0x45)
                {
                    arret();
                }
                else if(capteur_droit>0x15 || capteur_gauche>0x15)
                {
                    if(flag_roule==0)
                    {
                        avance();
                    }
                }
                else
                {
                    arret();
                }
            }
        }
    }
}

