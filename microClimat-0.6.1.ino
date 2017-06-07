#include <ESP8266WiFi.h>
#include <DHT.h>
#include <Thread.h>
#include <MQ135.h>
#include "Wire.h"

#define PCF8591 (0x90 >> 1)      // Device address = 0       
#define PCF8591_DAC_ENABLE 0x40
#define PCF8591_ADC_CH0 0x40
#define PCF8591_ADC_CH1 0x41
#define PCF8591_ADC_CH2 0x42
#define PCF8591_ADC_CH3 0x43
byte adc_value;
const char* ssid     = "TK516"; //TK516
const char* password = "revo12345"; //revo12345
const char* host = "www.microclimat.today";
const char* station_name = "VOLTKLNTU1";
const char* user_access_token = "b341cbd7fda9bdfc02ab03fa664e3d5c";
//DUST SENSOR
int ledPower = 14;
unsigned int samplingTime = 280;
unsigned int deltaTime = 40;
unsigned int sleepTime = 9680;
float voMeasured = 0;
float dustVoltage = 0;
float dustDensity = 0;
//DHT sensor
#define DHTTYPE DHT11
#define DHTPIN 16                                                                                                    
DHT dht(DHTPIN, DHTTYPE);
float humidity, temperature, hic;
//VIBration 
int vibrationLevel = 0;
//Light
int lightLevel = 0;
//MQ135sensor
MQ135 gasSensor = MQ135(A0);
float mqLevel;
float rzero;
float ppm;
float crzero, cppm, cresistance;
String errors = "0";
//MULTITHREADING|||MULTITHREADING|||MULTITHREADING|||MULTITHREADING|||MULTITHREADING|||MULTITHREADING|||MULTITHREADING
Thread getDHTThread = Thread();
Thread getVibrationThread = Thread();
Thread getLightThread = Thread();
Thread getDustThread = Thread();
Thread sendToServerThread = Thread();
Thread getMQ135Thread = Thread();
Thread printToSerialThread = Thread();

bool set_Error(int command){ //set_Error("SERVER_NOT_CONNECTED");
  if(command>0){
      (errors == "0") ? errors = command : errors = errors + "." + command; 
  }
  (command == 0) ? errors = "0" : errors = "-1";
}
//GETdhtGETdhtGETdhtGETdhtGETdhtGETdhtGETdhtGETdhtGETdhtGETdhtGETdhtGETdhtGETdhtGETdhtGETdhtGETdhtGETdhtGETdhtGETdht
void getDHT() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature(false);
  hic = dht.computeHeatIndex(temperature, humidity, false);
  if (isnan(humidity) || isnan(temperature) || isnan(hic)) {
    Serial.println("Failed to read from DHT sensor!");
    set_Error(1);
    return;
  }
}
//SSSSSSSEEEEEEEEEEEEENNNNNNNDDDDDDDDDDTTTTTTTTTOOOOOOOOOOSSSSSSSSEEEEEEEEERRRRRRRVVVVVVVEEEEEEEERRRRRR
void sendToServer(){
  Serial.println("\n\n-------------------------------------------------------");
  Serial.print("connecting to ");
  Serial.println(host);
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    set_Error(3);
    return;
  }
  String url = "/api/post/sensor/all.php?access_token=" + String(user_access_token) + "&data=" + String(station_name) + "|" + temperature + "|" + humidity + "|" + hic + "|" + ppm + "|" + cppm + "|" + rzero + "|" + lightLevel + "|" + dustDensity + "|" + dustVoltage + "|" + vibrationLevel + "|" + errors + "|0";

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      set_Error(4);
      return;
    }
  }
  
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println("\nSite is online:)\nclosing connection...\nClearing error report...\n");
  set_Error(0);
}
byte getADC(byte config){
  Wire.beginTransmission(PCF8591);
  Wire.write(config);
  Wire.endTransmission();
  Wire.requestFrom((int) PCF8591,2);
  while (Wire.available()) 
  {
    adc_value = Wire.read(); //This needs two reads to get the value.
    adc_value = Wire.read();
  }
  return adc_value;
}
//MMMMMMMMMMMQQQQQQQQQQ11111333333333555555555555555___SSSSSSSEEEEEEEENNNNNNNNNNNNNSSSSS
void getMQ135(){
  //mqLevel = getADC(PCF8591_ADC_CH0);
  //gasSensor.mqLevel = analogRead(A0);
  rzero = gasSensor.getRZero();
  ppm = gasSensor.getPPM();
  crzero = gasSensor.getCorrectedRZero(temperature, humidity);
  cppm = gasSensor.getCorrectedPPM(temperature, humidity);
  cresistance = gasSensor.getCorrectedResistance(temperature, humidity);
  if(cppm>1600)
    set_Error(7);
  if(ppm>1600)
    set_Error(8);
}
//getVibrgetVibrgetVibrgetVibrgetVibrgetVibrgetVibrgetVibrgetVibrgetVibrgetVibrgetVibr
void getVibr(){
  vibrationLevel = getADC(PCF8591_ADC_CH0);
}

