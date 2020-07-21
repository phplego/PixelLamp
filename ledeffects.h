#include <Arduino.h>
#include <FastLED.h>

#define MODE_COUNT 18          // 0..17
#define MATRIX_TYPE 0         // тип матрицы: 0 - зигзаг, 1 - параллельная

//bool loadingFlag = false;
const int WIDTH = 16;
const int HEIGHT = 16;
const int NUM_LEDS = WIDTH * HEIGHT;
const int SEGMENTS = 1;
unsigned char fireMatrixValue[8][16];


// **************** НАСТРОЙКА МАТРИЦЫ ****************
#define _WIDTH WIDTH
#define THIS_X x
#define THIS_Y y
// **************** **************** ****************


// The 16 bit version of our coordinates
static uint16_t noiseX;
static uint16_t noiseY;
static uint16_t noiseZ;

byte gSpeed = 30; // speed is set dynamically once we've started up
byte gScale = 40; // scale is set dynamically once we've started up

// This is the array that we keep our computed noise values in
#define MAX_DIMENSION WIDTH
uint8_t gNoiseArr[HEIGHT][HEIGHT];

CRGBPalette16 gNoiseCurrentPalette( PartyColors_p );
bool gNoiseColorLoop = true;
uint8_t gNoiseHue = 0;


// array of mode configs
struct {
    byte speed = 0;
    byte scale = 0;
} gModeConfigs [MODE_COUNT];

// type for routine function
typedef void (*RoutineFunction) ();



// служебные функции

// залить все
void fillAll(CRGB color) {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = color;
    }
}

// получить номер пикселя в ленте по координатам
uint16_t getPixelIndex(int8_t x, int8_t y) {
    if ((THIS_Y % 2 == 0) || MATRIX_TYPE) {               // если чётная строка
        return (THIS_Y * _WIDTH + THIS_X);
    } else {                                              // если нечётная строка
        return (THIS_Y * _WIDTH + _WIDTH - THIS_X - 1);
    }
}


// функция отрисовки точки по координатам X Y
void drawPixelXY(int8_t x, int8_t y, CRGB color) {
    if (x < 0 || x > WIDTH - 1 || y < 0 || y > HEIGHT - 1) return;
    int index = getPixelIndex(x, y) * SEGMENTS;
    leds[index] = color;
}

// функция получения цвета пикселя по его номеру
uint32_t getPixColor(int pixelIndex) {
    if (pixelIndex < 0 || pixelIndex > NUM_LEDS - 1) return 0;
    return (((uint32_t)leds[pixelIndex].r << 16) | ((long)leds[pixelIndex].g << 8 ) | (long)leds[pixelIndex].b);
}

// функция получения цвета пикселя в матрице по его координатам
uint32_t getPixColorXY(int8_t x, int8_t y) {
    return getPixColor(getPixelIndex(x, y));
}



// ================================= Effects ====================================


// fade one pixel
void fadePixel(int pixelNum, byte step) {     
    if (getPixColor(pixelNum) == 0) return;

    if (leds[pixelNum].r >= 30 ||
        leds[pixelNum].g >= 30 ||
        leds[pixelNum].b >= 30) {
        leds[pixelNum].fadeToBlackBy(step);
    } else {
        leds[pixelNum] = 0;
    }
}


// функция плавного угасания цвета для всех пикселей
void fader(byte step) {
    for(int i = 0; i < NUM_LEDS; i++)
        fadePixel(i, step);
}

// --------------------------------- конфетти ------------------------------------
void sparklesRoutine() {
    for (byte i = 0; i < gScale; i++) {
        byte x = random(0, WIDTH);
        byte y = random(0, HEIGHT);
        if (getPixColorXY(x, y) == 0)
        leds[getPixelIndex(x, y)] = CHSV(random(0, 255), 255, 255);
    }
    fader(70);
}



// -------------------------------------- огонь ---------------------------------------------
// эффект "огонь"
#define FIRE_SPARKLES 1        // вылетающие угольки вкл выкл
unsigned char fireLite[WIDTH];
int firePercent = 0;

