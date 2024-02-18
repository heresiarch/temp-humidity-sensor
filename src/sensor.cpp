#include <Arduino.h>
#include <RFM69.h>    //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>//get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>      //comes with Arduino IDE (www.arduino.cc)
#include <LowPower.h> //get library from: https://github.com/lowpowerlab/lowpower
#include <SparkFunBME280.h>
#include "eeprom-config.h"
#include "sensortypes.h"
#include "utils.h"

#define WITH_RFM69              //comment this line out if you don't have a RFM69 on your Moteino
#define FREQUENCY     RF69_868MHZ
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI      -75
#define SERIAL_BAUD   115200
#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif
#define LED           9 // Moteinos have LEDs on D9
#define FLASH_SS      8 // and FLASH SS on D8
// BME280 draws too much current only enabled when needed with D7
#define BME280_PWR_PIN 7
// default is 0x77 but I get one with 0x76
#define BME280_I2C_ADDRESS 0x76
BME280 mySensor;

#define BATT_MONITOR A7
// we use 4.7M and 1M 1% resistors = 1/(1000/(4700+1000)) see also here
// https://lowpowerlab.com/forum/low-power-techniques/battery-monitorsensing-ratio-calculation-on-motionmoteweathershield/
#define BATT_FORMULA(reading) reading * 0.0032 * 5.7
#define CYCLE_INTERVAL_SEC  600

#define power_twi_enable()      (PRR &= (uint8_t)~(1 << PRTWI))
#define power_twi_disable()     (PRR |= (uint8_t)(1 << PRTWI))
// Address in EEPROM where the configuration is stored
uint16_t eepromAddress = 0;
Configuration config = {};
SensorData sensorData = {};

void dumpConfig(const Configuration& config){
  Serial.println(F("Entered Parameters:"));
  Serial.print(F("Key: "));
  Serial.println(config.key);
  Serial.print(F("Node ID: "));
  Serial.println(config.nodeID);
  Serial.print(F("Network ID: "));
  Serial.println(config.networkID);
  Serial.print(F("Gateway ID: "));
  Serial.println(config.gatewayID);
  Serial.print(F("Sensor Type: "));
  Serial.println(config.sensorType == _BME280 ? "BME280" : "SHT31");
  Serial.print(F("RFM69 Module Type: "));
  Serial.println(config.rfm69Type == _RFM69HW ? "RFM69HW/HCW" : "RFM69W/CW");  
}

void dumpSensorData(const SensorData& sensorData){
  Serial.print(F("ID: "));
  Serial.println(sensorData.id);
  Serial.print(F("SensorType: "));
  Serial.println(sensorData.sensortype);
  Serial.print(F("Temperatur: "));
  Serial.println(float(sensorData.temperature) / 100.0);
  Serial.print(F("Humidity: "));
  Serial.println(sensorData.humidity);
  Serial.print(F("Battery: "));
  Serial.println(float(sensorData.batttery_voltage) / 100.0);
  Serial.print(F("CRC32: "));
  Serial.println(sensorData.checksum);  
}

void sendMessage(const SensorData& sensorData)
{
	byte sendLen = sizeof(sensorData);
	radio.send(config.gatewayID, (const void*) &sensorData , sendLen);
}

void enableBME280(const bool enable)
{
	if(enable)
	{
		pinMode(BME280_PWR_PIN, OUTPUT);
		digitalWrite(BME280_PWR_PIN, HIGH);
	}
	else
	{
		pinMode(BME280_PWR_PIN, OUTPUT);
		digitalWrite(BME280_PWR_PIN, LOW);
	}
}

