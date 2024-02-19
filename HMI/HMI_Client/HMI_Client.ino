// Dependencies for Arduino GIGA Display Shield
#include "Arduino_H7_Video.h"
#include "lvgl.h"
#include "Arduino_GigaDisplayTouch.h"
#include "stdio.h"

//Dependencies for working with Modbus TCP on Arduino GIGA R1 WiFi and Opta WiFi
#include <SPI.h>
#include <WiFi.h>
#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>
#include "arduino_secrets.h"

Arduino_H7_Video Display(800, 480, GigaDisplayShield); //Display type and dimensions
Arduino_GigaDisplayTouch TouchDetector; //Using touch in this application

lv_obj_t* img1; // Object for images
lv_obj_t* screen; // Object for different screens shown on the HMI
lv_obj_t* grid;  // Object to create grids


lv_obj_t* obj_cb; //Object for Checbox (LEDs and Relays)
lv_obj_t* cb_LR1; // Object for the checkbox object: LED 1 and Relay 1
lv_obj_t* cb_LR2; // Object for the checkbox object: LED 2 and Relay 2
lv_obj_t* obj_led1; // Object for testing the digital in 1 (Button) and show the bool state with a LED
lv_obj_t* obj_led2; // Object for testing the digital in 2 (Button) and show the bool state with a LED
lv_obj_t* obj_slide; // Object to show the status of the Analog Input (Temperature)

lv_obj_t* slider; // Slide to show the status of the analog input (Temperature)
lv_obj_t* label1; // Object to set text label for the obj_slide and slider
lv_obj_t* label2; // Object to show the temperature "°C" legend on the slider (analog input)
lv_obj_t* label3; // Object to show the message for connection errors between devices
lv_obj_t* label_led1; // Object to show the label for the digital input 1 (button)
lv_obj_t* label_led2; // Object to show the label for the digital input 2 (button)
lv_obj_t* label_LR1; // Object to show the label for the digital input 1 (button)
lv_obj_t* label_LR2; // Object to show the label for the digital input 2 (button)

char ssid[] = SECRET_SSID;        // Network SSID (name)
char pass[] = SECRET_PASS;    // Network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;  // Status for connection to the network

WiFiClient wifiClient;
ModbusTCPClient modbusTCPClient(wifiClient);

IPAddress server(192, 168, 0, 176); // IP Address for Opta WiFi, need to confirm before trying to connect both devices

static void button1_event1(lv_event_t * e){
  modbusTCPClient.coilWrite(0x00, 0x01);
}

static void button1_event2(lv_event_t * e){
  modbusTCPClient.coilWrite(0x00, 0x00);
}

static void button2_event1(lv_event_t * e){
  modbusTCPClient.coilWrite(0x01, 0x01);
}

static void button2_event2(lv_event_t * e){
  modbusTCPClient.coilWrite(0x01, 0x00);
}

void setup() {
  Display.begin();
  TouchDetector.begin();
  Serial.begin(9600);

 while (status != WL_CONNECTED) {
    //Serial.print("Attempting to connect to SSID:");
    //Serial.println(ssid);
    status = WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network
    delay(10000); // wait 10 seconds for connection:
  }
  //printWifiStatus();  // you're connected now, so print out the status
  if (!modbusTCPClient.connected()) {
    //Serial.println("Attempting to connect to Modbus TCP server"); // Start the Modbus TCP client
    if (!modbusTCPClient.begin(server)) {
      //Serial.println("Modbus TCP Client failed to connect! Restart HMI");
      errorScreen();
    } 
    else {
      //Serial.println("Modbus TCP Client connected");
      mainScreen();
    }
  }
}

void loop() {
  // client not connected
 if (!modbusTCPClient.connected()) {
    //Serial.println("Modbus TCP Client failed to connect! Restart HMI");
  } 
 else {
   // client connected
    InWidgets();
    //OutButtons();
  }
  lv_timer_handler();
}

/*void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID:");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address:");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}*/

