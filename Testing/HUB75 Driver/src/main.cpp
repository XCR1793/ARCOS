#include <Arduino.h>
#include <ImageTest.h>
#include <Display.h>

HUB75 Panels;

// R0  2
// B0  3
// R1  4
// B1  5
// A   6
// C   7
// CLK 8
// OE  9

// G0  14
// G1  13
// E   12
// B   12
// D   11
// LAT 10


void setup(){
    Serial.begin(9600);
    Panels.PanelCount(2);
    Panels.Buffer(5);
    Panels.SetSize(32, 64);
    Panels.SetPins(2, 14, 3, 4, 13, 5, 6, 15, 7, 11, 12, 10, 9, 8);
    Panels.Initialise();
    Panels.Clear();
    // Serial.println("Setup");
}

void loop(){
    Panels.DrawLine(0, 0, 128, 32, 0xffff);
    // Serial.println("Started");

    Panels.drive_HUB75();
    Panels.Clear();
    // Serial.println("Finished");
    // delay(500);
}