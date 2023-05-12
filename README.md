# GSM_SMS_GPRS_Combo
This project uses an Arduino board and a SIM800L GSM module to forward received SMS messages to a webhook. 
Setup involves connecting the SIM800L to the Arduino, configuring the APN and webhook URL, and uploading the code. 
The system continuously listens for SMS messages and forwards them via HTTP GET requests. 
Limitations include lack of support for multipart SMS messages and potential communication issues. 
Future improvements could include multipart SMS support, improved reliability, SMS confirmations, and robust error handling.
