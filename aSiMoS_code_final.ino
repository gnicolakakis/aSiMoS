// DEN ME NOIAZEI AMA DES SAS ARESEI TO STRACTURING <3
// go try mountain biking 

#if defined(ARDUINO) && ARDUINO >= 100                  // [emg]
#include "Arduino.h"                                    // [emg]
#else                                                   // [emg]
#include "WProgram.h"                                   // [emg]
#endif                                                  // [emg]
#include "EMGFilters.h"                                 // [emg]
#include <Wire.h>                                       // [max]
#include "DFRobot_BloodOxygen_S.h"                      // [max]
#include <Adafruit_MLX90614.h>// [mlx]
#include <U8g2lib.h>//Import the OLED graphics display library
#define I2C_ADDRESS 0x57                                // [max]
#define SensorInputPin A2                               // sensor input pin number // [emg]
Adafruit_MLX90614 mlx = Adafruit_MLX90614();            // [mlx]
DFRobot_BloodOxygen_S_I2C MAX30102(&Wire, I2C_ADDRESS); // [max]
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, A5, A4, U8X8_PIN_NONE); // screen
const int GSR = A0;                                     // [gsr]
int sensorValue = 0;                                    // [gsr]
int gsr_average = 0;                                    // [gsr]
int gsr_state = 32; //[gsr]
unsigned long threshold = 0;                            // threshold: Relaxed baseline values.(threshold=0:in the calibration process) // [emg]
unsigned long EMG_num = 0;                              // EMG_num: The number of statistical signals // [emg]
unsigned long avgStartTime = 0;                         // [emg]
unsigned long avgSum = 0;                               // [emg]
unsigned long avgCount = 0;                             // [emg]
const unsigned long AVG_INTERVAL = 10000;               // 10 seconds // [emg]
int sampleRate = SAMPLE_FREQ_500HZ;                     // [emg]
int humFreq = NOTCH_FREQ_50HZ;                          // [emg]
EMGFilters myFilter;                                    // [emg]

void page1() {
    // u8g2.setFont(u8g2_font_timR10_tf); //Sets the font to display
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.setFontPosTop(); //Set font position to be aligned close to the top
    u8g2.setCursor(3,50); //Set font display coordinates
    u8g2.print("Temp: ");
    u8g2.print(MAX30102.getTemperature_C());
    u8g2.setCursor(3,35);
    u8g2.print("GSR: ");
    u8g2.print(gsr_average);
    u8g2.setCursor(3,20);
    u8g2.print("H_R: ");
    u8g2.print(MAX30102._sHeartbeatSPO2.Heartbeat);
    u8g2.setCursor(3,5);
    u8g2.print("SPO2: ");
    u8g2.print(MAX30102._sHeartbeatSPO2.SPO2);
    u8g2.print("%");
    
    u8g2.setFont(u8g2_font_emoticons21_tr);
    u8g2.setCursor(90,5);
        
    //u8g2.setFontPosTop(); //Set font position to be aligned close to the top
    //u8g2.setCursor(3,50); //Set font display coordinates
    //gsr_state = check_gsr_values(gsr_average);
    
    if((gsr_average) < 350 && (MAX30102._sHeartbeatSPO2.Heartbeat<110) ){
        u8g2.print("\x20");  // No stress
    } else if ((gsr_average) < 500 && (MAX30102._sHeartbeatSPO2.Heartbeat<110)) {
        u8g2.print("\x24");  // Medium stress
    } else {
        u8g2.print("\x27");  // High stress
    }
        
}

void setup()
{
    myFilter.init(sampleRate, humFreq, true, true, true); // [emg]
    Serial.begin(115200);                                 // [global]
    avgStartTime = millis();                              // [emg]
    Wire.begin();                                         // Initialize MAX30102 // [max]
    while (!MAX30102.begin())
    {                                                      // [max]
        Serial.println("MAX30102: Initialization failed"); // [max]
        delay(1000);                                       // [max]
    }
    Serial.println("MAX30102: Initialized"); // [max]
    MAX30102.sensorStartCollect();           // [max]
    mlx.begin();                             // [mlx]
    u8g2.begin();//Initializing the display
    u8g2.enableUTF8Print();//Enable UTF8 printing
}

