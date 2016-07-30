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

unsigned long time = 0;
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
unsigned int sensor_fee = 130;

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
	if (button.holding&&button.holdTime > hold_time_limit)
	{
		if (actionHold != NULL)
			actionHold();
		button.holdTime -= hold_time_step;
		//Serial.print(button->pin);
		//Serial.write("in hold\n");
	}
	//for press
	else if (button.state == LOW &&button.holdTime > 0 && button.holding == false)
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
	if (time != millis())
	{
		int i;
		Button *b;
		for (i = 0, b = &buttonList[i]; i < numberOfButton; i++, b = &buttonList[i])
		{
			b->reading = digitalRead(b->pin);
			if (b->reading == b->state&&b->counter > 0)
				b->counter--;
			else if (b->reading != b->state)
				b->counter++;
			if (b->counter > debounce_count)
			{
				b->state = b->reading;
				b->counter = 0;
			}
		}
	}
	time = millis();
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

void setThermal()
{
	if (tempNow < tempMin)
	{
		digitalWrite(RELAYhot, HIGH);
		digitalWrite(RELAYcold, LOW);
	}
	else if (tempNow > tempMax)
	{
		digitalWrite(RELAYhot, LOW);
		digitalWrite(RELAYcold, HIGH);
	}
	else
	{
		digitalWrite(RELAYhot, LOW);
		digitalWrite(RELAYcold, LOW);
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

	sensors.begin();
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

	getButtonsState();
	processButton(buttonList[0], changeStatus, NULL);
	getButtonsState();
	processButton(buttonList[1], decrease, decrease);
	getButtonsState();
	processButton(buttonList[2], increase, increase);
	getButtonsState();
	updateDisplay(tempMin, tempMax, tempNow, current_status);
	getButtonsState();

	if (sensor_count > sensor_fee)
	{
		sensors.requestTemperatures();
		tempNow = sensors.getTempCByIndex(0);
		setThermal();
		sensor_count = 0;
	}
	getButtonsState();

	sensor_count++;
}

#pragma endregion