static void mainScreen (){
  //Display & Grid Setup ----------------------------------------------------------------------------------------------------------------------------
  screen = lv_obj_create(lv_scr_act());
  lv_obj_set_size(screen, Display.width(), Display.height());

  static lv_coord_t col_dsc[] = { 370, 350, LV_GRID_TEMPLATE_LAST };
  static lv_coord_t row_dsc[] = { 440, LV_GRID_TEMPLATE_LAST };

  grid = lv_obj_create(lv_scr_act());
  lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
  lv_obj_set_size(grid, Display.width(), Display.height());

  lv_obj_center(grid);

  //top left ----------------------------------------------------------------------------------------------------------------------------------------
  
  obj_cb = lv_obj_create(grid);
  lv_obj_set_grid_cell(obj_cb, LV_GRID_ALIGN_STRETCH, 0, 1,  //column
                       LV_GRID_ALIGN_STRETCH, 0, 1);         //row
  lv_obj_set_flex_flow(obj_cb, LV_FLEX_FLOW_COLUMN);

  LV_IMG_DECLARE(img_arduinologo);
  img1 = lv_img_create(obj_cb);
  lv_img_set_src(img1, &img_arduinologo);

  lv_obj_set_size(img1, 200, 150);

  cb_LR1 = lv_btn_create(obj_cb);  // Create the checkbox and store its object: button
  lv_obj_set_size(cb_LR1, 100, 40);
  label_LR1 = lv_label_create(cb_LR1);
  lv_label_set_text(label_LR1, "Led/Relay1");
  lv_obj_center(label_LR1);
  lv_obj_add_event_cb(cb_LR1, button1_event1, LV_EVENT_PRESSING, NULL);
  lv_obj_add_event_cb(cb_LR1, button1_event2, LV_EVENT_RELEASED, NULL);

  cb_LR2 = lv_btn_create(obj_cb);  // Create the checkbox and store its object: button
  lv_obj_set_size(cb_LR2, 100, 40);
  label_LR2 = lv_label_create(cb_LR2);
  lv_label_set_text(label_LR2, "Led/Relay2");
  lv_obj_center(label_LR2);
  lv_obj_add_event_cb(cb_LR2, button2_event1, LV_EVENT_PRESSING, NULL);
  lv_obj_add_event_cb(cb_LR2, button2_event2, LV_EVENT_RELEASED, NULL);

  // Create two LEDs and switch them OFF
  obj_led1 = lv_led_create(obj_cb);
  lv_led_off(obj_led1);
  obj_led2 = lv_led_create(obj_cb);
  lv_led_off(obj_led2);

  // Create a label next to each LED
  label_led1 = lv_label_create(lv_scr_act());
  lv_label_set_text(label_led1, "AUTO");
  lv_obj_align_to(label_led1, obj_led1, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
  label_led2 = lv_label_create(lv_scr_act());
  lv_label_set_text(label_led2, "INIT");
  lv_obj_align_to(label_led2, obj_led2, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  //bottom left -------------------------------------------------------------------------------------------------------------------------------------
  obj_slide = lv_obj_create(grid);
  lv_obj_set_grid_cell(obj_slide, LV_GRID_ALIGN_STRETCH, 1, 1,  //column
                       LV_GRID_ALIGN_STRETCH, 0, 1);            //row

  slider = lv_bar_create(obj_slide);
  lv_obj_set_size(slider, 310, 20); // Set the width
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);

  // Create a label above the slider
  label1 = lv_label_create(obj_slide);
  lv_label_set_text(label1, "0");
  lv_obj_align_to(label1, slider, LV_ALIGN_OUT_TOP_MID, 0, -15); // Align Top of the slider

  label2 = lv_label_create(obj_slide);
  lv_label_set_text(label2, "°C");
  lv_obj_align_to(label2, label1, LV_ALIGN_OUT_RIGHT_MID, 40, 0);
}

static void errorScreen(){
  //Display & Grid Setup ----------------------------------------------------------------------------------------------------------------------------
  screen = lv_obj_create(lv_scr_act());
  lv_obj_set_size(screen, Display.width(), Display.height());

  static lv_coord_t col_dsc[] = { 760, LV_GRID_TEMPLATE_LAST };
  static lv_coord_t row_dsc[] = { 440, LV_GRID_TEMPLATE_LAST };

  grid = lv_obj_create(lv_scr_act());
  lv_obj_set_grid_dsc_array(grid, col_dsc, row_dsc);
  lv_obj_set_size(grid, Display.width(), Display.height());

  lv_obj_center(grid);

  obj_cb = lv_obj_create(grid);
  lv_obj_set_grid_cell(obj_cb, LV_GRID_ALIGN_STRETCH, 0, 1,  //column
                         LV_GRID_ALIGN_STRETCH, 0, 1);         //row
  lv_obj_set_flex_flow(obj_cb, LV_FLEX_FLOW_COLUMN);

  LV_IMG_DECLARE(img_error);
  img1 = lv_img_create(obj_cb);
  lv_img_set_src(img1, &img_error);

  lv_obj_set_size(img1, 740, 440);

  label3 = lv_label_create(lv_scr_act());
  lv_label_set_text(label3, "Connection failed. Turn on Opta and then restart the HMI");
  lv_obj_align_to(label3, img1, LV_ALIGN_CENTER, 0, 0);
}

void InWidgets (){
  int voltFOpta;
  float TempScreen;
  char voltageStr[20];
  /* Reading coils to update the LEDs on HMI */
  if (modbusTCPClient.coilRead(0x02)){
     lv_led_on(obj_led1);
  }
  else
  {
    lv_led_off(obj_led1);
  }
  if (modbusTCPClient.coilRead(0x03)){
    lv_led_on(obj_led2);
  }
  else
  {
    lv_led_off(obj_led2);
  }
  /* Reading the input register to update the Temperature, analog in */
  voltFOpta = modbusTCPClient.inputRegisterRead(0x04);
  TempScreen = voltFOpta * (100.0/4095); //Analog input 0 - 10 volts; 12 bit resolution, 0 - 4095; 10mV/°C.
  lv_bar_set_value(slider, TempScreen, LV_ANIM_OFF);
  sprintf(voltageStr, "%.0f", TempScreen, "°C");
  lv_label_set_text(label1, voltageStr);
}