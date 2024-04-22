#include <TM1637Display.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
//
// 1: Bolt
// 2: Nut silver
// 3: Nut black
// 4: Washer silver
// 5: Washer black

#define ledPin 2

// Display
#define CLK1 32
#define CLK2 33
#define CLK3 25
#define CLK4 26
#define CLK5 27
#define DIO1 21
#define DIO2 17
#define DIO3 16
#define DIO4 22
#define DIO5 15

// RFID
#define RST_PIN  4           // Configurable, see typical pin layout above
#define SS_PIN   5          // Configurable, see typical pin layout above
 
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

// Wi-Fi
const char *ssid = "CALVIN-Student2";
const char *password = "CITStudentsOnly";
// const char *ssid = "Lux";
// const char *password = "Passwordt";
WiFiClient wifiClient;

// MQTT
const char *mqtt_server = "10.252.242.19";
const char *data_topic = "rfid";
const char* mqtt_username = "siaiti";
const char* mqtt_password = "wololo";
const char *clientID = "RFID_Station_1";
PubSubClient client(mqtt_server, 1883, wifiClient);

int block_id = 4;
int block_date = 5;
int block_type = 6;

int inventory1 = 100;
int inventory2 = 100;
int inventory3 = 100;
int inventory4 = 100;
int inventory5 = 100;

TM1637Display display1(CLK1, DIO1);
TM1637Display display2(CLK2, DIO2);
TM1637Display display3(CLK3, DIO3);
TM1637Display display4(CLK4, DIO4);
TM1637Display display5(CLK5, DIO5);


void setup()
{
  Serial.begin(9600);
  setup_wifi();
  SPI.begin();
  mfrc522.PCD_Init();
}

void loop()
{
  
  all_lcd();

  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return ;
	}

  // Select one of the cards
	if ( ! mfrc522.PICC_ReadCardSerial()) {
    Serial.println("Cannot select card!");
		return;
	}
 
  byte len = 18;

  byte buffer_id[18];
  byte buffer_date[18];
  byte buffer_type[18];

  if (err_check(buffer_id, buffer_date, buffer_type, len) != 0) {
    String value = "ID: error,Date: error,Product_type: error";
    Serial.println(value);
    return;
  }

  String value = "Station_ID: 1,";

  for (uint8_t i = 0; i < 16; i++)
  {
      if (byte(buffer_id[i]) != 0x00) value += (char)buffer_id[i];
  }
  value.trim();
  value += ",";


  for (uint8_t i = 0; i < 16; i++)
  {
      if (byte(buffer_date[i]) != 0x00) value += (char)buffer_date[i];
  }
  value.trim();
  value += ",";


  for (uint8_t i = 0; i < 16; i++)
  {
      if (byte(buffer_type[i]) != 0x00) value += (char)buffer_type[i];
  }
  value.trim();

  Serial.println(value);

  connect_mqtt();
  
  if (client.publish(data_topic, value.c_str())) {
    Serial.println("Data sent!");
  }
  else {
    Serial.println("Data failed to send. Reconnecting...");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn’t clash with the client.connect call
    client.publish(data_topic, value.c_str());
  }
  
  Serial.println("Show inventory start");
  char prod = value.charAt(value.length()-1);
  if (prod == 'X') {
    inventory1 -= 1;
    inventory2 -= 3;
    inventory3 -= 3;
    inventory4 -= 2;
    inventory5 -= 1;
    show_prod_x();
  } else if (prod == 'Y') {
    inventory1 -= 1;
    inventory2 -= 1;
    inventory3 -= 5;
    inventory5 -= 3;
    show_prod_y();
  } else if (prod == 'Z') {
    inventory1 -= 1;
    inventory2 -= 6;
    inventory4 -= 3;
    show_prod_z();
  }
  Serial.println("Show inventory done");

  send_inventory_data();
  Serial.println("Inventory data sent");
  all_lcd();
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  client.disconnect();
  
  return;
}

void refresh_lcd(TM1637Display display, int quantity)
{
  display.setBrightness(0x0f);
  display.clear();
  // Show decimal numbers with/without leading zeros
  display.showNumberDec(quantity, false);

}

void all_lcd()
{
  refresh_lcd(display1, inventory1);
  refresh_lcd(display2, inventory2);
  refresh_lcd(display3, inventory3);
  refresh_lcd(display4, inventory4);
  refresh_lcd(display5, inventory5);
}

void show_prod_x()
{
  refresh_lcd(display1, 1);
  refresh_lcd(display2, 3);
  refresh_lcd(display3, 3);
  refresh_lcd(display4, 2);
  refresh_lcd(display5, 1);
  delay(10000);
}

void show_prod_y()
{
  refresh_lcd(display1, 1);
  refresh_lcd(display2, 1);
  refresh_lcd(display3, 5);
  refresh_lcd(display4, 0);
  refresh_lcd(display5, 3);
  delay(10000);
}

void show_prod_z()
{
  refresh_lcd(display1, 1);
  refresh_lcd(display2, 6);
  refresh_lcd(display3, 0);
  refresh_lcd(display4, 3);
  refresh_lcd(display5, 0);
  delay(10000);
}

int err_check(byte buffer_id[], byte buffer_date[], byte buffer_type[], byte &len)
{
  int cond = 0;

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block_id, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed for Read: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    cond += 1;
    return cond;
  }

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block_date, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed for Read: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    cond += 1;
    return cond;
  }

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block_type, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed for Read: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    cond += 1;
    return cond;
  }
 
  status = mfrc522.MIFARE_Read(block_id, buffer_id, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    cond += 1;
    return cond;
  }

  status = mfrc522.MIFARE_Read(block_date, buffer_date, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    cond += 1;
    return cond;
  }

  status = mfrc522.MIFARE_Read(block_type, buffer_type, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    cond += 1;
    return cond;
  }

  return cond;
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connect_mqtt() {
  while (! client.connect(clientID, mqtt_username, mqtt_password)) Serial.println("Connection to MQTT Broker failed…");
  Serial.println("Connected to MQTT Broker!");
}

void send_inventory_data() {
  int size = sizeof(int)*8+1;
  char temp[size];
  client.publish("Bolt", itoa(inventory1, temp, 10));
  client.publish("Nut silver", itoa(inventory2, temp, 10));
  client.publish("Nut black", itoa(inventory3, temp, 10));
  client.publish("Washer silver", itoa(inventory4, temp, 10));
  client.publish("Washer black", itoa(inventory5, temp, 10));
}