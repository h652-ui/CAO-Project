#include "esp_camera.h"
#include <WiFi.h>
#include "time.h"
#include <FS.h>
#include <SPIFFS.h>
#include <ESP_Mail_Client.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <BlynkSimpleEsp32.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>

/*Define Camera Pins*/

#define CAMERA_MODEL_AI_THINKER

#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#else
#error "Camera model not selected"
#endif

/*Path and name of file to write*/
#define FILE_PHOTO "/photo.jpg"

/*Pins Connection*/
#define p_btn 2
#define lock 13
#define btn 14
#define IR_pin 15
//#define buzzer 16

/* The WIFI in credentials */
#define WIFI_SSID "Dell5410"
#define WIFI_PASSWORD "HamidKhan"
char authentication[] = "xrPe4GpwLBBZEkC-ctNhc_mDVaeZ04UX";       //Auth Code sent by Blynk
String url = "";
String local_IP = "";

/* For getTime() */
String pk_time = "";
const char* ntpServer = "pk.pool.ntp.org";
const long  gmtOffset_sec = 14400;
const int   daylightOffset_sec = 3600;

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The sign in credentials of sender*/
#define SENDER_EMAIL "hamidmuzaffar49@gmail.com"
#define SENDER_PWD "htarrzjejqakaxfi"

/* Receiver's email*/
#define RECEIVER_EMAIL "hamidmuzaffar218@gmail.com"

/*Store Number in EEPROM to make unique name of picture*/
// define the number of bytes you want to access
#define EEPROM_SIZE 1
int pictureNumber = 0;
String storage_path = "";
String RTDB_path_url = "";
String RTDB_path_status = "";
String RTDB_path_date = "";

/*Firebase Credentials*/
// Insert Firebase project API Key
#define API_KEY "AIzaSyBJeo4dPJ9xDmhqrKV0uYsb48ZuT63GuQ4"
// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "hamidmuzaffar49@gmail.com"
#define USER_PASSWORD "HamidKhan"
// Insert Firebase storage bucket ID e.g bucket-name.appspot.com
#define STORAGE_BUCKET_ID "esp-32-c099b.appspot.com"
// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://esp-32-c099b-default-rtdb.asia-southeast1.firebasedatabase.app/"
/*--------------------*/

//Define Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;

/* The SMTP Session object*/
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

/*Web Server Credentials*/

const char* PARAM_INPUT_1 = "output";
const char* PARAM_INPUT_2 = "state";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Bell System Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
        html {
            position: absolute;
            top: 0px;
            bottom: 0px; 
            right : 0px;
            left : 0px;
            font-family: Arial;
            display: inline-block;
            text-align: center;
            background-image: linear-gradient(to bottom right, rgb(13, 13, 35), rgb(52, 52, 118), rgb(157, 0, 255));
            background-repeat: no-repeat;
            background-size: cover;
            background-position: center center;
            color: white;
        }

        h2 {
            font-size: 3.0rem;
        }

        p {
            font-size: 3.0rem;
        }

        body {
            max-width: 600px;
            margin: 0px auto;
            padding-bottom: 25px;
        }

        .switch {
            position: relative;
            display: inline-block;
            width: 120px;
            height: 68px;
        }

        .switch input {
            display: none
        }

        .slider {
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: red;
            border-radius: 6px;
            transition: .4s;
        }

        .slider:before {
            position: absolute;
            content: "";
            height: 52px;
            width: 52px;
            left: 8px;
            bottom: 8px;
            background-color: #fff;
            -webkit-transition: .4s;
            transition: .4s;
            border-radius: 3px
        }

        input:checked+.slider {
            background-color: Green;
        }

        input:checked+.slider:before {
            -webkit-transform: translateX(52px);
            -ms-transform: translateX(52px);
            transform: translateX(52px);
        }
    </style>
</head>
<body>
  <h2>Bell System Web Server</h2>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/update?output=" + element.id + "&state=1", true);
        xhr.send();
        setTimeout(function () {
            element.checked =false;
            xhr.open("GET", "/update?output=" + element.id + "&state=0", true);
            xhr.send();
        }, 2000);
    }
</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    buttons += "<h4>Open The Door</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\""+String(lock)+"\"><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

