#include <SoftwareSerial.h>

String receivedData = "";
String receivedMessage = "";
String apn = "Safaricom";
String serverIP = "webhook.site/79b42cc5-9365-472f-86ac-a6dd7b8d84ec";
bool messageStarted = false;

// Create software serial object to communicate with SIM800L
SoftwareSerial mySerial(6, 5); // SIM800L Tx & Rx is connected to Arduino #11 & #10

void setup() {
  // Begin serial communication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(9600);

  // Begin serial communication with Arduino and SIM800L
  mySerial.begin(9600);

  Serial.println("Initializing...");
  delay(1000);

  // Test handshake with the SIM800L module
  mySerial.println("AT");
  waitForResponse("OK", 2000);

  // Check signal quality
  mySerial.println("AT+CSQ");
  waitForResponse("OK", 2000);

  // Configure the module to use TEXT mode for SMS
  mySerial.println("AT+CMGF=1");
  waitForResponse("OK", 2000);

  // Configure how new SMS messages should be handled
  mySerial.println("AT+CNMI=1,2,0,0,0");
  waitForResponse("OK", 2000);
}

void loop() {
  // Read incoming serial data and update the received message
  readSerialAndUpdateReceivedMessage();

  // If a new message was received, send it to the specified URL
  if (receivedMessage != "") {
    sendToURL(receivedMessage);
    receivedMessage = "";
  }
}

// Read incoming serial data and update the received message accordingly
void readSerialAndUpdateReceivedMessage() {
  while (mySerial.available()) {
    char inChar = (char)mySerial.read();
    Serial.print(inChar);

    if (inChar == '\n') {
      Serial.print("receivedData: ");
      Serial.println(receivedData);

      if (receivedData.startsWith("+CMT:")) {
        // Extract relevant data (sender, timestamp, and message content)
        // from the receivedData string

        int firstQuoteIndex = receivedData.indexOf('"');
        int secondQuoteIndex = receivedData.indexOf('"', firstQuoteIndex + 1);
        int thirdQuoteIndex = receivedData.indexOf('"', secondQuoteIndex + 1);
        int fourthQuoteIndex = receivedData.indexOf('"', thirdQuoteIndex + 1);

        receivedMessage = receivedData.substring(fourthQuoteIndex + 1);
        receivedMessage.trim();

        // If the message starts and ends with quotes, extract the content
        int msgStart = receivedMessage.indexOf('"');
        int msgEnd = receivedMessage.lastIndexOf('"');
        if (msgStart != -1 && msgEnd != -1) {
          receivedMessage = receivedMessage.substring(msgStart + 1, msgEnd);
        }

        Serial.print("Received message: ");
        Serial.println(receivedMessage);

        // Read the next line for message content
        if (mySerial.available()) {
          char msgChar;
          while ((msgChar = (char)mySerial.read()) != '\n' && msgChar != -1) {
            receivedMessage += msgChar;
          }
        }

        Serial.print("Received message (updated): ");
        Serial.println(receivedMessage);
      }
      receivedData = "";
    } else {
      receivedData += inChar;
    }
  }
}

// Extract the content of the SMS message
String parseMessage(String data) {
  int startIndex = data.indexOf("\r\n") + 2;
  int endIndex = data.lastIndexOf("\r\n");
  if (startIndex >= 0 && endIndex > startIndex) {
    return data.substring(startIndex, endIndex);
  }
  return "";
}

// Send the received message to a specified URL
void sendToURL(String message) {
  if (message != "") {
    // Set up GPRS and HTTP connections
    setupGPRSAndHTTP();

    // Prepare the URL with the message parameter
    String url = "http://webhook.site/79b42cc5-9365-472f-86ac-a6dd7b8d84ec?message=" + urlEncode(message);

    // Set the URL for the HTTP request
    mySerial.print("AT+HTTPPARA=\"URL\",\"");
    mySerial.print(url);
    mySerial.println("\"");

    // Send the HTTP request if the URL was set successfully
    if (waitForResponse("OK", 1000)) {
      Serial.println("URL set successfully.");
      mySerial.println("AT+HTTPACTION=0");

      if (waitForResponse("+HTTPACTION: 0,200", 10000)) {
        Serial.println("HTTP request sent successfully.");
      } else {
        Serial.println("Error sending HTTP request.");
      }
    } else {
      Serial.println("Error setting URL.");
    }

    // Terminate the HTTP connection and close the GPRS connection
    mySerial.println("AT+HTTPTERM");
    if (waitForResponse("OK", 1000)) {
      Serial.println("HTTP terminated successfully.");
    } else {
      Serial.println("Error terminating HTTP connection.");
    }

    mySerial.println("AT+SAPBR=0,1");
    if (waitForResponse("OK", 1000)) {
      Serial.println("GPRS connection closed successfully.");
    } else {
      Serial.println("Error closing GPRS connection.");
    }
  }
}

// Set up the GPRS and HTTP connections for sending the received message
void setupGPRSAndHTTP() {
  // Set the APN
  mySerial.println("AT+SAPBR=3,1,\"APN\",\"Safaricom\"");
  if (waitForResponse("OK", 1000)) {
    Serial.println("APN set successfully.");
  }

  // Open the GPRS connection
  mySerial.println("AT+SAPBR=1,1");
  if (waitForResponse("OK", 30000)) {
    Serial.println("GPRS connection opened successfully.");
  } else {
    Serial.println("Error opening GPRS connection.");
  }

  // Initialize HTTP
  mySerial.println("AT+HTTPINIT");
  if (waitForResponse("OK", 1000)) {
    Serial.println("HTTP initialized successfully.");
  } else {
    Serial.println("Error initializing HTTP.");
  }
}

// Close the HTTP and GPRS connections
void closeHTTPAndGPRS() {
  // Terminate the HTTP connection
  mySerial.println("AT+HTTPTERM");
  waitForResponse("OK", 1000);

  // Close the GPRS connection
  mySerial.println("AT+SAPBR=0,1");
  waitForResponse("OK", 1000);
}

// URL encode a string for use in a query string parameter
String urlEncode(String str) {
  String encodedStr = "";
  char c;
  for (int i = 0; i < str.length(); i++) {
    c = str[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encodedStr += c;
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", c);
      encodedStr += buf;
    }
  }
  return encodedStr;
}

// Wait for a specific response from the SIM800L module within a specified timeout
bool waitForResponse(const char* response, unsigned long timeout) {
  unsigned long startTime = millis();
  String receivedData = "";

  // Check for the desired response within the timeout period
  while (millis() - startTime < timeout) {
    if (mySerial.available()) {
      char c = mySerial.read();
      receivedData += c;
      if (receivedData.endsWith(response)) {
        Serial.print(receivedData);
        return true;
      }
    }
  }
  Serial.print(receivedData);
  return false;
}


