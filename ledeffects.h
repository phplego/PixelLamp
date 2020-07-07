#include <Arduino.h>
#include <FastLED.h>

#define MODE_AMOUNT 18
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

uint16_t gSpeed = 30; // speed is set dynamically once we've started up
uint16_t gScale = 40; // scale is set dynamically once we've started up

// This is the array that we keep our computed noise values in
#define MAX_DIMENSION WIDTH
uint8_t noise[HEIGHT][HEIGHT];

CRGBPalette16 currentPalette( PartyColors_p );
uint8_t colorLoop = 1;
uint8_t ihue = 0;





// mode names
const char * modeNames [MODE_AMOUNT] = {
    "sparkles",
    "fire",
    "rainbowVertical",
    "rainbowHorizontal",
    "colors",
    "madness",
    "cloud",
    "lava",
    "plasma",
    "rainbow",
    "rainbowStripe",
    "zebra",
    "forest",
    "ocean",
    "color",
    "snow",
    "matrix",
    "lighters"
};

// служебные функции

// залить все
void fillAll(CRGB color) {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = color;
    }
}

// получить номер пикселя в ленте по координатам
uint16_t getPixelNumber(int8_t x, int8_t y) {
    if ((THIS_Y % 2 == 0) || MATRIX_TYPE) {               // если чётная строка
        return (THIS_Y * _WIDTH + THIS_X);
    } else {                                              // если нечётная строка
        return (THIS_Y * _WIDTH + _WIDTH - THIS_X - 1);
    }
}


// функция отрисовки точки по координатам X Y
void drawPixelXY(int8_t x, int8_t y, CRGB color) {
    if (x < 0 || x > WIDTH - 1 || y < 0 || y > HEIGHT - 1) return;
    int thisPixel = getPixelNumber(x, y) * SEGMENTS;
    for (byte i = 0; i < SEGMENTS; i++) {
        leds[thisPixel + i] = color;
    }
}

// функция получения цвета пикселя по его номеру
uint32_t getPixColor(int thisSegm) {
    int thisPixel = thisSegm * SEGMENTS;
    if (thisPixel < 0 || thisPixel > NUM_LEDS - 1) return 0;
    return (((uint32_t)leds[thisPixel].r << 16) | ((long)leds[thisPixel].g << 8 ) | (long)leds[thisPixel].b);
}

// функция получения цвета пикселя в матрице по его координатам
uint32_t getPixColorXY(int8_t x, int8_t y) {
    return getPixColor(getPixelNumber(x, y));
}



// ================================= ЭФФЕКТЫ ====================================


void fadePixel(byte i, byte j, byte step) {     // новый фейдер
    int pixelNum = getPixelNumber(i, j);
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
    for (byte i = 0; i < WIDTH; i++) {
        for (byte j = 0; j < HEIGHT; j++) {
        fadePixel(i, j, step);
        }
    }
}

// --------------------------------- конфетти ------------------------------------
void sparklesRoutine() {
    for (byte i = 0; i < gScale; i++) {
        byte x = random(0, WIDTH);
        byte y = random(0, HEIGHT);
        if (getPixColorXY(x, y) == 0)
        leds[getPixelNumber(x, y)] = CHSV(random(0, 255), 255, 255);
    }
    fader(70);
}