//these values are substracetd from the generated values to give a shape to the animation
const unsigned char fireValueMask[8][16] PROGMEM = {
    {32 , 0  , 0  , 0  , 0  , 0  , 0  , 32 , 32 , 0  , 0  , 0  , 0  , 0  , 0  , 32 },
    {64 , 0  , 0  , 0  , 0  , 0  , 0  , 64 , 64 , 0  , 0  , 0  , 0  , 0  , 0  , 64 },
    {96 , 32 , 0  , 0  , 0  , 0  , 32 , 96 , 96 , 32 , 0  , 0  , 0  , 0  , 32 , 96 },
    {128, 64 , 32 , 0  , 0  , 32 , 64 , 128, 128, 64 , 32 , 0  , 0  , 32 , 64 , 128},
    {160, 96 , 64 , 32 , 32 , 64 , 96 , 160, 160, 96 , 64 , 32 , 32 , 64 , 96 , 160},
    {192, 128, 96 , 64 , 64 , 96 , 128, 192, 192, 128, 96 , 64 , 64 , 96 , 128, 192},
    {255, 160, 128, 96 , 96 , 128, 160, 255, 255, 160, 128, 96 , 96 , 128, 160, 255},
    {255, 192, 160, 128, 128, 160, 192, 255, 255, 192, 160, 128, 128, 160, 192, 255}
};

//these are the hues for the fire,
//should be between 0 (red) to about 25 (yellow)
const unsigned char fireHueMask[8][16] PROGMEM = {
    {1 , 11, 19, 25, 25, 22, 11, 1 , 1 , 11, 19, 25, 25, 22, 11, 1 },
    {1 , 8 , 13, 19, 25, 19, 8 , 1 , 1 , 8 , 13, 19, 25, 19, 8 , 1 },
    {1 , 8 , 13, 16, 19, 16, 8 , 1 , 1 , 8 , 13, 16, 19, 16, 8 , 1 },
    {1 , 5 , 11, 13, 13, 13, 5 , 1 , 1 , 5 , 11, 13, 13, 13, 5 , 1 },
    {1 , 5 , 11, 11, 11, 11, 5 , 1 , 1 , 5 , 11, 11, 11, 11, 5 , 1 },
    {0 , 1 , 5 , 8 , 8 , 5 , 1 , 0 , 0 , 1 , 5 , 8 , 8 , 5 , 1 , 0 },
    {0 , 0 , 1 , 5 , 5 , 1 , 0 , 0 , 0 , 0 , 1 , 5 , 5 , 1 , 0 , 0 },
    {0 , 0 , 0 , 1 , 1 , 0 , 0 , 0 , 0 , 0 , 0 , 1 , 1 , 0 , 0 , 0 }
};

void fireShiftUp() {
    for (uint8_t y = HEIGHT - 1; y > 0; y--) {
        for (uint8_t x = 0; x < WIDTH; x++) {
            uint8_t newX = x;
            if (x > 15) newX = x - 15;
            if (y > 7) continue;
            fireMatrixValue[y][newX] = fireMatrixValue[y - 1][newX];
        }
    }

    for (uint8_t x = 0; x < WIDTH; x++) {
        uint8_t newX = x;
        if (x > 15) newX = x - 15;
        fireMatrixValue[0][newX] = fireLite[newX];
    }
}


// draw a frame, interpolating between 2 "key frames"
// @param pcnt percentage of interpolation
void fireDrawFrame(int pcnt) {
    int nextv;

    //each row interpolates with the one before it
    for (unsigned char y = HEIGHT - 1; y > 0; y--) {
        for (unsigned char x = 0; x < WIDTH; x++) {
            uint8_t newX = x;
            if (x > 15) newX = x - 15;
            if (y < 8) {
                nextv =
                (((100.0 - pcnt) * fireMatrixValue[y][newX]
                    + pcnt * fireMatrixValue[y - 1][newX]) / 100.0)
                - pgm_read_byte(&(fireValueMask[y][newX]));

                CRGB color = CHSV(
                    gScale * 2.5 + pgm_read_byte(&(fireHueMask[y][newX])), // H
                    255, // S
                    (uint8_t)max(0, nextv) // V
                );

                leds[getPixelIndex(x, y)] = color;
            } else if (y == 8 && FIRE_SPARKLES) {
                if (random(0, 20) == 0 && getPixColorXY(x, y - 1) != 0) 
                    drawPixelXY(x, y, getPixColorXY(x, y - 1));
                else 
                    drawPixelXY(x, y, 0);
            } else if (FIRE_SPARKLES) {

                // старая версия для яркости
                if (getPixColorXY(x, y - 1) > 0)
                    drawPixelXY(x, y, getPixColorXY(x, y - 1));
                else 
                    drawPixelXY(x, y, 0);
            }
        }
    }

    //first row interpolates with the "next" line
    for (unsigned char x = 0; x < WIDTH; x++) {
        uint8_t newX = x;
        if (x > 15) newX = x - 15;
        CRGB color = CHSV(
            gScale * 2.5 + pgm_read_byte(&(fireHueMask[0][newX])), // H
            255,           // S
            (uint8_t)(((100.0 - pcnt) * fireMatrixValue[0][newX] + pcnt * fireLite[newX]) / 100.0) // V
        );
        leds[getPixelIndex(newX, 0)] = color;
    }
}