void getDust(){
  digitalWrite(ledPower,LOW);
  delayMicroseconds(samplingTime);

  voMeasured = getADC(PCF8591_ADC_CH2);

  delayMicroseconds(deltaTime);
  digitalWrite(ledPower,HIGH);
  delayMicroseconds(sleepTime);

  dustVoltage = voMeasured*(5.0/255.0);
  dustDensity = 0.9*dustVoltage-0.1;

  if ( dustDensity < 0)
  {
    dustDensity = 0.00;
    //set_Error(2);
  }
  //***************
}
//getLightgetLightgetLightgetLightgetLightgetLightgetLightgetLightgetLightgetLight
void getLight(){
  lightLevel = getADC(PCF8591_ADC_CH1);
}
//print_to_Serialprint_to_Serialprint_to_Serialprint_to_Serialprint_to_Serialprint_to_Serial
void print_to_Serial(){
  Serial.println("CO2|corrected CO2 = " + String(ppm) + "|" + String(cppm) + " ppm|RZERO = " + String(rzero) + "|Temperature = " + String(temperature) + "C|Humidity = "+ String(humidity) + "%|Index HIC = " + String(hic) + "|Vibr = " + String(vibrationLevel) + "|Light*4.016 = " + String(lightLevel) + "|Dust/dust voltagr = " + String(dustDensity) + "/" + String(dustVoltage) + "|tWire: "+ String(getADC(PCF8591_ADC_CH0)) + "/" + String(getADC(PCF8591_ADC_CH1)) + "/" + String(getADC(PCF8591_ADC_CH2)) + "/" + String(getADC(PCF8591_ADC_CH3))); 
  Serial.println("/api/post/sensor/all.php?access_token=" + String(user_access_token) + "&data=" + String(station_name) + "|" + temperature + "|" + humidity + "|" + hic + "|" + ppm + "|" + cppm + "|" + rzero + "|" + lightLevel + "|" + dustDensity + "|" + dustVoltage + "|" + vibrationLevel + "|" + errors + "|1");
}
//SSSSSSSSSSSSSSSSSSSSSSSSSSSEEEEEEEEEEEEEEETTTTTTTTTTTTTTTTTTTTTTUUUUUUUUUUUUUUUUUUUUUUPPPPPPPPPPPPPPPPPPPP
void setup() {
  pinMode(2, INPUT);
  pinMode(ledPower,OUTPUT);
  Wire.begin();
  dht.begin();
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  WiFi.hostname("MicroClimat.today"); 
  Serial.print("\nConecting to wifi: ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected\nIP address: " + WiFi.localIP());  
  Serial.println("Setting up multithreading...");
  getDHTThread.onRun(getDHT);
  getDHTThread.setInterval(10000);

  getVibrationThread.onRun(getVibr);
  getVibrationThread.setInterval(10000);

  getDustThread.onRun(getDust);
  getDustThread.setInterval(10000);
  
  getLightThread.onRun(getLight);
  getLightThread.setInterval(10000);
  
  sendToServerThread.onRun(sendToServer);
  sendToServerThread.setInterval(60000);

  getMQ135Thread.onRun(getMQ135);
  getMQ135Thread.setInterval(10000);

  printToSerialThread.onRun(print_to_Serial);
  printToSerialThread.setInterval(10000);
  Serial.println("Mutithreading is configured!");
  getDHT();
  getVibr();
  getDust();
  getLight();
  getMQ135();
}
//LOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOP
void loop() {
  if(WiFi.status() != WL_CONNECTED)
    set_Error(5);
  if(!Serial)
    set_Error(6);
  if (getDHTThread.shouldRun())
        getDHTThread.run();
  if (getDustThread.shouldRun())
        getDustThread.run();
  if (getLightThread.shouldRun())
        getLightThread.run();
  if (getVibrationThread.shouldRun())
        getVibrationThread.run();
  if (getMQ135Thread.shouldRun())
        getMQ135Thread.run();
  if (printToSerialThread.shouldRun())
        printToSerialThread.run();
  if (sendToServerThread.shouldRun())
        sendToServerThread.run();
}