void setup() {
  Serial.begin(115200);
  /* Connection with WIFI */
  wifiConnection();
  /* Connection with Blynk */
  Blynk.begin(authentication, WIFI_SSID, WIFI_PASSWORD);

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }

  /* Setting Camera Configuration */
  cameraConfiguration();
  /* Setting up Time Credentials */
  setTimeESP();
  /* Setting up Firebase */
  firebaseConfiguration();
  /*interrupt and lock pin setup*/
  pinMode(IR_pin, INPUT);
  pinMode(lock, OUTPUT);
  digitalWrite(lock, LOW);
  pinMode(btn, INPUT);
  digitalWrite(btn, LOW);
  pinMode(4,OUTPUT);
  pinMode(p_btn, INPUT_PULLUP);
//  pinMode(buzzer,OUTPUT);
//  digitalWrite(buzzer, LOW);
  /*Starting Web Server*/
  start_web_server();
  /*Sending mail after setup is completete*/
  sendEmail("Setting Up Everything. ESP is working now");
}

void loop() {
  /* Connection to the blynk */
  Blynk.run();
  if (digitalRead(btn) == HIGH) {
    change_status();
    digitalWrite(lock, HIGH);
    delay(1000);
    digitalWrite(lock, LOW);
    delay(1000);
  }
  if (digitalRead(p_btn) == LOW) {
    Serial.println("The button is pressed");
    digitalWrite(lock, HIGH);
    delay(1000);
    digitalWrite(lock, LOW);
    delay(1000);
  }
  if (digitalRead(IR_pin) == LOW) {
    /* Capture Photo and saved to SPIFF */
    capturePhotoSaveSpiffs();
    /* Getting Time Credentials */
    getTime();
    /* Sending to Firebase */
    storeIntoFirebase();
    /* Sending Mail */
    sendEmail("");
    /*Buzzer*/
//    digitalWrite(buzzer, HIGH);
//    delay(3000);
//    digitalWrite(buzzer, LOW);
  }
  if ((WiFi.status() != WL_CONNECTED)) 
  {
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    if ((WiFi.status() == WL_CONNECTED))
    {
      Serial.print("Camera Ready! Use 'http://");
      Serial.print(WiFi.localIP());
      Serial.println("' to connect");
    }
    delay(3000);
  }
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
  local_IP = String() + WiFi.localIP()[0] + "." + WiFi.localIP()[1] + "." + WiFi.localIP()[2] + "." +WiFi.localIP()[3];;
  Serial.println(WiFi.localIP());
  Serial.println();
}

void cameraConfiguration() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);
}

void firebaseConfiguration() {
  // Assign the api key
  configF.api_key = API_KEY;
  /* Assign the RTDB URL (required) */
  configF.database_url = DATABASE_URL;
  //Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  //Assign the callback function for the long running token generation task
  configF.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  Firebase.begin(&configF, &auth);
  Firebase.reconnectWiFi(true);
}

//Specify Paths to the database
void storage_RTDB_Path() {
  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0) + 1;
  storage_path = "/data/photo" + String(pictureNumber) + ".jpg";
  RTDB_path_url = "record" + String(pictureNumber) + "/url";
  RTDB_path_status = "record" + String(pictureNumber) + "/status";
  RTDB_path_date = "record" + String(pictureNumber) + "/date";
}

void storeIntoFirebase() {
  bool taskCompleted = false;
  storage_RTDB_Path();
  while (Firebase.ready() && !taskCompleted) {
    taskCompleted = true;
    Serial.print("Uploading picture... ");
    //MIME type should be valid to avoid the download problem.
    //The file systems for flash and SD/SDMMC can be changed in FirebaseFS.h.
    if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID /* Firebase Storage bucket id */, FILE_PHOTO /* path to local file */, mem_storage_type_flash /* memory storage type, mem_storage_type_flash and mem_storage_type_sd */, storage_path /* path of remote file stored in the bucket */, "image/jpeg" /* mime type */)) {
      Serial.printf("\nDownload URL: %s\n", fbdo.downloadURL().c_str());
      url = (String)fbdo.downloadURL();
      String l_status = "False";
      if (Firebase.RTDB.setString(&fbdo, RTDB_path_url, fbdo.downloadURL().c_str()) && Firebase.RTDB.setString(&fbdo, RTDB_path_status, l_status.c_str()) && Firebase.RTDB.setString(&fbdo, RTDB_path_date, pk_time.c_str())) {
        Serial.println("PASSED");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
        Blynk.setProperty(V1, "urls", url);
        EEPROM.write(1, pictureNumber);
        EEPROM.commit();
      } else {
        Serial.println("FAILED");
      }
    }
    else {
      taskCompleted = false;
      Serial.println(fbdo.errorReason());
    }
  }
}

