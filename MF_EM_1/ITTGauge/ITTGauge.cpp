#include <TFT_eSPI.h>
#include "ITTGauge.h"
#include "allocateMem.h"
#include "commandmessenger.h"
#include "include/ITT_Gauge.h"
#include "include/ITT_Dotted_Line.h"
#include "../Common/DotMatrix_Regular-30.h"
#include "../Common/Needle.h"
#include "../Common/Red_led.h"
#include "../Common/Red_marker.h"
#include "RunningAverage.h"

#define BACKGROUND_COLOR  0x1041

static TFT_eSPI    tft;
static TFT_eSprite mainGaugeSpr = TFT_eSprite(&tft);
static TFT_eSprite needleSpr = TFT_eSprite(&tft);
static TFT_eSprite redLEDSpr = TFT_eSprite(&tft);
static TFT_eSprite redMarkerSpr = TFT_eSprite(&tft);
static TFT_eSprite dottedLineSpr = TFT_eSprite(&tft);

RunningAverage RA_ITTNeedleRotationAngle(5);   // for Running Average of the needle rotation
int ITTMessageID = -100;

/* **********************************************************************************
    This is just the basic code to set up your custom device.
    Change/add your code as needed.
********************************************************************************** */

ITTGauge::ITTGauge(uint8_t Pin1, uint8_t Pin2)
{
    _pin1 = Pin1;
    _pin2 = Pin2;
}

void ITTGauge::begin()
{
}

void ITTGauge::attach(uint16_t Pin3, char *init)
{
    _pin3 = Pin3;

    // tft = &_tft;
    tft.init();
    tft.setRotation(0);
    tft.setPivot(120, 120);
    tft.fillScreen(TFT_BLACK);
    tft.startWrite(); // TFT chip select held low permanently


    mainGaugeSpr.createSprite(ITT_GAUGE_WIDTH, ITT_GAUGE_HEIGHT);
    mainGaugeSpr.setPivot(120, 120);
    mainGaugeSpr.loadFont(DotMatrix_Regular_30);
    mainGaugeSpr.setTextColor(TFT_GREEN);
    mainGaugeSpr.setTextDatum(TR_DATUM);
    mainGaugeSpr.pushImage(0, 0, ITT_GAUGE_WIDTH, ITT_GAUGE_HEIGHT, ITT_Gauge);

    needleSpr.createSprite(NEEDLE_WIDTH, NEEDLE_HEIGHT);
    needleSpr.setPivot(NEEDLE_WIDTH / 2, 80);
    needleSpr.pushImage(0, 0, NEEDLE_WIDTH, NEEDLE_HEIGHT, Needle);

    redLEDSpr.createSprite(RED_LED_WIDTH, RED_LED_HEIGHT);
    redLEDSpr.pushImage(0, 0, RED_LED_WIDTH, RED_LED_HEIGHT, Red_led);

    redMarkerSpr.createSprite(RED_MARKER_WIDTH, RED_MARKER_HEIGHT);
    redMarkerSpr.setPivot(RED_MARKER_WIDTH / 2, 110);
    redMarkerSpr.pushImage(0, 0, RED_MARKER_WIDTH, RED_LED_HEIGHT, Red_marker);

    dottedLineSpr.createSprite(ITT_DOTTED_LINE_WIDTH, ITT_DOTTED_LINE_HEIGHT);
    dottedLineSpr.setPivot(ITT_DOTTED_LINE_WIDTH / 2, 105);
    dottedLineSpr.pushImage(0, 0, ITT_DOTTED_LINE_WIDTH, ITT_DOTTED_LINE_HEIGHT, ITT_Dotted_Line);

    RA_ITTNeedleRotationAngle.clear();    // clear running average

    analogWrite(TFT_BL, instrumentBrightness);

}

void ITTGauge::detach()
{
    if (!_initialised)
        return;

    mainGaugeSpr.deleteSprite();
    needleSpr.deleteSprite();
    redLEDSpr.deleteSprite();
    dottedLineSpr.deleteSprite();
    tft.endWrite();

    _initialised = false;
}

void ITTGauge::set(int16_t messageID, char *setPoint)
{
    /* **********************************************************************************
        Each messageID has it's own value
        check for the messageID and define what to do.
        Important Remark!
        MessageID == -2 will be send from the board when PowerSavingMode is set
            Message will be "0" for leaving and "1" for entering PowerSavingMode
        MessageID == -1 will be send from the connector when Connector stops running
        Put in your code to enter this mode (e.g. clear a display)

    ********************************************************************************** */
    ITTMessageID = messageID;
    // do something according your messageID
    switch (messageID) {
    case -1:
        // setPowerSave(true);
        break;
    case -2:
        setPowerSave((bool)atoi(setPoint));
        break;
    case 0:
        setITT(atof(setPoint));
        break;
    case 1:
        setITTGreenArcStart(atof(setPoint));
        break;
    case 2:
        setITTGreenArcEnd(atof(setPoint));
        break;
    case 3:
        setITTYellowArcStart(atof(setPoint));
        break;
    case 4:
        setITTYellowArcEnd(atof(setPoint));
        break;
    case 5:
        setITTRedlineTOGA(atof(setPoint));
        break;
    case 6:
        setITTRedlineMax(atof(setPoint));
        break;
    case 100:
        setInstrumentBrightness(atof(setPoint));
        break;
    default:
        break;
    }

    // draw the Fuel Flow Gauge

}

