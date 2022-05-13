#include <WiFi.h>
#include "time.h"
#include <ESP_Mail_Client.h>

/* The WIFI in credentials */
#define WIFI_SSID "Dell5410"
#define WIFI_PASSWORD "HamidKhan"

/* For getTime() */
String pk_time = "";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 14400;
const int   daylightOffset_sec = 3600;

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The sign in credentials of sender*/
#define SENDER_EMAIL "hamidmuzaffar49@gmail.com"
#define SENDER_PWD "htarrzjejqakaxfi"

/* Receiver's email*/
#define RECEIVER_EMAIL "hamidmuzaffar218@gmail.com"

/* The SMTP Session object*/
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

void setup() {
  Serial.begin(115200);
  /* Connection with WIFI */
  wifiConnection();
  /* Setting Time Credentials */
  getTime();
  /* Sending Mail */
  sendEmail();
}

void loop() {

}

void wifiConnection() {
  Serial.println();
  Serial.print("Connecting to WIFI");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void getTime() {
  struct tm timeinfo;
  bool time_status = true;
  do {
    if (!time_status)
      Serial.println("Failed to obtain time");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    time_status = getLocalTime(&timeinfo);
    delay(2000);
  } while (!time_status);
  Serial.println("Succeed to obtain time");
  Serial.println("Time variables");
  char t_string[36];
  strftime(t_string, 36, "%A, %B %d %Y %I:%M:%S", &timeinfo);
  pk_time = String(t_string);
  Serial.println(pk_time);
  Serial.println();
}

void sendEmail() {
  smtp.debug(1);
  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the session config data */
  ESP_Mail_Session session;

  /* Set the session config */
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = SENDER_EMAIL;
  session.login.password = SENDER_PWD;

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = "ESP-Board";
  message.sender.email = SENDER_EMAIL;
  message.subject = "ESP Test Email";
  message.addRecipient("Hamid", RECEIVER_EMAIL);
  
  /*Send HTML message*/
  String htmlMsg = "Hello World - Sent from ESP board<br><br>Time : ";
  htmlMsg.concat(pk_time);
  message.html.content = htmlMsg.c_str();
  message.html.charSet = "utf-8";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_qp;

  SMTP_Attachment att;

  /** Set the attachment info e.g. 
   * file name, MIME type, file path, file storage type,
   * transfer encoding and content encoding
  */
  att.descr.filename = "image.jpg";
  att.descr.mime = "image/jpg"; //binary data
  att.file.path = "/image.jpg";
  att.file.storage_type = esp_mail_file_storage_type_flash;
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /* Add attachment to the message */
  message.addAttachment(att);

  /* Connect to server with the session config */
  while (!smtp.connect(&session)) {}

  /* Start sending Email and close the session */
  while (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status) {
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()) {
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++) {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);
      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }
}