// Check if photo capture was successful
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( FILE_PHOTO );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}

// Capture Photo and Save it to SPIFFS
void capturePhotoSaveSpiffs() {
  camera_fb_t * fb = NULL; // pointer
  bool ok = 0; // Boolean indicating if the picture has been taken correctly
  // Take a photo with the camera
  Serial.println("Taking a photo...");
  digitalWrite(4, HIGH);
  delay(200);
  fb = esp_camera_fb_get();
  digitalWrite(4, LOW);
  while (!fb) {
    Serial.println("Camera capture failed");
    digitalWrite(4, HIGH);
    delay(200);
    fb = esp_camera_fb_get();
    digitalWrite(4, LOW);
    delay(1000);    
  }

  // Photo file name
  Serial.printf("Picture file name: %s\n", FILE_PHOTO);
  File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);

  // Insert the data in the photo file
  while (!file) {
    Serial.println("Failed to open file in writing mode");
    delay(1000);
    file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
  }

  file.write(fb->buf, fb->len); // payload (image), payload length
  // check if file has been correctly saved in SPIFFS
  ok = checkPhoto(SPIFFS);
  while (!ok) {
    delay(1000);
    file.write(fb->buf, fb->len);
    ok = checkPhoto(SPIFFS);
  }
  Serial.print("The picture has been saved in ");
  Serial.print(FILE_PHOTO);
  Serial.print(" - Size: ");
  Serial.print(file.size());
  Serial.println(" bytes");
  // Close the file
  file.close();
}

void setTimeESP() {
  struct tm timeinfo;
  bool time_status;
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  time_status = getLocalTime(&timeinfo);
  if (!time_status) {
    Serial.println("Failed to obtain time");
    pk_time = "Default Time";
    return;
//    ESP.restart();
  }
  Serial.println("Succeed to obtain time");
  char t_string[36];
  strftime(t_string, 36, "%A, %B %d %Y %I:%M:%S", &timeinfo);
  pk_time = String(t_string);
  Serial.println(pk_time);
  Serial.println();
}

void getTime() {
  struct tm timeinfo;
  char t_string[36];
  if (getLocalTime(&timeinfo)) {
    strftime(t_string, 36, "%A, %B %d %Y %I:%M:%S", &timeinfo);
    pk_time = String(t_string);
    Serial.println(pk_time);
    Serial.println();
  }else{
     pk_time = "Default Time";
    return;
  }
}

void start_web_server(){
  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
      change_status();
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    Serial.print("GPIO: ");
    Serial.print(inputMessage1);
    Serial.print(" - Set to: ");
    Serial.println(inputMessage2);
    request->send(200, "text/plain", "OK");
  });

  // Start server
  server.begin();
}

void sendEmail(String msg) {
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
  String htmlMsg = "";
  if (msg == "") {
    htmlMsg = "Someone Arrived at the Door...<br>Time : <br>Use the following link to open the door...<br><a href=\""+String(local_IP)+"\">"+String(local_IP)+"</a><br><br>Or Use Blynk App<br><br>";
  } else {
    htmlMsg = msg;
  }
  htmlMsg.concat(pk_time);
  message.html.content = htmlMsg.c_str();
  message.html.charSet = "utf-8";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_qp;

  if (msg == "") {
    SMTP_Attachment att;
    /** Set the attachment info e.g.
       file name, MIME type, file path, file storage type,
       transfer encoding and content encoding
    */
    att.descr.filename = "photo.jpg";
    att.descr.mime = "image/jpg"; //binary data
    att.file.path = "/photo.jpg";
    att.file.storage_type = esp_mail_file_storage_type_flash;
    att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

    /* Add attachment to the message */
    message.addAttachment(att);
  }

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
    SPIFFS.remove(FILE_PHOTO);
  }
}

void change_status() {
  bool taskCompleted = false;
  String l_status = "True";
  while (Firebase.ready() && !taskCompleted) {
    taskCompleted = true;
    if (Firebase.RTDB.setString(&fbdo, RTDB_path_status, l_status.c_str())) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      EEPROM.write(0, pictureNumber);
      EEPROM.commit();
    }
    else {
      taskCompleted = false;
      Serial.println(fbdo.errorReason());
    }
  }
}