void fireRoutine() {
    if (firePercent >= 100) {
        fireShiftUp();
        
        // Randomly generate the next line (matrix row)
        for (uint8_t x = 0; x < WIDTH; x++) {
            fireLite[x] = random(64, 255);
        }
        firePercent = 0;
    }
    fireDrawFrame(firePercent);
    firePercent += 30;
}






byte gHue;
// ---------------------------------------- радуга ------------------------------------------
void rainbowVertical() {
    gHue += 2;
    for (byte y = 0; y < HEIGHT; y++) {
        CHSV thisColor = CHSV((byte)(gHue + y * gScale), 255, 255);
        for (byte x = 0; x < WIDTH; x++){
            drawPixelXY(x, y, thisColor);
        }
    }
}
void rainbowHorizontal() {
    gHue += 2;
    for (byte x = 0; x < WIDTH; x++) {
        CHSV thisColor = CHSV((byte)(gHue + x * gScale), 255, 255);
        for (byte y = 0; y < HEIGHT; y++){
            drawPixelXY(x, y, thisColor); 
        }
    }
}

// ---------------------------------------- ЦВЕТА ------------------------------------------
void colorsRoutine() {
    gHue += gScale;
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CHSV(gHue, 255, 255);
    }
}

// --------------------------------- ЦВЕТ ------------------------------------
void colorRoutine() {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CHSV(gScale * 2.5, 255, 255);
    }
}

// ------------------------------ снегопад 2.0 --------------------------------
void snowRoutine() {
    // сдвигаем всё вниз
    for (byte x = 0; x < WIDTH; x++) {
        for (byte y = 0; y < HEIGHT - 1; y++) {
            drawPixelXY(x, y, getPixColorXY(x, y + 1));
        }
    }

    for (byte x = 0; x < WIDTH; x++) {
        // заполняем случайно верхнюю строку
        // а также не даём двум блокам по вертикали вместе быть
        if (getPixColorXY(x, HEIGHT - 2) == 0 && (random(0, gScale) == 0))
            drawPixelXY(x, HEIGHT - 1, 0xE0FFFF - 0x101010 * random(0, 4));
        else
            drawPixelXY(x, HEIGHT - 1, 0x000000);
    }
}

// ------------------------------ МАТРИЦА ------------------------------
void matrixRoutine() {
    for (byte x = 0; x < WIDTH; x++) {
        // заполняем случайно верхнюю строку
        uint32_t thisColor = getPixColorXY(x, HEIGHT - 1);
        if((thisColor & 0x00FF00) != thisColor) // если попался "незеленый пиксель"
            thisColor = 0;

        if (thisColor == 0)
            drawPixelXY(x, HEIGHT - 1, 0x00FF00 * (random(0, gScale) == 0));
        else if (thisColor < 0x002000)
            drawPixelXY(x, HEIGHT - 1, 0);
        else
            drawPixelXY(x, HEIGHT - 1, thisColor - 0x002000);
    }

    // сдвигаем всё вниз
    for (byte x = 0; x < WIDTH; x++) {
        for (byte y = 0; y < HEIGHT - 1; y++) {
            drawPixelXY(x, y, getPixColorXY(x, y + 1));
        }
    }
}

// ----------------------------- СВЕТЛЯКИ ------------------------------
#define LIGHTERS_COUNT 20
struct { int x = 0; int y = 0; } gLightersPos [LIGHTERS_COUNT];
struct { int vx = 0; int vy = 0; } gLightersSpeed [LIGHTERS_COUNT];
CHSV gLightersColor[LIGHTERS_COUNT];
byte gLightersloopCounter;
bool gLightersInited = false;

