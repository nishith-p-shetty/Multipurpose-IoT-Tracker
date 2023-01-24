#include "WiFi.h"

#define SOS 15
#define SLEEP_PIN 2 // Make this pin HIGH to make A9G board to go to sleep mode

String SOS_NUM = "+91xxxxxxxxxx"; // Add a number on which you want to receive call or SMS

int SOS_Time = 5; // Press the button 5 sec

// Necessary Variables
boolean stringComplete = false;
String inputString = "";
String fromGSM = "";
bool CALL_END = 1;
char *response = " ";
String res = "";
int c = 0;

void setup()
{
    Serial.begin(115200);                     // For Serial Monitor
    Serial1.begin(115200, SERIAL_8N1, 5, 18); // For A9G Board

    delay(5000);
    Serial.println("\nWelcome To Tracker  DEBUG INFORMATION"); // Serial Monitor

    // Making Radio OFF for power saving
    WiFi.mode(WIFI_OFF);              // WiFi OFF
    btStop();                         // Bluetooth OFF
    Serial.println("Wifi & Ble OFF"); // Serial Monitor

    pinMode(SOS, INPUT_PULLUP);
    pinMode(SLEEP_PIN, OUTPUT);

    // Waiting for A9G to setup everything for 20 sec
    //delay(1000);
    delay(30000);

    digitalWrite(SLEEP_PIN, LOW); // Sleep Mode OFF
    Serial.println("Wake"); // Serial Monitor

    Serial.println("Just Checking : AT"); // Serial Monitor
    Serial1.println("AT");                // Just Checking
    delay(1000);

    Serial.println("Turning ON GPS : AT+GPS = 1"); // Serial Monitor
    Serial1.println("AT+GPS = 1");                 // Turning ON GPS
    delay(1000);

    //Serial.println("GPS low power : AT+GPSLP = 2"); // Serial Monitor
    Serial.println("GPS no low power : AT+GPSLP = 1"); // Serial Monitor
    //Serial1.println("AT+GPSLP = 2");                // GPS low power
    Serial1.println("AT+GPSLP = 1");                // GPS low power
    delay(1000);

    Serial.println("Configuring Sleep Mode to 1 : AT+SLEEP = 1"); // Serial Monitor
    Serial1.println("AT+SLEEP = 1");                              // Configuring Sleep Mode to 1
    delay(1000);

    Serial.println("Select SMS message format = text mode : AT+CMGF = 1"); // Serial Monitor
    Serial1.println("AT+CMGF = 1");                                        // Select SMS message format = text mode
    delay(1000);

    Serial.println("Set Text Mode Parameters : AT+CSMP  = 17,167,0,0"); // Serial Monitor
    Serial1.println("AT+CSMP  = 17,167,0,0 ");                          // Set Text Mode Parameters = (7bit encode of message to store or send in text mode)
    delay(1000);

    Serial.println("Preferred SMS message storage : AT+CPMS = \"SM\",\"ME\",\"SM\""); // Serial Monitor
    Serial1.println("AT+CPMS = \"SM\",\"ME\",\"SM\" ");                               // Preferred SMS message storage = sim memory sim
    delay(1000);

    digitalWrite(SLEEP_PIN, HIGH); // Sleep Mode ON
    Serial.println("Slept");       // Serial Monitor
}

