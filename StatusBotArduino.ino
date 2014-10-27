/**
 * \file StatusBot.ino
 *
 * \brief sketch for Lansing Makers Network (aka LMN) Web StatusBot
 * to post on lansingmakersnetwork.org
 *
 * \remarks comments are implemented with Doxygen Markdown format
 *
 * \author Michael P. Flaga
 * \author Randy Schott
 *
 * This sketch both returns and updates http://api.lansingmakersnetwok.org/status
 * (as specified by ServerInfo.h) with the status of the Switch.
 *
 * \note Uses the standard Ethernet Shield, as DHCP.
 */

/**
\mainpage Arduino LMN StatusBot

\tableofcontents

<CENTER>Copyright &copy; 2013</CENTER>

\section Intro Introduction
The LMN's Arduino StatusBot to connect to web's API, for display on the web server.

\section Contributors Contributors
 * \author Michael P. Flaga
 * \author Randy Schott


\section API Description

The api is pretty simple (I'm reverse engineering this from the code on the lmn server) and looks like this:

GET request to http://api.lansingmakersnetwok.org/status returns the current status of the space in json.

openStatus: true or false (0, 1)
updated: DateTime the status was last updated.

POST request to http://api.lansingmakersnetwork.org/status with the following parameters sets the current status of the space:

status: "true" or "false" (true == IS Open, false == is NOT open)

\section Arduino Arduino HTTP Trace

Arduino Request:
\code
Hypertext Transfer Protocol
    GET /status HTTP/1.0\n
    User-Agent: Wget/1.11.4\n
    Accept: *\/*\n
    Host: api.lansingmakersnetwork.org\n
    Connection: close\n
    \n
\endcode

Websites Response:
\code
Hypertext Transfer Protocol
    HTTP/1.1 200 OK\r\n
    Date: Fri, 10 Oct 2014 13:51:38 GMT\r\n
    Server: Apache/2.2.22 (Ubuntu)\r\n
    X-Powered-By: PHP/5.3.10-1ubuntu3.14\r\n
    Access-Control-Allow-Origin: *\r\n
    Content-Length: 62\r\n
    Connection: close\r\n
    Content-Type: application/json\r\n
    \r\n
    {"openStatus":false,"updated":"Friday, October 10th @ 9:50am"}
JavaScript Object Notation: application/json
    Object
        Member Key: "openStatus"
            False value
        Member Key: "updated"
            String value: Friday, October 10th @ 9:50am
\endcode

Arduino Post:
\code
Hypertext Transfer Protocol
    POST /status HTTP/1.1\n
    Host: api.lansingmakersnetwork.org\n
    Connection: close\n
    Content-Type: application/json; charset=utf8\n
    Content-Length: 66\n
    \n
    {"token": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", "openStatus": false}
JavaScript Object Notation: application/json
    Object
        Member Key: "token"
            String value: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
        Member Key: "openStatus"
            False value
\endcode

Return codes for POST:
 - 201: successfully updated.
 - 400: not successfully updated
 - 403: token wasn't valid.

Websites Response:
\code
Hypertext Transfer Protocol
    HTTP/1.1 201 Created\r\n
    Date: Fri, 10 Oct 2014 13:51:39 GMT\r\n
    Server: Apache/2.2.22 (Ubuntu)\r\n
    X-Powered-By: PHP/5.3.10-1ubuntu3.14\r\n
    Vary: Accept-Encoding\r\n
    Content-Encoding: gzip\r\n
    Content-Length: 20\r\n
    Connection: close\r\n
    Content-Type: text/html\r\n
    \r\n
\endcode

\section Wget Wget Examples

Wget Example:
Request:
\code
wget -d -O status.txt http://api.lansingmakersnetwork.org/status
\endcode

Post:
\code
wget -d -O status.txt --post-data="{\"token\": \"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\", \"openStatus\": false}" http://api.lansingmakersnetwork.org/status
\endcode

*/

