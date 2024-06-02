
// Smart Medibox - EN2853 - Embedded Systems and Applications
// Name - Sehara G.M.M.
// Index Number - 210583B


// Libraries
#include <Wire.h>               // I2C Library for communication
#include <PubSubClient.h>       // PubSubClient Library for MQTT communication
#include <Adafruit_GFX.h>       // Graphics Library for graphical display
#include <Adafruit_SSD1306.h>   // OLED Display Library for SSD1306 displays
#include <DHTesp.h>             // DHT Library for DHT sensors
#include <WiFi.h>               // WiFi Library for ESP32 WiFi functionality
#include <NTPClient.h>          // NTPClient Library for time synchronization
#include <WiFiUdp.h>            // WiFiUdp Library for UDP communication
#include <ESP32Servo.h>         // ESP32Servo Library for servo motor control


// OLED display parameters
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Define Pins
#define BUZZER 5
#define LED_1 15

#define PB_CANCEL 25
#define PB_OK 27
#define PB_UP 14
#define PB_DOWN 26

#define DHTPIN 12
#define LDR_LEFT 35
#define LDR_RIGHT 32

// NTP Server details
#define NTP_SERVER     "pool.ntp.org"
float timeZone = 5.5;
int UTC_OFFSET = 3600 * timeZone;
int UTC_OFFSET_DST = 0;

// Declare objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;

// *******************************************************************
WiFiClient espClient;         // WiFi client for ESP32
PubSubClient mqttClient(espClient);  // MQTT client for ESP32

WiFiUDP ntpUDP;               // UDP client for NTP
NTPClient timeClient(ntpUDP); // NTP client for time synchronization

// ********************************************************************


//Global Variables
int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;

unsigned long timeNow = 0;
unsigned long timeLast = 0;

bool alarm_enabled = true;
int n_alarms = 3;
int alarm_hours[] = {0, 1, 2};
int alarm_minutes[] = {1, 10, 33};
bool alarm_triggered[] = {false, false, false};

// Notes for buzzer
int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C_H = 523;
int notes[] = {C, D, E, F, G, A, B, C_H};

// Mode parameters
int current_mode = 0;
int max_modes = 5;
String modes[] = {"1- Set Time Zone", "2 - Set Alarm 1", "3 - Set Alarm 2", "4 - Set Alarm 3", "5 - Disable Alarms"};

// *****************************************************************************
// Arrays to store temparature and LDR values
char tempArr[6];
char ldrLArr[4];
char ldrRArr[4];

//Initializing the servo
Servo servo;

//theta offset and gamma value
int t_off = 30;
float gamma_i = 0.75;


// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************


void setup() {

  // Defining Pin Modes for each pin
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(PB_CANCEL, INPUT );
  pinMode(PB_OK, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(LDR_LEFT, INPUT);
  pinMode(LDR_RIGHT, INPUT);

  dhtSensor.setup(DHTPIN, DHTesp::DHT22);

  Serial.begin(115200);

  //Error message if the Display Initialization Failed
  if (! display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  //Turn on the OLED Display
  display.display();
  delay(500);

  //Start Connecting To Wifi
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI", 0, 0, 2);
  }

  //At this point, Medibox is succesfully connected to wifi
  display.clearDisplay();
  print_line("Connected to WIFI", 0, 0, 2);

  //******************************************************
  setupMqtt();                    // Initialize MQTT setup

  timeClient.begin();            // Start NTP time client
  timeClient.setTimeOffset(5.5 * 3600);  // Set time offset for timezone (in seconds)

  servo.attach(23);              // Attach servo to pin 23

  //***************************************************

  //To configure the time
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  //Clear the display
  display.clearDisplay();

  //Welcome Message
  print_line("Welcome to Medibox", 8, 20, 2);

  //Welcome Tone of the Medibox
  for (int i = 0; i < 5 ; i++) {
    tone(BUZZER, notes[i]);
    delay(100);
  }
  noTone(BUZZER);

  delay(2000);
  display.clearDisplay();
}

// *****************************************************************************************
// *****************************************************************************************
// *****************************************************************************************


void loop() {

  //***************************************************************************************
  //***************************************************************************************

  if (!mqttClient.connected()) {   // Check if MQTT client is not connected
    Serial.println("Reconnecting to MQTT Broker");
    connectToBroker();            // Reconnect to MQTT broker
  }

  mqttClient.loop();              // Maintain MQTT client connection

  mqttClient.publish("TEMPERATURE_MIHI", tempArr);  // Publish temperature data
  delay(50);                    

  updateLight();  

  //Publish light intensity
  mqttClient.publish("LIGHT_INTENSITY_RIGHT_MIHI", ldrLArr);  
  delay(50);                    
  mqttClient.publish("LIGHT_INTENSITY_LEFT_MIHI", ldrRArr);   
  delay(100);               

  //**********************************************************************************
  //**********************************************************************************

  update_time_with_check_alarm();

  //check whether the menu button is pressed
  if (digitalRead(PB_OK) == LOW) {
    delay(200);
    go_to_menu();
  }

  //checking the humidity and temperature
  check_temp();
}

// ***********************      Function to print a line in the Display     ****************************
// *****************************************************************************************

void print_line(String text, int column, int row, int text_size) {

  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row);
  display.println(text);
  display.display();

}