void loop()
{
    long sum = 0;                // [gsr]
    for (int i = 0; i < 10; i++) // Average the 10 measurements to remove the glitch // [gsr]
    {
        sensorValue = analogRead(GSR); // [gsr]
        sum += sensorValue;            // [gsr]
        delay(5);                      // [gsr]
    }
    gsr_average = sum / 10;      // [gsr]
    Serial.print("GSR: ");
    Serial.println(gsr_average); // [gsr]
    MAX30102.getHeartbeatSPO2(); // [max]
    Serial.print("SPO2: ");
    Serial.print(MAX30102._sHeartbeatSPO2.SPO2);
    Serial.println("%"); // [max]
    Serial.print("Heart Rate: ");
    Serial.print(MAX30102._sHeartbeatSPO2.Heartbeat);
    Serial.println(" BPM"); // [max]
    Serial.print("Board Temp: ");
    Serial.print(MAX30102.getTemperature_C());
    Serial.println(" °C");                     // [max]
    Serial.print("Ambient Temp = ");           // [mlx]
    Serial.print(mlx.readAmbientTempC());      // [mlx]
    Serial.print(" °C\tObject Temp = ");       // [mlx]
    Serial.print(mlx.readObjectTempC() + 2.0); // [mlx]
    Serial.println(" °C");                     // [mlx]
    delay(1000);                               // [global]

    static unsigned long lastSwitch = 0;
    static bool showPage1 = true;

    u8g2.firstPage();
    do {
        page1();
    } while (u8g2.nextPage());
    delay(1000);
    u8g2.clearBuffer();   

    int data = analogRead(SensorInputPin);
    int dataAfterFilter = myFilter.update(data);             // filter processing
    long envelope = (long)dataAfterFilter * dataAfterFilter; // Get envelope by squaring the input
    envelope = (envelope > threshold) ? envelope : 0;        // The data set below the base value is set to 0, indicating that it is in a relaxed state

    avgSum += envelope;
    avgCount++;

    if (millis() - avgStartTime >= AVG_INTERVAL)
    {
        if (avgCount > 0)
        {
            unsigned long avgValue = avgSum / avgCount;
            Serial.print("10s Average Envelope: ");
            Serial.print("EMG: ");
            Serial.println(avgValue);
        }

        avgSum = 0;
        avgCount = 0;
        avgStartTime = millis();
    }

    if (threshold > 0)
    {
        if (getEMGCount(envelope))
        {
            EMG_num++;
            Serial.print("EMG_num: ");
            Serial.println(EMG_num);
        }
    }
    else
    {
        Serial.print("EMG: ");
        Serial.println(envelope);
    }
    delayMicroseconds(500);
}

int getEMGCount(int gforce_envelope)
{
    static long integralData = 0;
    static long integralDataEve = 0;
    static bool remainFlag = false;
    static unsigned long timeMillis = 0;
    static unsigned long timeBeginzero = 0;
    static long fistNum = 0;
    static int TimeStandard = 200;
    /*
        The integral is processed to continuously add the signal value
        and compare the integral value of the previous sampling to determine whether the signal is continuous
    */
    integralDataEve = integralData;
    integralData += gforce_envelope;
    /*
        If the integral is constant, and it doesn't equal 0, then the time is recorded;
        If the value of the integral starts to change again, the remainflag is true, and the time record will be re-entered next time
    */
    if ((integralDataEve == integralData) && (integralDataEve != 0))
    {
        timeMillis = millis();
        if (remainFlag)
        {
            timeBeginzero = timeMillis;
            remainFlag = false;
            return 0;
        }
        /* If the integral value exceeds 200 ms, the integral value is clear 0,return that get EMG signal */
        if ((timeMillis - timeBeginzero) > TimeStandard)
        {
            integralDataEve = integralData = 0;
            return 1;
        }
        return 0;
    }
    else
    {
        remainFlag = true;
        return 0;
    }
}

//int check_gsr_values(int gsr_average){
    // gsr state = 0 unknown / starting state
    // gsr state = 1 good
    // gsr state = 2 mid 
    // gsr state = 3 bad
    //int gsr_state = 36;
    //if((gsr_average) < 400){
    //    gsr_state = 39;
    //} else if ((gsr_average) < 600 ) {
    //    gsr_state = 36;
    //} else {
    //    gsr_state = 32;
    //}
   // return gsr_state;
//}

// KAWAII KAIWAI 