//#define NO_ETHERNET

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @{
// Pin_Definitions

/** \brief A macro to define the Yellow LED pin
 *
 * Digital output pin used drive the LED. 
 * Where a HIGH or TRUE value will illuminate the LED,
 * indicating an error with updating to for from the website.
 */
#define PIN_LED_YLW 8 

/** \brief A macro to define the Red LED pin
 *
 * Digital output pin used drive the LED. 
 * Where a HIGH or TRUE value will illuminate the LED,
 * indicating the website is announcing the site as "Closed to public status"

 */
#define PIN_LED_RED 7

/** \brief A macro to define the Green LED pin
 *
 * Digital output pin used drive the LED.
 * Where a HIGH or TRUE value will illuminate the LED,
 * indicating the website is announcing the site as "Open to public status"
 */
#define PIN_LED_GRN 4

/** \brief A macro to define the External Relay pin
 *
 * Digital output pin used drive the External Relay.
 * Where a HIGH or TRUE value will engage the Relay,
 * indicating the space as "Open to public status"
 */
#define PIN_XRELAY 3

/** \brief A macro to define the Toggle switch pin
 *
 * Digital input pin used sense the position of the switch. 
 * Where a HIGH or TRUE value indicates a "Open to public status"
 * and LOW or FALSE value indicates a "Closed to public status".
 */
#define PIN_SWTCH   2

// @}
// Pin_Definitions

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @{
// API_Server_Definitions

/** \brief A macro to set the timeout of the HTTP request
 *
 * An integer value units of milliseconds.
 */
#define HTTP_RESPONSE_TIMEOUT 2000

// @}
// API_Server_Definitions

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// @{
// EEPROM_Configuration_Definitions

/** \brief A macro to set the start of configuration
 *
 * Define the initial location of the configuration struct used in EEPROM space.
 */
#define CONFIG_START 32

/** \brief A macro to set the version of configuration
 *
 * Define the version of the configuration struct used in EEPROM space.
 */
#define CONFIG_VERSION "ls1"

// @}
// EEPROM_Configuration_Definitions


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/** \brief A macro to set Debug Level
 *
 * any value other than zero will enable the function,
 * such as Serial.print().
 */
#define DEBUG_LEVEL 1

/** \brief A macro that encapsulates functions
 *
 * When DEBUG_LEVEL is zero, the encapsulated function is replaced with "0",
 * as not to be compiled in.
 */
 #define IFDEBUG(...) ((void)((DEBUG_LEVEL) && (__VA_ARGS__, 0)))

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield

#include <SPI.h>
#ifndef NO_ETHERNET
  #include <Ethernet.h>
  #include <utility/w5100.h>
#endif NO_ETHERNET
#include <TimerOne.h>
#include <avr/wdt.h>
#include <EEPROM.h>

/** \brief MAC Address of ethernet shield
 *
 * A byte array containing the HEX equivalent of the MAC, NIC or Ethernet address.
 */
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

/** \brief API Server Address
 *
 * A CHAR array containing the FQDN or IP address of the desired server.
 */
//char server[] = API_SERVER_URL;    // name address for Google (using DNS)

/** \brief Ethernet Client Object
 *
 * Instantiate the Ethernet Client object for global use.
 */

#ifndef NO_ETHERNET
EthernetClient client;
#endif NO_ETHERNET

/** \brief APIs recieved status
 *
 * Integer value indicating remote server API status.
 * - -1 : is error
 * -  0 : is CLOSED
 * -  1 : is OPEN
 */
int SiteStatus;

/** \brief APIs recieved status
 *
 * record time of the last successful API read.
 */
static unsigned long lastHttpResponseTime;

#define api_url_maxlen  40
#define api_fname_maxlen  40
#define token_id_maxlen  40

typedef struct StoreStruct_t{

  unsigned int api_port;
  char api_url[api_url_maxlen + 1];
  char api_fname[api_fname_maxlen + 1];
  char api_token[token_id_maxlen + 1];
  unsigned int api_checkin_period;
  char version_of_program[4]; // it is the last variable of the
}
StoreStruct_t;

StoreStruct_t EEPROM_configuration = {0};

//------------------------------------------------------------------------------
/**
 * \brief Setup the Arduino Chip's feature for our use.
 *
 * After Arduino's kernel has booted initialize basic features for this
 * application, such as pins, Serial ports, Shields and objects with .begin.
 */
void setup() {

  // initialize pins
  pinMode(PIN_LED_YLW, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GRN, OUTPUT);
  pinMode(PIN_XRELAY,  OUTPUT);
  pinMode(PIN_SWTCH,    INPUT);
  digitalWrite(PIN_SWTCH, LOW);

  // Open serial communications and wait for port to open:
  IFDEBUG(Serial.begin(115200));
  loadConfig();

  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  while (Serial.read() >= 0)
  {
    // flush the buffer 
  }
  IFDEBUG(Serial.println(F("Started")));
  // start the Ethernet connection:

  LEDsOFF();
  Timer1.initialize(200000); // 20Hz to indicate an error.
  Timer1.attachInterrupt( BlinkYlwLED ); // attach the service routine here

#ifndef NO_ETHERNET
  if (Ethernet.begin(mac) == 0) {
    IFDEBUG(Serial.println(F("Failed to configure Ethernet using DHCP")));
    reboot();
  }
#endif NO_ETHERNET
  LEDsOFF();

#ifndef NO_ETHERNET
  // set retransmission period and attempts, before failure. Default is rather long
  W5100.setRetransmissionTime(0x07D0);
  W5100.setRetransmissionCount(3);

  // print your local IP address:
  IFDEBUG(Serial.print(F("My IP address: ")));
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    IFDEBUG(Serial.print(Ethernet.localIP()[thisByte], DEC));
    IFDEBUG(Serial.print(F(".")));
  }
  IFDEBUG(Serial.println());
#endif NO_ETHERNET

  // give the Ethernet shield a second to initialize:
  WaitWithDots(2);

  IFDEBUG(Serial.println());

  IFDEBUG(Serial.println(F("---------------------------------------------")));

#ifndef NO_ETHERNET
  if (EEPROM_configuration.api_checkin_period > 0) {
    checkAPIstatus();
  } 
  else {
#endif NO_ETHERNET
    if (digitalRead(PIN_SWTCH) == 1) {
      SiteStatus = 1;
    } else {
      SiteStatus = 0;
    }
#ifndef NO_ETHERNET
  }
#endif NO_ETHERNET

  delay(1000);

  IFDEBUG(Serial.print(F("PIN_SWTCH = ")));
  IFDEBUG(Serial.println(digitalRead(PIN_SWTCH)));

  if (digitalRead(PIN_SWTCH) == 1) {
    digitalWrite( PIN_LED_GRN, 1);
    digitalWrite( PIN_XRELAY, 1);
  } else {
    digitalWrite( PIN_LED_RED, 1);
    digitalWrite( PIN_XRELAY, 0);
  }

  IFDEBUG(Serial.println(F("Setup is Complete")));
  IFDEBUG(Serial.println(F("---------------------------------------------")));
}

//------------------------------------------------------------------------------
/**
 * \brief Main Loop the Arduino Chip
 *
 * This is called at the end of Arduino kernel's main loop before recycling.
 * And is where the user's is executed.
 */
void loop()
{
#ifndef NO_ETHERNET
  if ((long)(millis() - lastHttpResponseTime - (unsigned long)(EEPROM_configuration.api_checkin_period * 1000)) >= 0 ) {
    IFDEBUG(Serial.print(F("Periodic Check of API status.")));
    checkAPIstatus();
  }
  else {
    //IFDEBUG(Serial.println((long)(millis() - lastHttpResponseTime - (unsigned long)(EEPROM_configuration.api_checkin_period * 1000)), DEC));
  }
#endif NO_ETHERNET
  
//  IFDEBUG(Serial.print(F("Time: ")));
//  IFDEBUG(Serial.print(((millis() - lastHttpResponseTime)/1000)));
//  IFDEBUG(Serial.print(F(": ")));


  if (SiteStatus != digitalRead(PIN_SWTCH)) {
    IFDEBUG(Serial.print(F("PIN_SWTCH = ")));
    IFDEBUG(Serial.println(digitalRead(PIN_SWTCH)));
#ifndef NO_ETHERNET
    updateAPIstatus();
#endif NO_ETHERNET
  }
  else {
//   IFDEBUG(Serial.println(F("No need to update the API.")));
  }

  if (digitalRead(PIN_SWTCH) == 1) {
    digitalWrite( PIN_LED_GRN, 1);
    digitalWrite( PIN_XRELAY, 1);
  } else {
    digitalWrite( PIN_LED_RED, 1);
    digitalWrite( PIN_XRELAY, 0);
  }

  WaitWithDots(1);

  //IFDEBUG(Serial.println());

  if(Serial.available()) {
    parse_menu(Serial.read()); // get command from serial input
  }
}

//------------------------------------------------------------------------------
/**
 * \brief Delay printing dots
 *
 * \param[in] uint8_t number of seconds to delay
 *
 * Delays number of seconds, while printing "." once per second.
 */
void WaitWithDots(uint8_t n) {
  for (int count = 1; count <= n; count++) {
    IFDEBUG(Serial.print(F(".")));
    delay(1000);
    if(Serial.available()) {
      parse_menu(Serial.read()); // get command from serial input
    }    
  }
}
#ifndef NO_ETHERNET

//------------------------------------------------------------------------------
/**
 * \brief queries API for current status
 *
 * WGET's the API to update status
 */
int httpGET() {
  String request;
  String response;
  int result;
  long startOfHttpResponseTime;

  IFDEBUG(Serial.println(F("connecting get...")));

  LEDsOFF();
  Timer1.initialize(200000); // 20Hz to indicate an error.
  Timer1.attachInterrupt( BlinkYlwLED ); // attach the service routine here

  // if you get a connection, report back via serial:
  if (client.connect(EEPROM_configuration.api_url, EEPROM_configuration.api_port)) {
    IFDEBUG(Serial.println(F("connected get")));
    // Make a HTTP request:
    request  = String(F("GET ")) + String(EEPROM_configuration.api_fname) + String(F(" HTTP/1.0\n"));
    request += String(F("User-Agent: Wget/1.11.4\n"));
    request += String(F("Accept: */*\n"));
    request += String(F("Host: ")) + String(EEPROM_configuration.api_url) + String(F("\n"));
    request += String(F("Connection: close\n"));
    request += String(F("\n"));
    client.print(request);
    IFDEBUG(Serial.println(F("Sent Request:")));
    IFDEBUG(Serial.print(request));
    IFDEBUG(Serial.println(F("Waiting for Response to get.")));
    result = 0;
  }
  else {
    // if you didn't get a connection to the server:
    IFDEBUG(Serial.println(F("connection failed")));
    result = -5;
  }

  response = "";
  startOfHttpResponseTime = millis();
  while((client.connected()) && ((millis() - startOfHttpResponseTime) < HTTP_RESPONSE_TIMEOUT)) {
    // if there are incoming bytes available
    // from the server, read them and print them:
    if (client.available()) {
      char c = client.read();
      //read char by char HTTP request
      if (response.length() < 300) {
        //store characters to string
        response += c;
      }
      //IFDEBUG(Serial.print(c));
    }
  }
  IFDEBUG(Serial.println());
  IFDEBUG(Serial.println(F("RECIEVED RESPONSE:")));
  IFDEBUG(Serial.println(response));
  IFDEBUG(Serial.println());

  IFDEBUG(Serial.println(F("disconnecting.")));
  client.stop();
  LEDsOFF();

  int APIstatus = response.indexOf("200 OK");
  if (!result && APIstatus == 0) {
    result = -3;
    IFDEBUG(Serial.println(F("Did not find 200 OK in HTTP Header"))); //
  }
  else {
    IFDEBUG(Serial.println(F("Did find 200 OK in HTTP Header")));
    APIstatus = response.indexOf("openStatus");
  }

  if (!result && APIstatus == 0) {
    result = -2;
    IFDEBUG(Serial.println(F("Did not contain JSON openStatus tag")));
  }
  else {
    IFDEBUG(Serial.println(F("Did contain JSON openStatus tag"))); //
    APIstatus = response.indexOf("true");
  }

  if (!result && APIstatus > 0) {
    result = 1;
    IFDEBUG(Serial.println(F("Found openStatus = OPEN")));
  }
  else {
    IFDEBUG(Serial.println(F("Found openStatus != true")));
    APIstatus = response.indexOf("false");
  }
  
  if (!result) {
    if (APIstatus > 0) {
      result = 0;
      IFDEBUG(Serial.println(F("Detected: JSON response for openStatus = CLOSED")));
    }
    else {
      result = -4;
      IFDEBUG(Serial.println(F("Detected: improper JSON response for openStatus"))); //
    }
  }

  IFDEBUG(Serial.print(F("Result code = ")));
  IFDEBUG(Serial.println(result));
  return result;
}
#endif NO_ETHERNET

#ifndef NO_ETHERNET
//------------------------------------------------------------------------------
/**
 * \brief Update API with current status
 *
 * \param[in] String, either "true" or "false"
 *
 * Sends JSON formated message about status with token to API to update status
 */
int httpPOST(String state) {
  String post, json;
  String response;
  int result;
  long startOfHttpResponseTime;

  IFDEBUG(Serial.println(F("connecting post...")));

  LEDsOFF();
  Timer1.initialize(200000); // 20Hz to indicate an error.
  Timer1.attachInterrupt( BlinkYlwLED ); // attach the service routine here

  // if you get a connection, report back via serial:
  if (client.connect(EEPROM_configuration.api_url, EEPROM_configuration.api_port)) {
    IFDEBUG(Serial.println(F("connected post")));
    // Make a HTTP request:
    json =  String(F("{\"token\": \"")) + String(EEPROM_configuration.api_token) + String(F("\", \"openStatus\": ")) + state + String(F("}"));

    post  = String(F("POST ")) + String(EEPROM_configuration.api_fname) + String(F(" HTTP/1.1\n"));
    post += String(F("Host: ")) + String(EEPROM_configuration.api_url) + String(F("\n"));
    post += String(F("Connection: close\n"));
    post += String(F("Content-Type: application/json; charset=utf8\n"));
    post += String(F("Content-Length: "));
    post += json.length() + String(F("\n"));
    post += String(F("\n"));
    post += json;
    client.print(post);
    IFDEBUG(Serial.println(F("Sent post:")));
    IFDEBUG(Serial.print(post));
    IFDEBUG(Serial.println());
    IFDEBUG(Serial.println(F("Waiting for Response from post.")));
    result = 0;
  }
  else {
    // kf you didn't get a connection to the server:
    IFDEBUG(Serial.println(F("connection failed")));
    result = -5;
  }

  response = "";
  startOfHttpResponseTime = millis();
  while((client.connected()) && ((millis() - startOfHttpResponseTime) < HTTP_RESPONSE_TIMEOUT)) {
    // if there are incoming bytes available
    // from the server, read them and print them:
    if (client.available()) {
      char c = client.read();
      //read char by char HTTP request
      if (response.length() < 300) {
        //store characters to string
        response += c;
      }
      //IFDEBUG(Serial.print(c));
    }
  }
  IFDEBUG(Serial.println());
  IFDEBUG(Serial.println(F("RECIEVED RESPONSE:")));
  IFDEBUG(Serial.println(response));
  IFDEBUG(Serial.println());

  IFDEBUG(Serial.println(F("disconnecting post.")));
  client.stop();
  LEDsOFF();

  int APIstatus = response.indexOf(" 201 ");
  if (!result && APIstatus > 0) {
    result = 201;
    IFDEBUG(Serial.println(F("Success: Response contained expected 201 OK in HTTP Header")));
  }
  else {
    APIstatus = response.indexOf(" 400 ");
  }

  if (!result && APIstatus > 0) {
    result = 400;
    IFDEBUG(Serial.println(F("Error: Response contained un-expected 400 OK in HTTP Header")));
  }
  else {
    APIstatus = response.indexOf(" 403 ");
  }

  if (!result) {
    if (APIstatus > 0) {
      result = 403;
      IFDEBUG(Serial.println(F("Error: Response contained un-expected 403 OK in HTTP Header")));
    }
    else {
      result = -1;
      IFDEBUG(Serial.println(F("Error: Response contained unknown HTTP Header")));
    }
  }

  IFDEBUG(Serial.print(F("Result code = ")));
  IFDEBUG(Serial.println(result));
  return result;
}
#endif NO_ETHERNET

//------------------------------------------------------------------------------
/**
 * \brief Toggle Green LED
 *
 * attached to ISR to toggle Green LED
 */
void BlinkGRNLED()
{
    // Toggle LED
    digitalWrite( PIN_LED_GRN, digitalRead( PIN_LED_GRN ) ^ 1 );
}

//------------------------------------------------------------------------------
/**
 * \brief Toggle Yellow LED
 *
 * attached to ISR to toggle Yellow LED
 */
void BlinkYlwLED()
{
    // Toggle LED
    digitalWrite( PIN_LED_YLW, digitalRead( PIN_LED_YLW ) ^ 1 );
}

//------------------------------------------------------------------------------
/**
 * \brief Toggle Red LED
 *
 * attached to ISR to toggle Red LED
 */
void BlinkRedLED()
{
    // Toggle LED
    digitalWrite( PIN_LED_RED, digitalRead( PIN_LED_RED ) ^ 1 );
}

//------------------------------------------------------------------------------
/**
 * \brief All LEDs OFF
 *
 * Disable Timer1 interrupt and set state of all LEDs to OFF.
 */
void LEDsOFF()
{
  Timer1.detachInterrupt();
  digitalWrite( PIN_LED_GRN, 0);
  digitalWrite( PIN_LED_YLW, 0);
  digitalWrite( PIN_LED_RED, 0);
}


//------------------------------------------------------------------------------
/**
 * \brief Reboot
 *
 * Reboot using the Hardware WatchDog
 */
void reboot()
{
  LEDsOFF();
  Timer1.initialize(100000); // 10Hz to indicate an error.
  Timer1.attachInterrupt( BlinkRedLED ); // attach the service routine here

  Serial.println("Using the WatchDog to Soft Reset");
  wdt_enable(WDTO_2S); // provides a Soft Reset
  for(;;)
  {
    // wait for the end, and hopefully next reboot will work!
  }
}

//------------------------------------------------------------------------------
/**
 * \brief Check the APIs current status
 *
 * Call the httpGET with all the retries and failsafes
 */
void checkAPIstatus()
{
  int RetryCount;

  SiteStatus = -1;
  RetryCount = 1;
  while (SiteStatus < 0) {
    IFDEBUG(Serial.println(F("Quering initial status from website.")));
    SiteStatus = httpGET();
    if (SiteStatus == 0) {
      IFDEBUG(Serial.println(F("Status is Closed.")));
      lastHttpResponseTime = millis();
    }
    else if (SiteStatus == 1) {
      IFDEBUG(Serial.println(F("Status is Open.")));
      lastHttpResponseTime = millis();
    }
    else {
     if (RetryCount > 120) {
        reboot();
      }
      else if (RetryCount % 5) {
        IFDEBUG(Serial.println(F("Status is Uknown. Delay to retry.")));
        LEDsOFF();
        Timer1.initialize(2000000); // 2Hz to indicate an error.
        Timer1.attachInterrupt( BlinkRedLED ); // attach the service routine here
        WaitWithDots(5);
      } else {
        // reboot
        IFDEBUG(Serial.println(F("Longer Delay before retrying.")));
        LEDsOFF();
        Timer1.initialize(1000000); // 1Hz to indicate an error.
        Timer1.attachInterrupt( BlinkRedLED ); // attach the service routine here
        WaitWithDots(30);
      }
      IFDEBUG(Serial.println());
    }
    RetryCount++;
  }
}

//------------------------------------------------------------------------------
/**
 * \brief update the APIs current status
 *
 * Call the httpPOST with all the retries and failsafes
 */
void updateAPIstatus()
{
  String message;

  IFDEBUG(Serial.println(F("Need to update the API.")));
  if (digitalRead(PIN_SWTCH) == 0) {
    message = String(F("false"));
  } else {
    message = String(F("true"));
  }

  for (int count = 1; count <= 5; count++) {
    if (httpPOST(message) == 201) {
      IFDEBUG(Serial.println(F("Update Successful")));
      SiteStatus = digitalRead(PIN_SWTCH);
      count = 10;
      lastHttpResponseTime = millis();
    }
    else {
      IFDEBUG(Serial.println(F("Update Failed")));
      IFDEBUG(Serial.println(F("Update Failed. Delay to retry.")));
      LEDsOFF();
      Timer1.initialize(1000000); // 1Hz to indicate an error.
      Timer1.attachInterrupt( BlinkYlwLED ); // attach the service routine here
      WaitWithDots(5);
    }
  }
}

//------------------------------------------------------------------------------
/**
 * \brief Display the current configuration
 *
 * Send the EEPROM's configuration to the Serial port
 */
void show_info()
{
  IFDEBUG(Serial.print(F("(Called 'show_info' from menu: ")));
  IFDEBUG(Serial.println(')'));

  IFDEBUG(Serial.print(F("URL: \"")));
  IFDEBUG(Serial.print(EEPROM_configuration.api_url));
  IFDEBUG(Serial.println(F("\"")));

  IFDEBUG(Serial.print(F("FName: \"")));
  IFDEBUG(Serial.print(EEPROM_configuration.api_fname));
  IFDEBUG(Serial.println(F("\"")));

  IFDEBUG(Serial.print(F("Token: \"")));
  IFDEBUG(Serial.print(EEPROM_configuration.api_token));
  IFDEBUG(Serial.println(F("\"")));

  IFDEBUG(Serial.print(F("Port: \"")));
  IFDEBUG(Serial.print(EEPROM_configuration.api_port, DEC));
  IFDEBUG(Serial.println(F("\"")));

  IFDEBUG(Serial.print(F("Checkin Period: \"")));
  IFDEBUG(Serial.print(EEPROM_configuration.api_checkin_period, DEC));
  IFDEBUG(Serial.println(F("\"")));
}

//------------------------------------------------------------------------------
/**
 * \brief Load the current configuration
 *
 * Load the EEPROM's configuration into operational RAM.
 */
 void loadConfig() {
  // To make sure there are EEPROM_configuration, and they are YOURS!
  // If nothing is found it will use the default EEPROM_configuration.
  if ( //EEPROM.read(CONFIG_START + sizeof(EEPROM_configuration) - 1) == EEPROM_configuration.version_of_program[3] // this is '\0'
    EEPROM.read(CONFIG_START + sizeof(EEPROM_configuration) - 2) == EEPROM_configuration.version_of_program[2] &&
    EEPROM.read(CONFIG_START + sizeof(EEPROM_configuration) - 3) == EEPROM_configuration.version_of_program[1] &&
    EEPROM.read(CONFIG_START + sizeof(EEPROM_configuration) - 4) == EEPROM_configuration.version_of_program[0]) { // reads EEPROM_configuration from EEPROM
    IFDEBUG(Serial.println("EEPROM being loaded."));
    for (unsigned int t=0; t<sizeof(EEPROM_configuration); t++)
    *((char*)&EEPROM_configuration + t) = EEPROM.read(CONFIG_START + t);
  } else {
    // EEPROM_configuration aren't valid! will overwrite with default EEPROM_configuration
    IFDEBUG(Serial.println("EEPROM being defaulted."));
    saveConfig();
  }
  show_info();
}

//------------------------------------------------------------------------------
/**
 * \brief Save the current configuration
 *
 * Save the EEPROM's configuration from operational RAM.
 */
 void saveConfig() {
  Serial.println("EEPROM being saved.");
  for (unsigned int t=0; t<sizeof(EEPROM_configuration); t++)
  { // writes to EEPROM
    EEPROM.write(CONFIG_START + t, *((char*)&EEPROM_configuration + t));
    // and verifies the data
    if (EEPROM.read(CONFIG_START + t) != *((char*)&EEPROM_configuration + t)) {
      // error writing to EEPROM
    }
  }
}

//------------------------------------------------------------------------------
/**
 * \brief Parse the Serial Input
 *
 * Parse the command recieved and subsequently retrieve additional configuration
 * input from the serial interface and save them into operational ram and EEPROM.
 */
 void parse_menu(byte key_command) {
  char resp;
  int respValue;

  Serial.print(F("Received command: "));
  Serial.write(key_command);
  Serial.println(F(" "));
  while (Serial.read() >= 0)
  {
    // flush the buffer 
  }
  
  if(key_command == 'h') {
    Serial.println(F("-----------------------------"));
    Serial.println(F("Current saved settings"));
    show_info();
    Serial.println(F("-----------------------------"));
    Serial.println(F("Available commands..."));
    Serial.println(F("-----------------------------"));
    Serial.println(F("[u] Set api URL"));
    Serial.println(F("[f] Set api fname   "));
    Serial.println(F("[p] Set api port    "));
    Serial.println(F("[t] Set api token   "));
    Serial.println(F("[c] Set api checkin period"));
    Serial.println(F("-----------------------------"));
    Serial.println(F("Enter above key:"));
    Serial.println(F("-----------------------------"));
  }
  else if(key_command == 'u') {
    Serial.println(F("Enter URL of api?"));
    String response = getStrFromSerial();
    if (response.length() > 0 ) {
      response.toCharArray(EEPROM_configuration.api_url,(response.length() + 1));
      saveConfig();
    }
  }
  else if(key_command == 'f') {
    Serial.println(F("Enter filename to get from api?"));
    String response = getStrFromSerial();
    if (response.length() > 0 ) {
      response.toCharArray(EEPROM_configuration.api_fname,(response.length() + 1));
      saveConfig();
    }
  }
  else if(key_command == 'p') {
    Serial.println(F("Enter port of api?"));
    String response = getStrFromSerial();
    if (response.length() > 0 ) {
      EEPROM_configuration.api_port = response.toInt();
      saveConfig();
    }
  }
  else if(key_command == 't') {
    Serial.println(F("Enter token of api?"));
    String response = getStrFromSerial();
    if (response.length() > 0 ) {
      response.toCharArray(EEPROM_configuration.api_token,(response.length() + 1));
      saveConfig();
    }
  }
  else if(key_command == 'c') {
    Serial.println(F("Enter period for checkin? (0 is disabled)"));
    String response = getStrFromSerial();
    if (response.length() > 0 ) {
      EEPROM_configuration.api_checkin_period = response.toInt();
      saveConfig();
    }
  }

  while (Serial.read() >= 0)
   ; // clear read buffer
}

//------------------------------------------------------------------------------
/**
 * \brief Read a String
 *
 * A helper to better construct a string from the recieved input of Serial,
 * along with a timeout, as not to indefinitely block.
 */
 String getStrFromSerial()
{
  Serial.println(F("(Make sure to enable Carriage Returns)"));
  String str;
  Serial.setTimeout(5000) ;
  str = Serial.readStringUntil('\r');
  if (str.length()) {
    Serial.print(F("You entered \""));
    Serial.print(str);
    Serial.println(F("\""));
  }
  else {
    Serial.println(F("Timed out."));
  }
  Serial.setTimeout(1000);
  return str;
}