void lightersRoutine() {
    if (!gLightersInited) {
        gLightersInited = true;
        randomSeed(millis());
        for (byte i = 0; i < LIGHTERS_COUNT; i++) {
            gLightersPos[i].x = random(0, WIDTH * 10);
            gLightersPos[i].y = random(0, HEIGHT * 10);
            gLightersSpeed[i].vx = random(-10, 10);
            gLightersSpeed[i].vy = random(-10, 10);
            gLightersColor[i] = CHSV(100, 0, 255);
        }
    }
    FastLED.clear();
    if (++gLightersloopCounter > 20) gLightersloopCounter = 0;
    for (byte i = 0; i < LIGHTERS_COUNT; i++) {
        if (gLightersloopCounter == 0) {     // меняем скорость каждые 255 отрисовок
            gLightersSpeed[i].vx += random(-3, 4);
            gLightersSpeed[i].vy += random(-3, 4);
            gLightersSpeed[i].vx = constrain(gLightersSpeed[i].vx, -5, 5);
            gLightersSpeed[i].vy = constrain(gLightersSpeed[i].vy, -5, 5);
        }

        gLightersPos[i].x += gLightersSpeed[i].vx;
        gLightersPos[i].y += gLightersSpeed[i].vy;

        if (gLightersPos[i].x < 0) gLightersPos[i].x = (WIDTH - 1) * 10;
        if (gLightersPos[i].x >= WIDTH * 10) gLightersPos[i].x = 0;

        if (gLightersPos[i].y < 0) {
            gLightersPos[i].y = 0;
            gLightersSpeed[i].vy = -gLightersSpeed[i].vy;
        }
        if (gLightersPos[i].y >= (HEIGHT - 1) * 10) {
            gLightersPos[i].y = (HEIGHT - 1) * 10;
            gLightersSpeed[i].vy = -gLightersSpeed[i].vy;
        }
        drawPixelXY(gLightersPos[i].x / 10, gLightersPos[i].y / 10, gLightersColor[i]);
    }
}

//====================================================================





// ******************* СЛУЖЕБНЫЕ *******************
void fillNoiseLED() {
    uint8_t dataSmoothing = 0;
    if ( gSpeed < 50) {
        dataSmoothing = 200 - (gSpeed * 4);
    }
    for (int i = 0; i < MAX_DIMENSION; i++) {
        int ioffset = gScale * i;
        for (int j = 0; j < MAX_DIMENSION; j++) {
            int joffset = gScale * j;

            uint8_t data = inoise8(noiseX + ioffset, noiseY + joffset, noiseZ);

            data = qsub8(data, 16);
            data = qadd8(data, scale8(data, 39));

            if ( dataSmoothing ) {
                uint8_t olddata = gNoiseArr[i][j];
                uint8_t newdata = scale8( olddata, dataSmoothing) + scale8( data, 256 - dataSmoothing);
                data = newdata;
            }

            gNoiseArr[i][j] = data;
        }
    }
    noiseZ += gSpeed;

    // apply slow drift to X and Y, just for visual variation.
    noiseX += gSpeed / 8;
    noiseY -= gSpeed / 16;

    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
        uint8_t index = gNoiseArr[j][i];
        uint8_t bri =   gNoiseArr[i][j];
        // if this palette is a 'loop', add a slowly-changing base value
        if ( gNoiseColorLoop) {
            index += gNoiseHue;
        }
        // brighten up, as the color palette itself often contains the
        // light/dark dynamic range desired
        if ( bri > 127 ) {
            bri = 255;
        } else {
            bri = dim8_raw( bri * 2);
        }
        CRGB color = ColorFromPalette( gNoiseCurrentPalette, index, bri);      
        drawPixelXY(i, j, color);   //leds[getPixelIndex(i, j)] = color;
        }
    }
    gNoiseHue += 1;
}

void fillnoise8() {
    for (int i = 0; i < MAX_DIMENSION; i++) {
        int ioffset = gScale * i;
        for (int j = 0; j < MAX_DIMENSION; j++) {
            int joffset = gScale * j;
            gNoiseArr[i][j] = inoise8(noiseX + ioffset, noiseY + joffset, noiseZ);
        }
    }
    noiseZ += gSpeed;
}