// ***********************      To print the current time     ****************************
// *****************************************************************************************

void print_time_now(void) {
  display.clearDisplay();
  //Printing the days, hours, minutes and seconds seperately
  print_line(String(days), 0, 0, 2);
  print_line(String(":"), 20, 0, 2);
  print_line(String(hours), 30, 0, 2);
  print_line(String(":"), 50, 0, 2);
  print_line(String(minutes), 60, 0, 2);
  print_line(String(":"), 80, 0, 2);
  print_line(String(seconds), 90, 0, 2);
}

// ***********************      Get the current time from Wifi    ****************************
// *****************************************************************************************

void update_time() {
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  hours = atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute, 3, "%M", &timeinfo);
  minutes = atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond, 3, "%S", &timeinfo);
  seconds = atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay, 3, "%d", &timeinfo);
  days = atoi(timeDay);

}

// ***********************      To ring the alarm and display it on the display    ****************************
// ********************************************************************************************************

void ring_alarm() {
  display.clearDisplay();
  print_line("Medicine Time !!!", 0, 0, 2);

  digitalWrite(LED_1, HIGH);

  bool break_happened = false; //to stop ringing when the cancel button is pressed

  //Ring the alarm
  while (break_happened == false && digitalRead(PB_CANCEL) == HIGH) {
    for (int i = 0; i < n_notes; i++) {

      //check whenther the cancel button is pressed
      if (digitalRead(PB_CANCEL) == LOW) {
        delay(200);
        break_happened = true;
        break;
      }

      //the alarm tone
      tone(BUZZER, notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }

  }
  digitalWrite(LED_1, LOW);
  display.clearDisplay();

}

// ***********************      To check the time is the alarm time     ****************************
// ********************************************************************************************************

void update_time_with_check_alarm(void) {

  print_time_now();
  update_time();

  if (alarm_enabled == true) {
    for (int i = 0; i < n_alarms; i++) {
      if (alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minutes[i] == minutes ) {
        ring_alarm();
        alarm_triggered[i] = true;
      }
    }
  }
}

// ***********************      To identify whether a button is pressed     ****************************
// ********************************************************************************************************

//This function returns the the button that was pressed
int wait_for_button_press() {
  while (true) {
    if (digitalRead(PB_UP) == LOW) {
      beep_sound(); //To give a beep sound when a button is pressed
      delay(200);
      return PB_UP;
    }

    else if (digitalRead(PB_DOWN) == LOW) {
      beep_sound();
      delay(200);
      return PB_DOWN;
    }

    else if (digitalRead(PB_OK) == LOW) {
      beep_sound();
      delay(200);
      return PB_OK;
    }

    else if (digitalRead(PB_CANCEL) == LOW) {
      beep_sound();
      delay(200);
      return PB_CANCEL;
    }
  }
}

// *******************************************      The main menu     ********************************
// ********************************************************************************************************

void go_to_menu() {

  display.clearDisplay();
  print_line("Menu", 0, 2, 2);
  delay(1000);

  while (digitalRead(PB_CANCEL) == HIGH) {
    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);
    delay(1000);

    //saved the button pressed to a variable
    int pressed = wait_for_button_press();

    if (pressed == PB_UP) {
      delay(200);
      current_mode += 1;
      current_mode = current_mode % max_modes;
    }

    else   if (pressed == PB_DOWN) {
      delay(200);
      current_mode -= 1;
      if (current_mode < 0) {
        current_mode = max_modes - 1;
      }
    }

    else   if (pressed == PB_OK) {
      delay(200);
      run_mode(current_mode);
    }

    else   if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }
}