void setupBME280()
{
		//***Driver settings********************************//
	//commInterface can be I2C_MODE or SPI_MODE
	//specify chipSelectPin using arduino pin names
	//specify I2C address.  Can be 0x77(default) or 0x76

	//For I2C, enable the following and disable the SPI section
	mySensor.settings.commInterface = I2C_MODE;
	mySensor.settings.I2CAddress = BME280_I2C_ADDRESS;

	//For SPI enable the following and dissable the I2C section
	//mySensor.settings.commInterface = SPI_MODE;
	//mySensor.settings.chipSelectPin = 10;


	//***Operation settings*****************************//

	//renMode can be:
	//  0, Sleep mode
	//  1 or 2, Forced mode
	//  3, Normal mode
	mySensor.settings.runMode = 3; //Normal mode

	//tStandby can be:
	//  0, 0.5ms
	//  1, 62.5ms
	//  2, 125ms
	//  3, 250ms
	//  4, 500ms
	//  5, 1000ms
	//  6, 10ms
	//  7, 20ms
	mySensor.settings.tStandby = 0;

	//filter can be off or number of FIR coefficients to use:
	//  0, filter off
	//  1, coefficients = 2
	//  2, coefficients = 4
	//  3, coefficients = 8
	//  4, coefficients = 16
	mySensor.settings.filter = 0;

	//tempOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
	mySensor.settings.tempOverSample = 1;

	//pressOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
    mySensor.settings.pressOverSample = 1;

	//humidOverSample can be:
	//  0, skipped
	//  1 through 5, oversampling *1, *2, *4, *8, *16 respectively
	mySensor.settings.humidOverSample = 1;
	//Calling .begin() causes the settings to be loaded
	delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
	mySensor.begin();
	delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
}

float getBatteryVoltage()
{
	float batteryVolts = 0.0;
	unsigned long readings=0;
	for (byte i=0; i<10; i++) //take 10 samples, and average
	{
		unsigned int sensorValue = analogRead(BATT_MONITOR);
		delay(1);
		readings += sensorValue;
	}
	batteryVolts = BATT_FORMULA(readings / 10.0);
	return batteryVolts;
}


void setup() {
  // Check EEPROM config first:
  Serial.begin(SERIAL_BAUD);
  if(!loadConfig(config,eepromAddress))
  {
    Serial.println(F("config invalid - entering initial setup"));
    doConfig(config,Serial,eepromAddress);
  }
  dumpConfig(config);
  #ifdef WITH_RFM69
    //radio.sleep();
  #endif

  #ifdef WITH_SPIFLASH
    if (flash.initialize())
      flash.sleep();
  #endif

    for (uint8_t i=0; i<=A5; i++)
    {
  #ifdef WITH_RFM69
      if (i == RF69_SPI_CS) continue;
  #endif
  #ifdef WITH_SPIFLASH
      if (i == FLASH_SS) continue;
  #endif
      pinMode(i, OUTPUT);
      digitalWrite(i, LOW);
    }
}

void loop() {
  Serial.begin(115200);
  Wire.setClock(400000); //Increase to fast I2C speed!
  enableBME280(true);
  delay(20);
  setupBME280();
  
  float temperature = mySensor.readTempC();
  float humidity = mySensor.readFloatHumidity();
  if(!isnan(temperature) && !isnan(humidity)){
    sensorData.temperature = int16_t(mySensor.readTempC() * 100.0);
    sensorData.humidity = mySensor.readFloatHumidity();
    sensorData.batttery_voltage = uint16_t(getBatteryVoltage() * 100.0);
    sensorData.id = config.nodeID;
    sensorData.sensortype = 1;
    sensorData.checksum = calculateCRC32((const uint8_t*) &sensorData,sizeof(sensorData)-sizeof(sensorData.checksum));
    dumpSensorData(sensorData);
    mySensor.writeRegister(BME280_CTRL_MEAS_REG, 0x00); //sleep the BME280
    radio.initialize(FREQUENCY,config.nodeID,config.networkID);
    radio.encrypt(config.key);
    if(config.rfm69Type == _RFM69HW){
      radio.setHighPower(true); //uncomment only for RFM69HW!
    }
    sendMessage(sensorData);
    radio.sleep();
  }
  //sleep TWI
  TWCR &= ~(_BV(TWEN) | _BV(TWIE) | _BV(TWEA));
  // saves ~ 160uAmps, Wire keeps the internal pullups running!!!
  digitalWrite(SDA, 0);
  digitalWrite(SCL, 0);
  power_twi_disable();
  int counter = 0;
  while(counter < CYCLE_INTERVAL_SEC)
  {
	  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
	  counter+=8;
  }
  power_twi_enable();
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
  delay(5000);
}