void madnessNoise() {
    fillnoise8();
    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
            CRGB thisColor = CHSV(gNoiseArr[j][i], 255, gNoiseArr[i][j]);
            drawPixelXY(i, j, thisColor);   
        }
    }
    gNoiseHue += 1;
}

void rainbowNoise() {
    gNoiseCurrentPalette = RainbowColors_p;
    gNoiseColorLoop = true;
    fillNoiseLED();
}

void rainbowStripeNoise() {
    gNoiseCurrentPalette = RainbowStripeColors_p;
    gNoiseColorLoop = true;
    fillNoiseLED();
}

void zebraNoise() {
    // 'black out' all 16 palette entries...
    fill_solid( gNoiseCurrentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    gNoiseCurrentPalette[0] = CRGB::White;
    gNoiseCurrentPalette[4] = CRGB::White;
    gNoiseCurrentPalette[8] = CRGB::White;
    gNoiseCurrentPalette[12] = CRGB::White;
    
    gNoiseColorLoop = true;
    fillNoiseLED();
}

void forestNoise() {
    gNoiseCurrentPalette = ForestColors_p;
    gNoiseColorLoop = false;
    fillNoiseLED();
}

void oceanNoise() {
    gNoiseCurrentPalette = OceanColors_p;
    gNoiseColorLoop = false;
    fillNoiseLED();   
}

void plasmaNoise() {
    gNoiseCurrentPalette = PartyColors_p;
    gNoiseColorLoop = true;
    fillNoiseLED();
}

void cloudNoise() {
    gNoiseCurrentPalette = CloudColors_p;
    gNoiseColorLoop = false;
    fillNoiseLED();
}

void lavaNoise() {
    gNoiseCurrentPalette = LavaColors_p;
    gNoiseColorLoop = false;
    fillNoiseLED();
}


//========================================================

struct {
    const char * name;
    RoutineFunction routineFunction;
} gModeStructs [MODE_COUNT] = {
    {"sparkles", sparklesRoutine},
    {"fire", fireRoutine},
    {"lighters", lightersRoutine},
    {"cloud", cloudNoise},
    {"lava", lavaNoise},
    {"plasma", plasmaNoise},
    {"rainbow", rainbowNoise},
    {"rainbowStripe", rainbowStripeNoise},
    {"zebra", zebraNoise},
    {"forest", forestNoise},
    {"ocean", oceanNoise},
    {"madness", madnessNoise},
    {"color", colorRoutine},
    {"snow", snowRoutine},
    {"matrix", matrixRoutine},
    {"rainbowVertical", rainbowVertical},
    {"rainbowHorizontal", rainbowHorizontal},
    {"colors", colorRoutine},
};


unsigned long gLastFrameTime = 0;
byte gCurrentMode = 0;

void effectsLoop() 
{
    const unsigned int FRAME_INTERVAL = 30;

    if (millis() - gLastFrameTime >= FRAME_INTERVAL ) {
        gLastFrameTime = millis();

        if(gCurrentMode >= MODE_COUNT)
            gCurrentMode = 0;

        gSpeed = gModeConfigs[gCurrentMode].speed;
        gScale = gModeConfigs[gCurrentMode].scale;

        gModeStructs[gCurrentMode].routineFunction();

        // switch (gCurrentMode) {
        //     case 0: sparklesRoutine();
        //         break;
        //     case 1: fireRoutine();
        //         break;
        //     case 2: rainbowVertical();
        //         break;
        //     case 3: rainbowHorizontal();
        //         break;
        //     case 4: colorsRoutine();
        //         break;
        //     case 5: madnessNoise();
        //         break;
        //     case 6: cloudNoise();
        //         break;
        //     case 7: lavaNoise();
        //         break;
        //     case 8: plasmaNoise();
        //         break;
        //     case 9: rainbowNoise();
        //         break;
        //     case 10: rainbowStripeNoise();
        //         break;
        //     case 11: zebraNoise();
        //         break;
        //     case 12: forestNoise();
        //         break;
        //     case 13: oceanNoise();
        //         break;
        //     case 14: colorRoutine();
        //         break;
        //     case 15: snowRoutine();
        //         break;
        //     case 16: matrixRoutine();
        //         break;
        //     case 17: lightersRoutine();
        //         break;
        // }
        FastLED.show();
    }
}