// -------------------------------------- огонь ---------------------------------------------
// эффект "огонь"
#define SPARKLES 1        // вылетающие угольки вкл выкл
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

            leds[getPixelNumber(x, y)] = color;
        } else if (y == 8 && SPARKLES) {
            if (random(0, 20) == 0 && getPixColorXY(x, y - 1) != 0) drawPixelXY(x, y, getPixColorXY(x, y - 1));
            else drawPixelXY(x, y, 0);
        } else if (SPARKLES) {

            // старая версия для яркости
            if (getPixColorXY(x, y - 1) > 0)
            drawPixelXY(x, y, getPixColorXY(x, y - 1));
            else drawPixelXY(x, y, 0);

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
        leds[getPixelNumber(newX, 0)] = color;
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






byte hue;
// ---------------------------------------- радуга ------------------------------------------
void rainbowVertical() {
    hue += 2;
    for (byte j = 0; j < HEIGHT; j++) {
        CHSV thisColor = CHSV((byte)(hue + j * gScale), 255, 255);
        for (byte i = 0; i < WIDTH; i++)
        drawPixelXY(i, j, thisColor);
    }
}
void rainbowHorizontal() {
    hue += 2;
    for (byte i = 0; i < WIDTH; i++) {
        CHSV thisColor = CHSV((byte)(hue + i * gScale), 255, 255);
        for (byte j = 0; j < HEIGHT; j++)
        drawPixelXY(i, j, thisColor);   //leds[getPixelNumber(i, j)] = thisColor;
    }
}

// ---------------------------------------- ЦВЕТА ------------------------------------------
void colorsRoutine() {
    hue += gScale;
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CHSV(hue, 255, 255);
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
#define LIGHTERS_AM 21
int lightersPos[2][LIGHTERS_AM];
int8_t lightersSpeed[2][LIGHTERS_AM];
CHSV lightersColor[LIGHTERS_AM];
byte loopCounter;

int angle[LIGHTERS_AM];
int speedV[LIGHTERS_AM];
int8_t angleSpeed[LIGHTERS_AM];
bool lightersInited = false;

void lightersRoutine() {
    if (!lightersInited) {
        lightersInited = true;
        randomSeed(millis());
        for (byte i = 0; i < LIGHTERS_AM; i++) {
            lightersPos[0][i] = random(0, WIDTH * 10);
            lightersPos[1][i] = random(0, HEIGHT * 10);
            lightersSpeed[0][i] = random(-10, 10);
            lightersSpeed[1][i] = random(-10, 10);
            lightersColor[i] = CHSV(100, 0, 255);
        }
    }
    FastLED.clear();
    if (++loopCounter > 20) loopCounter = 0;
    for (byte i = 0; i < gScale; i++) {
        if (loopCounter == 0) {     // меняем скорость каждые 255 отрисовок
            lightersSpeed[0][i] += random(-3, 4);
            lightersSpeed[1][i] += random(-3, 4);
            lightersSpeed[0][i] = constrain(lightersSpeed[0][i], -10, 10);
            lightersSpeed[1][i] = constrain(lightersSpeed[1][i], -10, 10);
        }

        lightersPos[0][i] += lightersSpeed[0][i];
        lightersPos[1][i] += lightersSpeed[1][i];

        if (lightersPos[0][i] < 0) lightersPos[0][i] = (WIDTH - 1) * 10;
        if (lightersPos[0][i] >= WIDTH * 10) lightersPos[0][i] = 0;

        if (lightersPos[1][i] < 0) {
            lightersPos[1][i] = 0;
            lightersSpeed[1][i] = -lightersSpeed[1][i];
        }
        if (lightersPos[1][i] >= (HEIGHT - 1) * 10) {
            lightersPos[1][i] = (HEIGHT - 1) * 10;
            lightersSpeed[1][i] = -lightersSpeed[1][i];
        }
        drawPixelXY(lightersPos[0][i] / 10, lightersPos[1][i] / 10, lightersColor[i]);
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
                uint8_t olddata = noise[i][j];
                uint8_t newdata = scale8( olddata, dataSmoothing) + scale8( data, 256 - dataSmoothing);
                data = newdata;
            }

            noise[i][j] = data;
        }
    }
    noiseZ += gSpeed;

    // apply slow drift to X and Y, just for visual variation.
    noiseX += gSpeed / 8;
    noiseY -= gSpeed / 16;

    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
        uint8_t index = noise[j][i];
        uint8_t bri =   noise[i][j];
        // if this palette is a 'loop', add a slowly-changing base value
        if ( colorLoop) {
            index += ihue;
        }
        // brighten up, as the color palette itself often contains the
        // light/dark dynamic range desired
        if ( bri > 127 ) {
            bri = 255;
        } else {
            bri = dim8_raw( bri * 2);
        }
        CRGB color = ColorFromPalette( currentPalette, index, bri);      
        drawPixelXY(i, j, color);   //leds[getPixelNumber(i, j)] = color;
        }
    }
    ihue += 1;
}

void fillnoise8() {
    for (int i = 0; i < MAX_DIMENSION; i++) {
        int ioffset = gScale * i;
        for (int j = 0; j < MAX_DIMENSION; j++) {
            int joffset = gScale * j;
            noise[i][j] = inoise8(noiseX + ioffset, noiseY + joffset, noiseZ);
        }
    }
    noiseZ += gSpeed;
}

void madnessNoise() {
    fillnoise8();
    for (int i = 0; i < WIDTH; i++) {
        for (int j = 0; j < HEIGHT; j++) {
            CRGB thisColor = CHSV(noise[j][i], 255, noise[i][j]);
            drawPixelXY(i, j, thisColor);   
        }
    }
    ihue += 1;
}

void rainbowNoise() {
    currentPalette = RainbowColors_p;
    colorLoop = 1;
    fillNoiseLED();
}

void rainbowStripeNoise() {
    currentPalette = RainbowStripeColors_p;
    colorLoop = 1;
    fillNoiseLED();
}

void zebraNoise() {
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    
    colorLoop = 1;
    fillNoiseLED();
}

void forestNoise() {
    currentPalette = ForestColors_p;
    colorLoop = 0;
    fillNoiseLED();
}

void oceanNoise() {
    currentPalette = OceanColors_p;
    colorLoop = 0;
    fillNoiseLED();   
}

void plasmaNoise() {
    currentPalette = PartyColors_p;
    colorLoop = 1;
    fillNoiseLED();
}

void cloudNoise() {
    currentPalette = CloudColors_p;
    colorLoop = 0;
    fillNoiseLED();
}

void lavaNoise() {
    currentPalette = LavaColors_p;
    colorLoop = 0;
    fillNoiseLED();
}


//========================================================


unsigned long effTimer = 0;
int currentMode = 0;

void effectsTick() 
{

    if (millis() - effTimer >= 30 ) {
        effTimer = millis();
        switch (currentMode) {
            case 0: sparklesRoutine();
                break;
            case 1: fireRoutine();
                break;
            case 2: rainbowVertical();
                break;
            case 3: rainbowHorizontal();
                break;
            case 4: colorsRoutine();
                break;
            case 5: madnessNoise();
                break;
            case 6: cloudNoise();
                break;
            case 7: lavaNoise();
                break;
            case 8: plasmaNoise();
                break;
            case 9: rainbowNoise();
                break;
            case 10: rainbowStripeNoise();
                break;
            case 11: zebraNoise();
                break;
            case 12: forestNoise();
                break;
            case 13: oceanNoise();
                break;
            case 14: colorRoutine();
                break;
            case 15: snowRoutine();
                break;
            case 16: matrixRoutine();
                break;
            case 17: lightersRoutine();
                break;
        }
        FastLED.show();
    }
}