// ************************************      To set the time zone     **********************************
// ********************************************************************************************************


void set_time_zone() {
  int temp_hour = 0; // Initialize temporary variable to store hour

  // Loop until user sets hour within the valid range
  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2); // Display current hour

    int pressed = wait_for_button_press();


    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      if (temp_hour > 14) { // If hour exceeds maximum timezone
        temp_hour = -12; // Reset to minimum timezone
      }
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < -12) { // If hour goes below minimum timezone
        temp_hour = 14; // Reset to maximum timezone
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      UTC_OFFSET = (temp_hour * 3600); // Set UTC offset based on selected hour
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int temp_minute = 0; // Initialize temporary variable to store minute

  // Loop until user sets minute within the valid range
  while (true) {
    display.clearDisplay();
    print_line("Enter Minute: " + String(temp_minute), 0, 0, 2); // Display current minute

    int pressed = wait_for_button_press();


    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 15; // Increment minute by 15
      temp_minute = temp_minute % 60; // Ensure minute stays within 0-59 range
    }

    else if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 15; // Decrement minute by 15
      if (temp_minute <= -1) { // If minute goes below 0
        temp_minute = 45; // Reset to 45
      }
    }

    else if (pressed == PB_OK) {
      delay(200);
      if (temp_hour >= 0) {
        UTC_OFFSET = UTC_OFFSET + (temp_minute * 60); // Add minute offset if timezone is positive
      }
      else {
        UTC_OFFSET = UTC_OFFSET - (temp_minute * 60); // Subtract minute offset if timezone is negative
      }
      break;
    }

    else if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  // Configure time with the calculated UTC offset and NTP server
  configTime(UTC_OFFSET, UTC_OFFSET, NTP_SERVER);

  display.clearDisplay();
  print_line("Time Zone is set", 0, 0, 2); // Indicate time zone is set
  delay(1000);
}



// ***********************      To set the alarm time     ****************************
//*******************************************************************************

void set_alarm(int alarm) {
  int temp_hour = alarm_hours[alarm];

  while (true) {
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hour), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_hour += 1;
      temp_hour = temp_hour % 24;
    }

    else   if (pressed == PB_DOWN) {
      delay(200);
      temp_hour -= 1;
      if (temp_hour < 0) {
        temp_hour = 23;
      }
    }

    else   if (pressed == PB_OK) {
      delay(200);
      alarm_hours[alarm] = temp_hour;
      break;
    }

    else   if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  int temp_minute = alarm_minutes[alarm];

  while (true) {
    display.clearDisplay();
    print_line("Enter minute: " + String(temp_minute), 0, 0, 2);

    int pressed = wait_for_button_press();
    if (pressed == PB_UP) {
      delay(200);
      temp_minute += 1;
      temp_minute = temp_minute % 60;
    }

    else   if (pressed == PB_DOWN) {
      delay(200);
      temp_minute -= 1;
      if (temp_minute < 0) {
        temp_minute = 59;
      }
    }

    else   if (pressed == PB_OK) {
      delay(200);
      alarm_minutes[alarm] = temp_minute;
      break;
    }

    else   if (pressed == PB_CANCEL) {
      delay(200);
      break;
    }
  }

  display.clearDisplay();
  print_line("Alarm is set", 0, 0, 2);

  Serial.println(alarm_hours[alarm]);
  Serial.println(alarm_minutes[alarm]);

  delay(2000);
}

// ************************************ To select the run mode *************************************
// *************************************************************************************************

void run_mode(int mode) {
  if (mode == 0) {
    set_time_zone();
  }
  else if (mode == 1 || mode == 2 || mode == 3) {
    set_alarm(mode - 1);
  }

  else if (mode == 4) {
    alarm_enabled = false;
  }
}

// ************************************ To Check the Temperature and humidity *************************************
// *************************************************************************************************

