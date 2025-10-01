#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// Common pins
constexpr uint32_t HUB_R0 = 7;
constexpr uint32_t HUB_G0 = 15;
constexpr uint32_t HUB_B0 = 16;
constexpr uint32_t HUB_R1 = 17;
constexpr uint32_t HUB_G1 = 18;
constexpr uint32_t HUB_B1 = 8;

constexpr uint32_t HUB_A   = 41;
constexpr uint32_t HUB_B   = 40;
constexpr uint32_t HUB_C   = 39;
constexpr uint32_t HUB_D   = 38;
constexpr uint32_t HUB_E   = 42;

constexpr uint32_t HUB_LAT = 36;
constexpr uint32_t HUB_CLK = 37;

constexpr uint32_t HUB_OE0 = 35; // OE for first panel
constexpr uint32_t HUB_OE1 = 6;  // OE for second panel

constexpr uint32_t HUB_WIDTH  = 64;
constexpr uint32_t HUB_HEIGHT = 32;

MatrixPanel_I2S_DMA *panel0 = nullptr;
MatrixPanel_I2S_DMA *panel1 = nullptr;

// Define color variables
uint16_t myBLACK, myWHITE, myRED, myGREEN, myBLUE;

// Color wheel function
uint16_t colorWheel(MatrixPanel_I2S_DMA* disp, uint8_t pos) {
    if(pos < 85) return disp->color565(pos*3, 255-pos*3, 0);
    else if(pos < 170) {
        pos -= 85;
        return disp->color565(255-pos*3, 0, pos*3);
    } else {
        pos -= 170;
        return disp->color565(0, pos*3, 255-pos*3);
    }
}

// Draw text with color animation
void drawText(MatrixPanel_I2S_DMA* disp, int colorWheelOffset) {
    disp->setTextSize(1);
    disp->setTextWrap(false);
    disp->setCursor(5,0);

    const char *str = "ESP32 DMA";
    for(uint8_t w=0; w<strlen(str); w++) {
        disp->setTextColor(colorWheel(disp, w*32 + colorWheelOffset));
        disp->print(str[w]);
    }

    disp->println();
    disp->print(" ");
    for(uint8_t w = 9; w < 18; w++) {
        disp->setTextColor(colorWheel(disp, w*32 + colorWheelOffset));
        disp->print("*");
    }

    disp->println();
    disp->setTextColor(disp->color444(15,15,15));
    disp->println("LED MATRIX!");

    disp->setTextColor(disp->color444(0,8,15));
    disp->print('3');
    disp->setTextColor(disp->color444(15,4,0));
    disp->print('2');
    disp->setTextColor(disp->color444(15,15,0));
    disp->print('x');
    disp->setTextColor(disp->color444(8,15,0));
    disp->print('6');
    disp->setTextColor(disp->color444(8,0,15));
    disp->print('4');

    disp->setCursor(34,24);
    disp->setTextColor(disp->color444(0,15,15));
    disp->print("*");
    disp->setTextColor(disp->color444(15,0,0));
    disp->print('R');
    disp->setTextColor(disp->color444(0,15,0));
    disp->print('G');
    disp->setTextColor(disp->color444(0,0,15));
    disp->print("B");
    disp->setTextColor(disp->color444(15,0,8));
    disp->println("*");
}

void drawDemoShapes(MatrixPanel_I2S_DMA* disp) {
    disp->fillRect(0,0,disp->width(), disp->height(), disp->color444(0,15,0));
    delay(500);
    disp->drawRect(0,0,disp->width(), disp->height(), disp->color444(15,15,0));
    delay(500);
    disp->drawLine(0,0, disp->width()-1, disp->height()-1, disp->color444(15,0,0));
    disp->drawLine(disp->width()-1,0, 0, disp->height()-1, disp->color444(15,0,0));
    delay(500);
    disp->drawCircle(10,10,10, disp->color444(0,0,15));
    delay(500);
    disp->fillCircle(40,21,10, disp->color444(15,0,15));
    delay(500);
    disp->fillScreen(disp->color444(0,0,0));
}

void setup() {
    // Panel 0 configuration
    HUB75_I2S_CFG cfg0(HUB_WIDTH, HUB_HEIGHT, 1);
    cfg0.gpio.r1 = HUB_R1; cfg0.gpio.g1 = HUB_G1; cfg0.gpio.b1 = HUB_B1;
    cfg0.gpio.r2 = HUB_R0; cfg0.gpio.g2 = HUB_G0; cfg0.gpio.b2 = HUB_B0;
    cfg0.gpio.a = HUB_A; cfg0.gpio.b = HUB_B; cfg0.gpio.c = HUB_C; cfg0.gpio.d = HUB_D; cfg0.gpio.e = HUB_E;
    cfg0.gpio.lat = HUB_LAT; cfg0.gpio.oe = HUB_OE0; cfg0.gpio.clk = HUB_CLK;
    panel0 = new MatrixPanel_I2S_DMA(cfg0);
    panel0->begin();
    panel0->setBrightness8(90);
    panel0->clearScreen();

    // Panel 1 configuration
    HUB75_I2S_CFG cfg1(HUB_WIDTH, HUB_HEIGHT, 1);
    cfg1.gpio.r1 = HUB_R1; cfg1.gpio.g1 = HUB_G1; cfg1.gpio.b1 = HUB_B1;
    cfg1.gpio.r2 = HUB_R0; cfg1.gpio.g2 = HUB_G0; cfg1.gpio.b2 = HUB_B0;
    cfg1.gpio.a = HUB_A; cfg1.gpio.b = HUB_B; cfg1.gpio.c = HUB_C; cfg1.gpio.d = HUB_D; cfg1.gpio.e = HUB_E;
    cfg1.gpio.lat = HUB_LAT; cfg1.gpio.oe = HUB_OE1; cfg1.gpio.clk = HUB_CLK;
    panel1 = new MatrixPanel_I2S_DMA(cfg1);
    panel1->begin();
    panel1->setBrightness8(90);
    panel1->clearScreen();

    // Define colors (for reference, optional)
    myBLACK = panel0->color565(0,0,0);
    myWHITE = panel0->color565(255,255,255);
    myRED   = panel0->color565(255,0,0);
    myGREEN = panel0->color565(0,255,0);
    myBLUE  = panel0->color565(0,0,255);

    // Demo shapes for both panels
    drawDemoShapes(panel0);
    drawDemoShapes(panel1);
}

uint8_t wheelval = 0;

void loop() {
    drawText(panel0, wheelval);
    drawText(panel1, wheelval);
    wheelval++;
    delay(20);
}
