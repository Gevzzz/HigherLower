#define __DELAY_BACKWARD_COMPATIBLE__

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <led.h>
#include <buttons.h>
#include <display.h>
#include <potentio.h>
#include <string.h>
#include <buzzer.h>
#include <usart.h>
#include <stdlib.h>

#define levelDisplay 0
#define movingDisplay 1
#define createdDisplay 2
#define targetDisplay 3
//alle noten
#define C5 523.250
#define D5 587.330
#define E5 659.250
#define F5 698.460
#define G5 783.990
#define A5 880.000
#define B5 987.770
#define C6 1046.500

#define starterLevens 4

#define snelheid 500

//verplicht struct gebruiken
typedef struct
{
    int levensKwijt;
    int level;
    int doelGetal;
    int amountOfSegments;
    int time;
} LEVELLOG;

int levelEinde = 1;
int wrong = 0;
int segmentsCreated;

double levelTijd;
uint8_t beginGame = 0;
int level = 0;

double levelTickRate = -1;
int currentTick = 0;

int currentSeg;
int createdSeg[7];
int lives = starterLevens;
int target = -1;
int levensKwijt;

int segmentArray[10][7] = {
    // a  b  c  d  e  f  g
    {1, 1, 1, 1, 1, 1, 0}, 
    {0, 1, 1, 0, 0, 0, 0}, 
    {1, 1, 0, 1, 1, 0, 1}, 
    {1, 1, 1, 1, 0, 0, 1}, 
    {0, 1, 1, 0, 0, 1, 1}, 
    {1, 0, 1, 1, 0, 1, 1},
    {1, 0, 1, 1, 1, 1, 1}, 
    {1, 1, 1, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1}, 
    {1, 1, 1, 1, 0, 1, 1}  
void snelheidLevelsAanmaken()
{
    levelTickRate = (snelheid - ((snelheid * 0.1) * level)) / 16.384;
}

//methodes segmenten
void segmentVerwijderen()
{
    //for lusje 7 keer     createdSeg[i] = 0;

    for (int i = 0; i < 7; i++)
    {
        createdSeg[i] = 0;
    }
}
void aanmakenNieuwesegment()
{
    currentSeg = rand() % 7;
    segmentsCreated++;
    writeLineToSegment(movingDisplay, currentSeg);
}
int levelKlaar()
{
    for (int i = 0; i < 7; i++)
    {
        if (segmentArray[target][i] != createdSeg[i])
        {
            return 0;
        }
    }
    return 1;
}

int aanmakenSegment()
{

    if (createdSeg[currentSeg] == 1 || !segmentArray[target][currentSeg])
    {
        return 1;
    }
    createdSeg[currentSeg] = 1;
    //geluidjeafspelen
    playTone(C6, 150);
    return 0;
}
//geluid afspelen
//canvas code bekijken
ISR(PCINT1_vect)
{
    if (!beginGame && buttonPushed(1))
    {
        beginGame = 1;
    }
    if (buttonPushed(3))
    {
        _delay_ms(20);
        if (buttonPushed(3))
        {
            wrong = aanmakenSegment();
        }
    }
}

ISR(TIMER0_OVF_vect)
{
    if (levelEinde != 1)
    {
        levelTijd += 16.384;
        if (levelTickRate != -1)
        {
            currentTick++;
        }
        if (currentTick >= levelTickRate && levelTickRate != -1)
        {
            aanmakenNieuwesegment();
            currentTick = 0;
        }
    }
}

//zoek dit op canvas op
void initTimer0()
{
    // STAP 1: kies de WAVE FORM en dus de Mode of Operation
    TCCR0A |= _BV(WGM00) | _BV(WGM01); // WGM00 = 1 en WGM01 = 1 --> Fast PWM Mode: TCNT0 telt steeds tot 255
    // STAP 2: stel *altijd* een PRESCALER in. De snelheid van tellen wordt bepaald door de CPU-klok / PRESCALER
    TCCR0B |= _BV(CS02) | _BV(CS00); // CS02 = 1 en CS00 = 1 --> prescalerfactor is nu 1024 (=elke 64Âµs)
    // STAP 3: enable INTERRUPTs voor twee gevallen: TCNT0 == TOP en TCNT0 == OCR0A
    TIMSK0 |= _BV(TOIE0); // overflow interrupt enablen
}

void createTarget()
{
    target = rand() % 10;
    if (target == 4)
    {
        uint32_t frequencies[] = {C5, E5};
        for (int i = 0; i < 2; i++)
        {
            playTone(frequencies[i], 75);
        }
    }
}

void levensUpdaten(int lives)
{
    lightDownOneLed(starterLevens - lives);
    levensKwijt++;

    //hier maak je een fataal geluid
    playTone(G5, 150);
    wrong = 0;
}

void displayCreated()
{
    for (int i = 0; i < 7; i++)
    {
        if (createdSeg[i])
        {
            writeLineToSegment(createdDisplay, i);
        }
    }
}

//waarom werkt dit niet, laat voor later
//logs aanmkane
void saveToLog(LEVELLOG *logs)
{
    logs[level] = *(LEVELLOG *)calloc(1, sizeof(LEVELLOG));

    logs[level].level = level;

    logs[level].doelGetal = target;

    logs[level].amountOfSegments = segmentsCreated;

    logs[level].levensKwijt = levensKwijt;

    logs[level].time = levelTijd;
}

void printLog(LEVELLOG *logs, int length)
{
    for (int i = 0; i < 10; i++)
    {
        printf("Level %d: %d |   %d |  %d |  %d.%d\"\n",
               logs[i].level, logs[i].doelGetal, logs[i].amountOfSegments, logs[i].levensKwijt, logs[i].time / 1000, logs[i].time % 1000);
			   
        free((void *)logs[i].level);
        free((void *)logs[i].doelGetal);
        free((void *)logs[i].amountOfSegments);
        free((void *)logs[i].levensKwijt);
        free(&logs[i].time);
    }
}

int main()
{
    initADC();
    initDisplay();
    initUSART();
	
    enableBuzzer();
    enableAllButtons();
	
    enableAllLeds();
    lightDownAllLeds();
	
	
    clearDisplay();
	

    //interupt doen
    PCICR |= _BV(PCIE1);
    PCMSK1 |= _BV(PC1) | _BV(PC2) | _BV(PC3);
    initTimer0();
    //initCreatedSeg();

    sei(); // interrupts globaal enablen

    printString("Welkom bij CijferVormer, druk op de linkse knop om te beginnen\n");
    printString("Draai aan de potentiometer voor je het spel begint voor een random seed aan te maken!\n");

    int seed;
    while (!beginGame)
    {
        //methode van library
        scrollingString("CijferVormer", 500);
        seed = readPotentioMeter();
    }
    srand(seed);

    //light up all leds
    lightUpAllLeds();
    //liefst calloc
    LEVELLOG *logs = malloc(10 * sizeof(LEVELLOG));
    //hier komt je spel
    while (level < 10 && lives > 0)
    {
        if (target == -1)
        {
            segmentVerwijderen();
            snelheidLevelsAanmaken();
            levelTijd = 0;
            levelEinde = 0;
            levensKwijt = 0;
            segmentsCreated = 0;
            createTarget();
        }
        writeNumberToSegment(levelDisplay, level);
        writeNumberToSegment(targetDisplay, target);
        writeLineToSegment(movingDisplay, currentSeg);
        displayCreated();

        if (wrong)
        {
            levensUpdaten(--lives);
        }
        if (levelKlaar())
        {
            saveToLog(&log);
            levelEinde = 1;
            level++;
            target = -1;
            segmentVerwijderen();
        }
    }
    //waarom warning hier
    levelEinde = 1;
    saveToLog(&logs);
    clearDisplay();

    printLog(&logs, level + 1);
    return 0;
}