void check_temp() {
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  //*******************************************************************************

  String(data.temperature, 2).toCharArray(tempArr, 6);

  //********************************************************************************

  if (data.temperature > 32) {
    display.clearDisplay();
    print_line("TEMP HIGH", 0, 30, 2);
    delay(1000);
  }

  else if (data.temperature < 26) {
    display.clearDisplay();
    print_line("TEMP LOW", 0, 30, 2);
    delay(1000);
  }

  else if (data.humidity > 80) {
    display.clearDisplay();
    print_line("HUMIDITY", 0, 20, 2);
    print_line("HIGH", 0, 40, 2);
    delay(1000);
  }

  else if (data.humidity < 60) {
    display.clearDisplay();
    print_line("HUMIDITY", 0, 20, 2);
    print_line("LOW", 0, 40, 2);
    delay(1000);
  }
}

// ************************************ To Give a beep sound when a button is pressed *************************************
// *************************************************************************************************


void beep_sound() {
  tone(BUZZER, notes[0]);
  delay(10);
  noTone(BUZZER);
  delay(10);
}

//****************************************************************************************************************************
//*****************************************************************************

// Function to set up MQTT connection
void setupMqtt() {
  // Set the MQTT server and port
  mqttClient.setServer("test.mosquitto.org", 1883 );
  // Set the callback function to handle received messages
  mqttClient.setCallback(receiveCallback);
}


// Function to connect to MQTT broker
void connectToBroker() {
  // Loop until the MQTT client is connected
  while (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    
    // Attempt to connect to the MQTT broker
    if (mqttClient.connect("ESP32Client-sdfsjdfsdfsdf")) {
      Serial.println("MQTT Connected");
      
      // Subscribe to Min angle and Controlling factor adjustment
      mqttClient.subscribe("MIN_ANG_ADJUSTMENT");
      mqttClient.subscribe("CF_ADJUSTMENT");
    } else {
      // If connection failed, print error message and state
      Serial.print("Failed To connect to MQTT Broker");
      Serial.print(mqttClient.state());
      delay(5000); // Wait before retrying
    }
  }
}


// Callback function to handle received MQTT messages
void receiveCallback(char *topic, byte *payload, unsigned int length) {
  // Print the topic of the received message
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  // Print the received message payload
  char payloadCharArray[length];
  Serial.print("Message Received: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    payloadCharArray[i] = (char)payload[i];
  }
  Serial.println();

  // Parse and handle the received message based on the topic
  if (strcmp(topic, "MIN_ANG_ADJUSTMENT") == 0) {
    // Convert the payload to an integer and assign it to 't_off'
    t_off = String(payloadCharArray).toInt();

  } else if (strcmp(topic, "CF_ADJUSTMENT") == 0) {
    // Convert the payload to a float and assign it to 'gamma_i'
    gamma_i = String(payloadCharArray).toFloat();
  }
}


// Function to update the light values and angles
void updateLight() {
  // Read analog values from left and right LDRs
  float lsv = analogRead(LDR_LEFT) * 1.00;
  float rsv = analogRead(LDR_RIGHT) * 1.00;

  // Convert analog values to normalized values (between 0 and 1)
  float lsv_cha = (float)(lsv - 4063.00) / (32.00 - 4063.00);
  float rsv_cha = (float)(rsv - 4063.00) / (32.00 - 4063.00);

  // Update the angle based on the normalized light values
  updateAngle(lsv_cha, rsv_cha);

  // Convert the normalized light values to character arrays for display
  String(lsv_cha).toCharArray(ldrLArr, 4);
  String(rsv_cha).toCharArray(ldrRArr, 4);
}


// Function to update the angle of the servo motor
void updateAngle(float lsv, float rsv) {
  // Determine the maximum normalized light value and corresponding direction
  float max_I = lsv;
  float D = 1.5; // Default direction (left)

  // If the right sensor's value is greater, update max_I and direction
  if (rsv > max_I) {
    max_I = rsv;
    D = 0.5; // Direction (right)
  }

  // Calculate the servo angle based on light values and adjustment factors
  int theta = t_off * D + (180 - t_off) * max_I * gamma_i;
  // Ensure the angle is within the valid range (0 to 180 degrees)
  theta = min(theta, 180);

  // Move the servo motor to the calculated angle
  servo.write(theta);
  // Print the servo angle for debugging purposes
  Serial.println("Servo angle: " + String(theta));
}