void ITTGauge::update()
{
    // Do something which is required regulary
    if (ITTMessageID == -1 || powerSaveFlag == true)  // Mobiflight Connector has stopped or entered power save mode
    {
        tft.fillScreen(TFT_BLACK);
        analogWrite(TFT_BL, 0);
    }
    else
    {
        float pwmOutput = 0;
        pwmOutput = sq(instrumentBrightness) / 255.0;  // needed to correct PWM output due to human eye brightness perception
        analogWrite(TFT_BL, pwmOutput);
        drawGauge();
    }
}   

void ITTGauge::drawGauge()
{

    oneValue = (int)ITT % 10;
    tenValue = (int)(ITT / 10) % 10;
    hundredValue = (int)(ITT / 100) % 10;
    thousandValue = (int)(ITT / 1000) % 10;

    minGreenAngle = scaleValue(minGreenITT, 200, 1200, -110, 110);
    maxGreenAngle = scaleValue(maxGreenITT, 200, 1200, -110, 110);
    minYellowAngle = scaleValue(minYellowITT, 200, 1200, -110, 110);
    maxYellowAngle = scaleValue(maxYellowITT, 200, 1200, -110, 110);
    redLineAngle = scaleValue(redlineITT, 200, 1200, -110, 110);
    startLimitsAngle = scaleValue(startLimitsITT, 200, 1200, -110, 110);

    needleRotationAngle = scaleValue(ITT, 200, 1200, -110, 110);
    RA_ITTNeedleRotationAngle.addValue(needleRotationAngle);

    mainGaugeSpr.fillSprite(TFT_BLACK);
    
    mainGaugeSpr.pushImage(0, 0, 240, 240, ITT_Gauge);

    // Draw Green Arc
    mainGaugeSpr.drawSmoothArc(120, 120, 205 / 2, 195 / 2, minGreenAngle + 180, maxGreenAngle + 180, TFT_GREEN, TFT_BLACK);

    // Draw Yellow Arc
    mainGaugeSpr.drawSmoothArc(120, 120, 205 / 2, 195 / 2, minYellowAngle + 180, maxYellowAngle + 180, TFT_YELLOW, TFT_BLACK);

    // Draw Red Marker
    redMarkerSpr.pushRotated(&mainGaugeSpr, redLineAngle, BACKGROUND_COLOR);

    // Draw Start Limits Marker
    dottedLineSpr.pushRotated(&mainGaugeSpr, startLimitsAngle, BACKGROUND_COLOR);

    // Draw the numbers in the digital display
    mainGaugeSpr.drawString(String(oneValue), 162, 170);
    if (ITT >= 10)
        mainGaugeSpr.drawString(String(tenValue), 140, 170);
    if (ITT >= 100)
        mainGaugeSpr.drawString(String(hundredValue), 119, 170);
    if (ITT >= 1000)
        mainGaugeSpr.drawString(String(thousandValue), 97, 170);

    // Draw the needle
    needleSpr.pushRotated(&mainGaugeSpr, RA_ITTNeedleRotationAngle.getAverage(), BACKGROUND_COLOR);

    // Draw Red Led if red line is crossed
    if (ITT >= redlineITT )
        redLEDSpr.pushToSprite(&mainGaugeSpr, 38, 159, BACKGROUND_COLOR);

    mainGaugeSpr.pushSprite(0, 0);

}

// Setters
void ITTGauge::setITT (float value)
{
    ITT = value;
}

void ITTGauge::setITTGreenArcStart(float value)
{
    minGreenITT = value;
}

void ITTGauge::setITTGreenArcEnd(float value)
{
    maxGreenITT = value;
}

void ITTGauge::setITTYellowArcStart(float value)
{
    minYellowITT = value;
}

void ITTGauge::setITTYellowArcEnd(float value)
{
    maxYellowITT = value;
}

void ITTGauge::setITTRedlineTOGA(float value)
{
    redlineITT = value;
}

void ITTGauge::setITTRedlineMax(float value)
{
    startLimitsITT = value;
}

void ITTGauge::setInstrumentBrightness(float value)
{
    float pwmOutput = 0;

    instrumentBrightness = scaleValue(value, 0, 1, 0, 255);
    pwmOutput = sq(instrumentBrightness) / 255.0;  // needed to correct PWM output due to human eye brightness perception
    analogWrite(TFT_BL, pwmOutput);
}

void ITTGauge::setPowerSave(bool enabled)
{
    if (enabled) {
        powerSaveFlag = true;
    } else {
        powerSaveFlag = false;
    }
}


// Scale function
float ITTGauge::scaleValue(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}