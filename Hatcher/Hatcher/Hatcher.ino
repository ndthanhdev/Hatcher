/*
  Name:		Hatcher.ino
  Created:	7/27/2016 20:53:37
  Author:	thanh
*/


#include <OneWire.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 10

int const numberOfButton = 3;

struct Button
{
	int pin;
	int state = LOW;
	int counter = 0;
	bool holding = false;
	byte holdTime = 0;
	int reading;
} buttonList[numberOfButton];

int debounce_count = 4;
const unsigned int hold_time_limit = 130;
const unsigned int hold_time_step = 20;

int current_status = 0;

int RELAYhot = A0;
int RELAYcold = A1;

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float tempMax = 37.0f;
float tempMin = 36.0f;
float tempNow = 38.0f;

unsigned int sensor_count = 0;
unsigned int sensor_fee = 100;
unsigned int relay_fee = 1500;
unsigned int relay_count = 0;

void changeStatus()
{
	current_status = (current_status + 1) % 3;
	if (current_status == 0)
		save();
}

void increase()
{
	if (current_status == 1)
	{
		tempMin = tempMin < 100 ? tempMin + 0.1f : 100;
		if (tempMin > tempMax)
			tempMax = tempMin;
	}
	else if (current_status == 2)
	{
		tempMax = tempMax < 100 ? tempMax + 0.1f : 100;
	}
}

void decrease()
{
	if (current_status == 1)
	{
		tempMin = tempMin > 0 ? tempMin - 0.1f : 0;
	}
	else if (current_status == 2)
	{
		tempMax = tempMax > 0 ? tempMax - 0.1f : 0;
		if (tempMax < tempMin)
			tempMin = tempMax;
	}

}

void processButton(Button &button, void(*actionPress)(), void(*actionHold)())
{
	if (button.state == HIGH)
	{
		button.holdTime++;
		if (button.holdTime > hold_time_limit)
		{
			button.holding = true;
		}
	}
	//for hold
	if (button.holding && button.holdTime > hold_time_limit)
	{
		if (actionHold != NULL)
			actionHold();
		button.holdTime -= hold_time_step;
		//Serial.print(button->pin);
		//Serial.write("in hold\n");
	}
	//for press
	else if (button.state == LOW && button.holdTime > 0 && button.holding == false)
	{
		//Serial.print(button->pin);
		//Serial.write("in press\n");
		if (actionPress != NULL)
			actionPress();
		button.holdTime = 0;
		button.holding = false;
	}
	else if (button.state == LOW)
	{
		button.holdTime = 0;
		button.holding = false;
	}
}

void getButtonsState()
{
	int i;
	for (i = 0; i < numberOfButton; i++)
	{
		buttonList[i].reading = digitalRead(buttonList[i].pin);
		if (buttonList[i].reading == buttonList[i].state && buttonList[i].counter > 0)
			buttonList[i].counter--;
		else if (buttonList[i].reading != buttonList[i].state)
			buttonList[i].counter++;
		if (buttonList[i].counter > debounce_count)
		{
			buttonList[i].state = buttonList[i].reading;
			buttonList[i].counter = 0;
		}
	}
}

void save()
{
	EEPROM.put(0, tempMin);
	EEPROM.put(sizeof(float), tempMax);
}
void load()
{
	EEPROM.get(0, tempMin);
	EEPROM.get(sizeof(float), tempMax);
}

void updateDisplay(float t1, float t2, float t3, int status)
{
	lcd.begin(16, 2);
	lcd.setCursor(0, 0);
	lcd.print("cdat:");
	lcd.setCursor(5, 0);
	lcd.print(t1);
	lcd.print("~");
	lcd.print(t2);
	lcd.print("        ");

	lcd.setCursor(0, 1);
	lcd.print("nhdo:");
	lcd.setCursor(5, 1);
	lcd.print(t3);
	lcd.print("           ");

	if (status != 0)
	{
		lcd.setCursor(15, 1);
		lcd.print(status);
	}
	else
	{
		lcd.setCursor(14, 1);
		lcd.print("OK");
	}
}

void readTemperature()
{
	sensors.begin(); sensors.setResolution(12);
	sensors.requestTemperaturesByIndex(0);
	tempNow = sensors.getTempCByIndex(0);
}

void setRelay()
{
	if (tempNow < tempMin)
	{
		digitalWrite(RELAYhot, LOW);
		digitalWrite(RELAYcold, HIGH);
	}
	else if (tempNow > tempMax)
	{
		digitalWrite(RELAYhot, HIGH);
		digitalWrite(RELAYcold, LOW);
	}
	else
	{
		digitalWrite(RELAYhot, HIGH);
		digitalWrite(RELAYcold, HIGH);
	}
}

#pragma region ProgramZone

// the setup function runs once when you press reset or power the board
void setup() {
	//Serial.begin(115200);

	buttonList[0].pin = 6;
	buttonList[1].pin = 7;
	buttonList[2].pin = 8;

	load();

	sensors.begin(); sensors.setResolution(12);
	lcd.begin(16, 2);

	pinMode(RELAYhot, OUTPUT);
	pinMode(RELAYcold, OUTPUT);
	pinMode(buttonList[0].pin, INPUT);
	pinMode(buttonList[1].pin, INPUT);
	pinMode(buttonList[2].pin, INPUT);


	//welcome
	lcd.setCursor(2, 0);
	lcd.print("may ap trung");
	lcd.setCursor(4, 1);
	lcd.print("xin chao");
	delay(2500);
}
// the loop function runs over and over again until power down or reset
void loop() {

	delay(1);
	getButtonsState();
	processButton(buttonList[0], changeStatus, NULL);
	processButton(buttonList[1], decrease, decrease);
	processButton(buttonList[2], increase, increase);
	updateDisplay(tempMin, tempMax, tempNow, current_status);

	if (sensor_count > sensor_fee)
	{
		readTemperature();
		//Serial.println(tempNow);
		sensor_count = 0;
	}
	//to check lcd sate
	if (relay_count > relay_fee)
	{
		setRelay();
		relay_count = 0;
	}
	sensor_count++;
	relay_count++;
}

#pragma endregion