void loop()
{
    {
        // listen from GSM Module
        if (Serial1.available())
        {
            char inChar = Serial1.read();

            if (inChar == '\n')
            {
                // check the state
                if (fromGSM == "SEND LOCATION\r")
                {
                    Get_gmap_link(0);              // Send Location without Call
                    digitalWrite(SLEEP_PIN, HIGH); // Sleep Mode ON
                    Serial.println("Slept");       // Serial Monitor
                } 

                if (fromGSM == "SEND LC\r")
                {
                    Get_gmap_link(1);              // Send Location without Call
                    digitalWrite(SLEEP_PIN, HIGH); // Sleep Mode ON
                    Serial.println("Slept");       // Serial Monitor
                } 

                else if (fromGSM == "RING\r")
                {
                    digitalWrite(SLEEP_PIN, LOW); // Sleep Mode OFF
                    Serial.println("---------ITS RINGING-------");
                    Serial1.println("ATA");     // receive
                    Serial.println("Received"); // Serial Monitor
                }

                else if (fromGSM == "NO CARRIER\r")
                {
                    Serial.println("---------CALL ENDS-------");
                    CALL_END = 1;
                    digitalWrite(SLEEP_PIN, HIGH); // Sleep Mode ON
                    Serial.println("Slept");       // Serial Monitor
                }

                // write the actual response
                Serial.println(fromGSM);
                // clear the buffer
                fromGSM = "";
            }

            else
            {
                fromGSM += inChar;
            }
            delay(20);
        }

        // read from port 0, send to port 1:
        if (Serial.available())
        {
            int inByte = Serial.read();
            Serial1.write(inByte);
        }

        // When SOS button is pressed
        if (digitalRead(SOS) == LOW && CALL_END == 1)
        {
            Serial.println("SOS Pressed");  // Serial Monitor
            Serial.println("Calling In.."); // Waiting for 5 sec

            for (c = 0; c < SOS_Time; c++)
            {
                Serial.println((SOS_Time - c));
                delay(1000);

                if (digitalRead(SOS) == HIGH)
                {
                    Serial.println("Didn't Call, SOS released within " + (String)SOS_Time + " Seconds"); // Serial Monitor
                    break;
                }
            }

            if (c == SOS_Time)
            {
                Get_gmap_link(1); // Send Location with Call
            }

            // only write a full message to the GSM module
            if (stringComplete)
            {
                Serial1.print(inputString);
                inputString = "";
                stringComplete = false;
            }
        }
    }
}

// Getting Location and making Google Maps link of it. Also making call if needed
void Get_gmap_link(bool makeCall)
{
    digitalWrite(SLEEP_PIN, LOW);
    Serial.println("Wake"); // Serial Monitor

    delay(1000);

    Serial.println("Fetching Location : AT+LOCATION = 2"); // Serial Monitor
    Serial1.println("AT+LOCATION = 2");

    while (!Serial1.available());
    while (Serial1.available())
    {
        char add = Serial1.read();
        res = res + add;
        delay(1);
    }

    res = res.substring(17, 38);
    response = &res[0];

    Serial.print("Recevied Location Data : ");
    Serial.println(response); // printin the String in lower character form
    Serial.println("\n");

    if (strstr(response, "GPS NOT"))
    {
        Serial.println("Couldn't Fetch Location data");

        // Sending SMS without any location
        Serial.println("Sending SMS"); // Serial Monitor
        Serial1.println("AT+CMGF=1");
        delay(1000);
        Serial1.println("AT+CMGS=\"" + SOS_NUM + "\"\r");
        delay(1000);
        Serial1.println("Unable to fetch location. Please try again");
        delay(1000);
        Serial1.println((char)26);
        delay(1000);
        Serial.println("SMS Sent"); // Serial Monitor
    }

    else
    {
        int i = 0;

        while (response[i] != ',')
        {    
            i++;
        }

        String location = (String)response;
        String lat = location.substring(2, i);
        String longi = location.substring(i + 1);
        Serial.println(lat + " " + longi);

        String Gmaps_link = ("http://maps.google.com/maps?q=" + lat + "+" + longi); // http://maps.google.com/maps?q=38.9419+-78.3020
        
        // Sending SMS with Google Maps Link with our Location
        Serial.println("Sending SMS"); // Serial Monitor
        Serial1.println("AT+CMGF=1");
        delay(1000);
        Serial1.println("AT+CMGS=\"" + SOS_NUM + "\"\r");
        delay(1000);
    
        Serial1.println("I'm here: " + Gmaps_link);
        delay(1000);
        Serial1.println((char)26);
        delay(1000);
        Serial.println("SMS Sent"); // Serial Monitor
    
        Serial1.println("AT+CMGD=1,4"); // delete stored SMS to save memory
        delay(5000);
        Serial.println("Deleted stored SMS to save memory"); // Serial Monitor
    }
    response = "";
    res = "";

    if (makeCall)
    {
        Serial.println("Calling Now");
        Serial1.println("ATD" + SOS_NUM);
        CALL_END = 0;
    }
}
