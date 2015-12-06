#include <UTFT.h>
#include <memorysaver.h>
// UTFT_Buttons_Demo_320x240 (C)2013 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// A small demo to demonstrate the use of some of the
// functions of the UTFT_Buttons add-on library.
//
// This demo was made for modules with a screen resolution 
// of 320x240 pixels, but should work on larger screens as
// well.
//
// This program requires both the UTFT and UTouch libraries
// in addition to the UTFT_Buttons add-on library.
//

// This code block is only needed to support multiple
// MCU architectures in a single sketch.
#if defined(__AVR__)
#define imagedatatype  unsigned int
#elif defined(__PIC32MX__)
#define imagedatatype  unsigned short
#elif defined(__arm__)
#define imagedatatype  unsigned short
#endif
// End of multi-architecture block

#include <SPI.h>
#include "MFRC522.h"
#include "UTFT.h"
#include "UTouch.h"
#include "UTFT_Buttons.h"
#include "UTFT_DLB.h"
#include "UTFT_DLB_Buttons.h"
//#include <UTFT_DLB_Buttons.h>
#include <EEPROM.h>
//#include <stdio.h>

#define KEY_TONE 2000
#define WRONG_TONE 500
#define PIN_BUZZER 12

#define TIMEOUT 30

#define VGA_TEMP VGA_RED
#define CUSTOM_BLUE 0x653F
#define CUSTOM_ORANGE 0xFDCC

#define CARDS_ADDR 400
#define TIME_OPEN_ADDR 401
#define ALARM_ADDR 402
#define ADMIN_PASS_ADDR 403
#define ALARM_DELAY_ADDR 407

#define RST_PIN         49          // Configurable, see typical pin layout above
#define SS_PIN          53         // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance.

// Declare which fonts we will be using
//extern uint8_t Smallfont[];
extern uint8_t DejaVuSans18[];
extern uint8_t ArialNumFontPlus[];
extern uint8_t CalibriBold32x48[];

extern imagedatatype arrow_up[];
extern imagedatatype arrow_down[];
extern imagedatatype arrow_left[];
extern imagedatatype arrow_right[];
extern imagedatatype settings_icon[];
extern imagedatatype unlock_icon[];
extern imagedatatype tag_icon[];
extern imagedatatype keyboard[];
extern imagedatatype keyboard[];
extern imagedatatype warning_icon[];

//extern uint8_t Dingbats1_XL[];
extern uint8_t hallfetica_normal[];

// Set up UTFT...
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino 2009/Uno/Leonardo shield   : <display model>,19,18,17,16
// Standard Arduino Mega/Due shield            : <display model>,38,39,40,41
// CTE TFT LCD/SD Shield for Arduino Due       : <display model>,25,26,27,28
// Standard chipKit Uno32/uC32                 : <display model>,34,35,36,37
// Standard chipKit Max32                      : <display model>,82,83,84,85
// AquaLEDSource All in One Super Screw Shield : <display model>,82,83,84,85
//
// Remember to change the model parameter to suit your display module!
//UTFT          utft(ILI9325C,A5,A4,A3,A2);
// UTFT_DLB(byte model, int RS, int WR,int CS, int RST, int SER=0);
UTFT_DLB utft(ILI9325C,38,39,40,41);
//UTFT_DLB utft_dlb(ILI9325C,A5,A4,A3,A2);

// Set up UTouch...
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino 2009/Uno/Leonardo shield   : 15,10,14,9,8
// Standard Arduino Mega/Due shield            : 6,5,4,3,2
// CTE TFT LCD/SD Shield for Arduino Due       : 6,5,4,3,2
// Standard chipKit Uno32/uC32                 : 20,21,22,23,24
// Standard chipKit Max32                      : 62,63,64,65,66
// AquaLEDSource All in One Super Screw Shield : 62,63,64,65,66
UTouch        myTouch(47,46,45,44,19);

// Finally we set up UTFT_Buttons :)
UTFT_DLB_Buttons  myButtons(&utft, &myTouch);

char stCurrent[12]="";
int stCurrentLen=0;
long card[100];
uint8_t cards=0;
int time_open;
boolean alarm;
boolean door_open =false;
uint32_t admin_pass;
int alarm_delay;
uint8_t seconds = 0;
boolean toggle1;
boolean key_pressed = false;
boolean sleep = false;

//char stLast[10]="";

//This function will write a 4 byte (32bit) long to the eeprom at
//the specified address to address + 3.
void EEPROMWritelong(int address, uint32_t value)
{
	//Decomposition from a long to 4 bytes by using bitshift.
	//One = Most significant -> Four = Least significant byte
	byte four = (value & 0xFF);
	byte three = ((value >> 8) & 0xFF);
	byte two = ((value >> 16) & 0xFF);
	byte one = ((value >> 24) & 0xFF);

	//Write the 4 bytes into the eeprom memory.
	EEPROM.write(address, four);
	EEPROM.write(address + 1, three);
	EEPROM.write(address + 2, two);
	EEPROM.write(address + 3, one);
}

uint32_t EEPROMReadlong(long address)
{
	//Read the 4 bytes from the eeprom memory.
	long four = EEPROM.read(address);
	long three = EEPROM.read(address + 1);
	long two = EEPROM.read(address + 2);
	long one = EEPROM.read(address + 3);

	//Return the recomposed long by using bitshift.
	return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

void init_EEPROM()
{
	EEPROMWritelong(0,111111111UL);
	EEPROMWritelong(4,222222222UL);
	EEPROMWritelong(8,333333333UL);
	EEPROMWritelong(12,444444444UL);
	EEPROMWritelong(16,555555555UL);
	EEPROMWritelong(20,666666666UL);
	EEPROMWritelong(24,777777777UL);
	EEPROMWritelong(28,888888888UL);
	EEPROMWritelong(32,999999999UL);
	EEPROMWritelong(36,000000000UL);

	EEPROM.write(CARDS_ADDR, 10);
	EEPROM.write(TIME_OPEN_ADDR, 3);
	EEPROM.write(ALARM_ADDR, false);
	EEPROMWritelong(ADMIN_PASS_ADDR,1234);
	EEPROM.write(ALARM_DELAY_ADDR, 5);
}

void updateStr(int val, int size)
{
	if (stCurrentLen<size)
	{
		stCurrent[stCurrentLen]=val;
		stCurrent[stCurrentLen+1]='\0';
		stCurrentLen++;
		//utft.setColor(0, 255, 0);
		uint8_t *temp = utft.getFont();
		utft.setFont(CalibriBold32x48);
		utft.print(stCurrent, CENTER, 7);
		utft.setFont(temp);
	}
	else
	{
		tone(12,WRONG_TONE,100);
	}
}

uint32_t draw_keypad(int size)
{
	utft.clrScr();
	utft.setColor(VGA_WHITE);
	utft.setBackColor(VGA_BLACK);
	myButtons.setButtonColors(VGA_WHITE,VGA_GRAY,CUSTOM_BLUE,CUSTOM_ORANGE,CUSTOM_BLUE);
	int but1, but2, but3, but4, but5, but_cancelar, pressed_button;
	int but6, but7, but8, but9, but0, but_gravar;
	boolean temp = false;
	
	stCurrent[0]='\0';
	stCurrentLen=0;
	
	myButtons.setTextFont(hallfetica_normal);

	// Draw the upper row of buttons
	but1 = myButtons.addButton( 15+0*60, 60, 50, 50, "1");
	but2 = myButtons.addButton( 15+1*60, 60, 50, 50, "2");
	but3 = myButtons.addButton( 15+2*60, 60, 50, 50, "3");
	but4 = myButtons.addButton( 15+3*60, 60, 50, 50, "4");
	but5 = myButtons.addButton( 15+4*60, 60, 50, 50, "5");
	
	but6 = myButtons.addButton( 15+0*60, 60+60, 50, 50, "6");
	but7 = myButtons.addButton( 15+1*60, 60+60, 50, 50, "7");
	but8 = myButtons.addButton( 15+2*60, 60+60, 50, 50, "8");
	but9 = myButtons.addButton( 15+3*60, 60+60, 50, 50, "9");
	but0 = myButtons.addButton( 15+4*60, 60+60, 50, 50, "0");
	but_cancelar = myButtons.addButton(20, 190,  130,  30, "Cancelar");
	but_gravar = myButtons.addButton(170, 190,  130,  30, "Gravar", BUTTON_DISABLED);
	myButtons.drawButtons();
		
	while(!sleep)
	{
		// Look for new cards
		if ( size == 10 && stCurrentLen < size && mfrc522.PICC_IsNewCardPresent()) {
					// Select one of the cards
					if (  mfrc522.PICC_ReadCardSerial()) {
						tone(PIN_BUZZER,KEY_TONE,100);
						uint32_t card_uid=get_card_uid(mfrc522.uid.uidByte, mfrc522.uid.size);
						Serial.println(card_uid);
						
						sprintf(stCurrent, "%lu", card_uid);
						stCurrentLen=10;

						uint8_t *temp = utft.getFont();
						utft.setFont(CalibriBold32x48);
						utft.print(stCurrent, CENTER, 7);
						utft.setFont(temp);
					}
		}

		if (myTouch.dataAvailable() == true)
		{
			seconds = 0;
			key_pressed == false;
			tone(PIN_BUZZER,KEY_TONE,100);		
			pressed_button = myButtons.checkButtons();
			if (pressed_button==but1)		updateStr('1',size);
			else if (pressed_button==but2)	updateStr('2',size);
			else if (pressed_button==but3)	updateStr('3',size);
			else if (pressed_button==but4)	updateStr('4',size);
			else if (pressed_button==but5)	updateStr('5',size);
			else if (pressed_button==but6)	updateStr('6',size);
			else if (pressed_button==but7)	updateStr('7',size);
			else if (pressed_button==but8)	updateStr('8',size);
			else if (pressed_button==but9)	updateStr('9',size);
			else if (pressed_button==but0)	updateStr('0',size);
			else if (pressed_button==but_gravar)
			{
				myButtons.deleteAllButtons();
				return strtoul( stCurrent, NULL, 10 );
			}
			else if (pressed_button==but_cancelar)
			{
				if (stCurrentLen>0)
				{
					stCurrent[0]='\0';
					stCurrentLen=0;
					utft.setColor(0, 0, 0);
					utft.fillRect(0, 7, 319, 7+48);
					utft.setColor(VGA_WHITE);
				} else
				{
					stCurrent[0]='\0';
					stCurrentLen=0;
					myButtons.deleteAllButtons();
					return char(0);
				}
			}
			else
			{
				tone(PIN_BUZZER,WRONG_TONE,100);
			}
		}
		if ((stCurrentLen == size) && (myButtons.buttonEnabled(but_gravar)==false)) myButtons.enableButton(but_gravar,true);
		else if ((stCurrentLen < size) && (myButtons.buttonEnabled(but_gravar)==true)) myButtons.enableButton(but_gravar,false);
		if (stCurrentLen == 1 && !temp)
		{
			myButtons.relabelButton(but_cancelar,"Apagar",true);
			temp = !temp;
		}
		else if (stCurrentLen == 0 && temp)
		{
			myButtons.relabelButton(but_cancelar,"Cancelar",true);
			temp = !temp;
		}
	}
}

char *i2str(int i, char *buf){
	byte l=0;
	if(i<0) buf[l++]='-';
	boolean leadingZ=true;
	for(int div=10000, mod=0; div>0; div/=10){
		mod=i%div;
		i/=div;
		if(!leadingZ || i!=0){
			leadingZ=false;
			buf[l++]=i+'0';
		}
		i=mod;
	}
	buf[l]=0;
	return buf;
}

void list_cards()
{
	utft.clrScr();
	titulo("LISTA DE TAGS");

	myButtons.setButtonColors(VGA_WHITE,VGA_GRAY,CUSTOM_BLUE,CUSTOM_ORANGE,CUSTOM_BLUE);
	utft.setFont(hallfetica_normal);
	int but_u, but_d, but_apagar, but_sair, pressed_button;
	
	but_u = myButtons.addButton( 252, 55, 32, 32,arrow_up, BUTTON_NO_BORDER);
	but_d = myButtons.addButton( 252, 130, 32, 32,arrow_down, BUTTON_NO_BORDER);
	but_sair = myButtons.addButton( 10, 200, 100, 30, "Voltar");
	but_apagar = myButtons.addButton( 125, 200, 100, 30, "Apagar");
	myButtons.drawButtons();
	

	utft.setColor(CUSTOM_BLUE);
	//utft.fillRoundRect(30, 30, 210, 146);
	utft.fillRoundRect(30,100+18/2+116/2, 210, 100+18/2-116/2);
	
	int sel = 0;
	int card_min = 0;
	int card_max = 4;
	if (card_max>cards-1) card_max=cards-1;	
	char str[10];
	
	while(!sleep)
	{
		int posy = 100+18/2-116/2+10;
		int posx = 40;
		int i = 0;

		utft.setBackColor(VGA_BLACK);
		utft.setColor(VGA_WHITE);
		utft.setFont(DejaVuSans18);
		sprintf(str, " %d/%d ", sel+1,cards);
		uint8_t txt_size = utft.getStringWidth(str);
		utft.print(str,252+32/2-txt_size/2,100);
		utft.setFont(hallfetica_normal);
		utft.setColor(CUSTOM_ORANGE);

		for (i=card_min ; i<=card_max ; i++)
		{
			if (sel==i) utft.setBackColor(VGA_GRAY);
			else utft.setBackColor(CUSTOM_BLUE);

			uint32_t card=EEPROMReadlong(i*4);
			sprintf(stCurrent, "%010lu", card);
			utft.print(stCurrent, posx, posy);
			posy += 20;
		}
		
		while (myTouch.dataAvailable() != true);
		
		seconds = 0;
		
		tone(PIN_BUZZER,KEY_TONE,100);
		pressed_button = myButtons.checkButtons();
		if (pressed_button==but_u && sel>0)
		{
			sel--;
			if (sel<card_min)
			{
				card_min--;
				card_max--;
			}
		}
		else if (pressed_button==but_d && sel<cards-1)
		{
			sel++;
			if (sel>card_max)
			{
				card_min++;
				card_max++;
			}
		}
		else if (pressed_button==but_apagar)
		{
			for (int i=sel ; i<cards-1 ; i++)
			{
				card[i]=card[i+1];
				EEPROMWritelong(i*4,card[i]);
			}
			card[cards]=0xFFFFFFFF;
			EEPROMWritelong((i+1)*4,card[i+1]);
			cards--;
			EEPROM.write(CARDS_ADDR, cards);
		}
		else if (pressed_button==but_sair)
		{
			myButtons.deleteAllButtons();
			return;
		}
		else tone(PIN_BUZZER,WRONG_TONE,100);
	}
}

void lista()
{
	utft.clrScr();
	//utft.fillScr(VGA_WHITE);
	titulo("Defenicoes");
	myButtons.setTextFont(DejaVuSans18);
	myButtons.setButtonColors(VGA_WHITE,VGA_GRAY,CUSTOM_BLUE,CUSTOM_ORANGE,CUSTOM_BLUE);
	utft.setFont(DejaVuSans18);
	int but_u, but_d, but_apagar, but_sair, pressed_button;
	but_u = myButtons.addButton( 250, 50, 40, 40, "U");
	but_d = myButtons.addButton( 250, 140, 40, 40, "D");
	but_sair = myButtons.addButton( 10, 180, 100, 30, "Voltar");
	but_apagar = myButtons.addButton( 125, 180, 100, 30, "Apagar");
	myButtons.drawButtons();
	

	utft.setColor(CUSTOM_BLUE);
	utft.fillRoundRect(30, 30, 210, 146);

	char* items[10] = {"Tempo de Abertura", "Alarme","Retardo do Alarme","Porta Aberta","teste5","teste6","teste7","teste8","teste9","teste10"};
	
	int sel = 0;
	int card_min = 0;
	int card_max = 4;
	if (card_max>cards-1) card_max=cards-1;
	
	while(1)
	{
		int posy = 40;
		int posx = 40;
		int i = 0;

		utft.setBackColor(VGA_BLACK);
		utft.setColor(VGA_WHITE);

		for (i=card_min ; i<=card_max ; i++)
		{
			char* temp = items[i];
			if (sel==i) utft.setBackColor(VGA_GRAY);
			else utft.setBackColor(CUSTOM_BLUE);
			utft.print(items[i], posx, posy);
			posy += 20;
		}
		
		while (myTouch.dataAvailable() != true);
		
		tone(PIN_BUZZER,KEY_TONE,100);
		pressed_button = myButtons.checkButtons();
		if (pressed_button==but_u && sel>0)
		{
			sel--;
			if (sel<card_min)
			{
				card_min--;
				card_max--;
			}
		}
		else if (pressed_button==but_d && sel<cards-1)
		{
			sel++;
			if (sel>card_max)
			{
				card_min++;
				card_max++;
			}
		}
		else if (pressed_button==but_apagar)
		{
			for (int i=sel ; i<cards-1 ; i++)
			{
				card[i]=card[i+1];
				EEPROMWritelong(i*4,card[i]);
			}
			card[cards]=0xFFFFFFFF;
			EEPROMWritelong((i+1)*4,card[i+1]);
			cards--;
			EEPROM.write(CARDS_ADDR, cards);
		}
		else if (pressed_button==but_sair)
		{
			myButtons.deleteAllButtons();
			return;
		}
		else tone(PIN_BUZZER,WRONG_TONE,100);
	}
}

void menu_def()
{
	int but_u1, but_d1, but_u2, but_d2, but_alarm, but_definir, but_right, but_sair, but_gravar, pressed_button;
	
	while(!sleep)
	{
		utft.clrScr();
		titulo("Defenicoes");
		utft.setColor(VGA_WHITE);
		utft.setBackColor(VGA_BLACK);
		utft.setFont(DejaVuSans18);
		utft.print("Tempo de Abertura",10,35);
		utft.print("Alarme",10,75);
		utft.print("Retardo do alarme",10,115);
		utft.print("Password",10,155);
	
		myButtons.setButtonColors(VGA_WHITE,VGA_GRAY,CUSTOM_BLUE,CUSTOM_ORANGE,CUSTOM_BLUE);
		but_d1 = myButtons.addButton( 225, 29, 32, 32,arrow_down, BUTTON_NO_BORDER);
		but_u1 = myButtons.addButton( 285, 29, 32, 32,arrow_up, BUTTON_NO_BORDER);
		but_definir = myButtons.addButton( 245, 147, 50, 32,keyboard, BUTTON_NO_BORDER);
		but_d2 = myButtons.addButton( 225, 109, 32, 32,arrow_down, BUTTON_NO_BORDER);
		but_u2 = myButtons.addButton( 285, 109, 32, 32,arrow_up, BUTTON_NO_BORDER);
		but_right = myButtons.addButton( 263, 204, 32, 32,arrow_right, BUTTON_NO_BORDER);
		but_sair = myButtons.addButton( 10, 205, 100, 30, "Sair");
		but_gravar = myButtons.addButton( 125, 205, 100, 30, "Gravar");
		myButtons.drawButtons();
		
		myButtons.setButtonColors(VGA_BLACK,VGA_BLACK,VGA_BLACK,VGA_BLACK,VGA_BLACK);
		but_alarm = myButtons.addButton( 253, 74, 43, 22, "",BUTTON_NO_BORDER);
		myButtons.drawButton(but_alarm);
		myButtons.setButtonColors(VGA_WHITE,VGA_GRAY,CUSTOM_BLUE,CUSTOM_ORANGE,CUSTOM_BLUE);
				
		draw_switch(263,84,alarm);
		
		utft.setColor(VGA_WHITE);
		
		utft.setFont(hallfetica_normal);
		utft.printNumI(time_open,262,35);
		utft.printNumI(alarm_delay,262,115);
		
		boolean redraw = false;
		
		while (!redraw && !sleep)
		{
			if (myTouch.dataAvailable()==true)
			{
				seconds = 0;
				tone(PIN_BUZZER,KEY_TONE,100);
				utft.setColor(VGA_WHITE);
				pressed_button = myButtons.checkButtons();
			
				if (pressed_button==but_u1 && time_open<9)
				{
					time_open++;
					utft.printNumI(time_open,262,35);
				} else if (pressed_button==but_d1 && time_open>0)
				{
					time_open--;
					utft.printNumI(time_open,262,35);
				} else if (pressed_button==but_alarm)
				{
					alarm = !alarm;
					draw_switch(263,84,alarm);
				} else if (pressed_button==but_definir)
				{
					myButtons.deleteAllButtons();
					uint32_t pass = draw_keypad(4);
					if (pass != char(0))
					{
						EEPROMWritelong(ADMIN_PASS_ADDR,pass);
					}
					redraw = true;
				} else if (pressed_button==but_u2 && alarm_delay<99)
				{
					alarm_delay++;
					utft.printNumI(alarm_delay,262,115);
				}  else if (pressed_button==but_d2 && alarm_delay>0)
				{
					alarm_delay--;
					utft.printNumI(alarm_delay,262,115);
				}  else if (pressed_button==but_gravar)
				{
					EEPROM.write(TIME_OPEN_ADDR, time_open);
					EEPROM.write(ALARM_ADDR, alarm);
					//EEPROMWritelong(ADMIN_PASS_ADDR,1234);
					EEPROM.write(ALARM_DELAY_ADDR, alarm_delay);
				}  else if (pressed_button==but_sair)
				{
					myButtons.deleteAllButtons();
					return;
				} else tone(PIN_BUZZER,WRONG_TONE,100);
			}
		}
	}
}

void menu_cards()
{
	while(!sleep)
	{
		utft.clrScr();
		titulo("ADIC./REMOVER TAGS");
		
		utft.setFont(hallfetica_normal);
		
		myButtons.setButtonColors(VGA_WHITE,VGA_GRAY,CUSTOM_BLUE,CUSTOM_ORANGE,CUSTOM_BLUE);
		int but_adicionar, but_apagar, but_sair, pressed_button;
		but_adicionar = myButtons.addButton( 80-100/2, 90, 115, 50, "Adici.");
		but_apagar = myButtons.addButton(225-100/2, 90, 115, 50, "Apagar");
		but_sair = myButtons.addButton( 10, 200, 100, 30, "Voltar");
		myButtons.drawButtons();
		
		boolean redraw = false;
		
		while (redraw == false && !sleep)
		{
			if (myTouch.dataAvailable()==true)
			{
				seconds = 0;	
				tone(PIN_BUZZER,KEY_TONE,100);
				utft.setColor(VGA_WHITE);
				pressed_button = myButtons.checkButtons();
				
				if (pressed_button==but_adicionar)
				{
					myButtons.deleteAllButtons();
					long new_card = draw_keypad(10);
					if (new_card != 0)
					{
						card[cards] = new_card;
						EEPROMWritelong(cards*4, new_card);
						cards++;
						EEPROM.write(CARDS_ADDR, cards);
					}
					redraw = true;
				} else if (pressed_button==but_apagar)
				{
					myButtons.deleteAllButtons();
					list_cards();
					redraw = true;
				} else if (pressed_button==but_sair)
				{
					myButtons.deleteAllButtons();
					return;
				} else tone(PIN_BUZZER,WRONG_TONE,100);
			}
		}
	}
}

void ShowReaderVersion() {
	// Get the MFRC522 firmware version
	byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
	if (v == 0x88)
	utft.print("clone",100,100);
	else if (v == 0x90)
	utft.print("0.0",100,100);
	else if (v == 0x91)
	utft.print("v1.0",100,100);
	else if (v == 0x92)
	utft.print("v2.0",100,100);
	else
	utft.print("unknown",100,100);
	// When 0x00 or 0xFF is returned, communication probably failed
	if ((v == 0x00) || (v == 0xFF))
	utft.print("WARNING: Communication failure, is the MFRC522 properly connected?",0,100);
}

uint32_t get_card_uid(byte *buffer, byte bufferSize) {
	for (byte i = 0; i < bufferSize; i++) {
		Serial.print(buffer[i] < 0x10 ? " 0" : " ");
		Serial.print(buffer[i], HEX);
	}
	uint32_t card_uid;

	card_uid  =  (uint32_t)buffer[0] << 24;
	card_uid += (uint32_t)buffer[1] << 16;
	card_uid += (uint32_t)buffer[2] << 8;
	card_uid += (uint32_t)buffer[3];
	
	return card_uid;
}

void titulo(char *texto)
{
	utft.setFont(DejaVuSans18);
	utft.setColor(CUSTOM_ORANGE);
	utft.fillRect(0,0,319,18);
	utft.setColor(VGA_WHITE);
	utft.setBackColor(VGA_TRANSPARENT);
	uint8_t txt_size = utft.getStringWidth(texto);
	utft.print(texto,(320-txt_size)/2,2);
}

void main_menu()
{
	while (!sleep)
	{
		utft.clrScr();
		titulo("MENU");
		myButtons.setTextFont(DejaVuSans18);
	
		myButtons.setButtonColors(VGA_WHITE,VGA_GRAY,CUSTOM_BLUE,CUSTOM_ORANGE,CUSTOM_BLUE);
		int but_def, but_card, but_open, pressed_button;
		but_def = myButtons.addButton( 75, 80-40/2, 64, 64,settings_icon, BUTTON_NO_BORDER);
		but_card = myButtons.addButton(181, 80-40/2, 64, 64,tag_icon, BUTTON_NO_BORDER);
		but_open = myButtons.addButton(75, 160-40/2, 64, 64,unlock_icon, BUTTON_NO_BORDER);
		myButtons.drawButtons();
		
		boolean redraw = false;
	
		while(!sleep && !redraw)
		{
			if (myTouch.dataAvailable() == true)
			{
				seconds = 0;
				tone(PIN_BUZZER,KEY_TONE,100);
				pressed_button = myButtons.checkButtons();
				if (pressed_button==but_def)
				{
					myButtons.deleteAllButtons();
					//lista();
					menu_def();
					redraw = true;
				} else if (pressed_button==but_card)
				{
					myButtons.deleteAllButtons();
					menu_cards();
					redraw = true;
				} else tone(PIN_BUZZER,WRONG_TONE,100);
			}
		}
	}
}

void draw_switch(int x, int y, boolean sw)
{
	int r = 8;
	int i;
	int color1, color2;
	
	if (sw)
	{
		color1 = 0x7BEF; //Grey
		color2 = 0x5FED;
	} else
	{
		color2 = 0x7BEF; //Grey
		color1 = 0x5FED;
	}
	
	utft.setColor(color1);
	utft.fillRoundRect(x, y-3, x+20, y+3);

	if (sw)
	{
		for (i=0; i<=20 ;i++)
		{
			utft.setColor(0x0000);
			utft.drawCircle(x+i-1, y, r);
			utft.setColor(color1);
			utft.fillRoundRect(x, y-3, x+20, y+3);
			utft.fillCircle(x+i, y, r);
			//delay(5);
		}
	} else
	{
		for (i=20; i>=0 ;i--)
		{
			utft.setColor(0x0000);
			utft.drawCircle(x+i+1, y, r);
			utft.setColor(color1);
			utft.fillRoundRect(x, y-3, x+20, y+3);
			utft.fillCircle(x+i, y, r);
			//delay(5);
		}
		i++;
	}
	utft.setColor(color2);
	utft.fillRoundRect(x, y-3, x+20, y+3);
	utft.fillCircle(x+i, y, r);
}

void setup()
{
	//pinMode(PE0, OUTPUT);
	//digitalWrite(PE0, HIGH);
	//init_EEPROM();
	utft.InitLCD();
	utft.clrScr();

	cards = EEPROM.read(CARDS_ADDR);
	if (cards>0)
	{
		for (int i=0;i<cards;i++)
		{
			card[i] = EEPROMReadlong(i*4);
		}
	}
	
	time_open = EEPROM.read(TIME_OPEN_ADDR);
	alarm = EEPROM.read(ALARM_ADDR);
	admin_pass = EEPROMReadlong(ADMIN_PASS_ADDR);
	alarm_delay = EEPROM.read(ALARM_DELAY_ADDR);
	
	myTouch.InitTouch();
	myTouch.setPrecision(PREC_MEDIUM);
	
	//myButtons.setTextFont(BigFont);
	myButtons.setTextFont(hallfetica_normal);
	myButtons.setSymbolFont(hallfetica_normal);

	Serial.begin(9600);		// Initialize serial communications with the PC
	SPI.begin();			// Init SPI bus
	mfrc522.PCD_Init();		// Init MFRC522
	//ShowReaderDetails();	// Show details of PCD - MFRC522 Card Reader details
	//Serial.println(F("Scan PICC to see UID, type, and data blocks..."));
	
	cli();//stop interrupts
	
	//set timer1 interrupt at 1Hz
	TCCR1A = 0;// set entire TCCR1A register to 0
	TCCR1B = 0;// same for TCCR1B
	TCNT1  = 0;//initialize counter value to 0
	// set compare match register for 1hz increments
	OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
	// turn on CTC mode
	TCCR1B |= (1 << WGM12);
	// Set CS12 and CS10 bits for 1024 prescaler
	TCCR1B |= (1 << CS12) | (1 << CS10);
	// enable timer compare interrupt
	TIMSK1 |= (1 << OCIE1A);
	
	/*
	pinMode(21, INPUT_PULLUP);
	// Global Enable INT2 interrupt
	EIMSK  |= ( 1 << INT2);
	// Signal LOW triggers interrupt
	EICRA  |= ( 1 << ISC20);
	EICRA  |= ( 1 << ISC21);
	*/
	
	sei();
	
	pinMode(13, OUTPUT);

}

void loop()
{
	while (!myTouch.dataAvailable())
	{
		worker();
	}
	utft.lcdOn();
	tone(PIN_BUZZER,KEY_TONE,100);
	seconds = 0;
	sleep = false;

	uint32_t pass = draw_keypad(4);
	if (pass != admin_pass)
	{
		tone(PIN_BUZZER,WRONG_TONE,100);
		return;
	}
	
	tone(PIN_BUZZER,KEY_TONE,100);
	delay(100);
	tone(PIN_BUZZER,WRONG_TONE,100);
	delay(100);
	tone(PIN_BUZZER,KEY_TONE,100);
	delay(100);
	tone(PIN_BUZZER,WRONG_TONE,100);
	delay(100);
	tone(PIN_BUZZER,KEY_TONE,100);
	delay(100);
	tone(PIN_BUZZER,WRONG_TONE,100);
	delay(100);
	
	main_menu();
}

void worker()
{
	if (mfrc522.PICC_IsNewCardPresent()) {
		if (sleep) utft.lcdOn();
		// Select one of the cards
		if (mfrc522.PICC_ReadCardSerial()) {
			tone(PIN_BUZZER,KEY_TONE,100);
			uint32_t card_uid=get_card_uid(mfrc522.uid.uidByte, mfrc522.uid.size);
			//Serial.println(card_uid);
					
			sprintf(stCurrent, "%lu", card_uid);

			uint8_t *temp = utft.getFont();
			utft.setFont(CalibriBold32x48);
			utft.setBackColor(VGA_BLACK);

			utft.setFont(DejaVuSans18);
			boolean card_aceite = false;
			for (int i=0; i<cards; i++)
			{
				if (card_uid == card[i])
				{
					card_aceite = true;
					utft.setColor(CUS);
					utft.print(stCurrent, CENTER, (240-48)/2);
					//uint8_t txt_size = utft.getStringWidth("Autorizado");
					//utft.print("Autorizado", (320-txt_size)/2, 50);
					//ACTIVA PORTA
					delay(time_open*1000);
					break;
				}
			}
			if (!card_aceite)
			{
				utft.setColor(CUSTOM_ORANGE);
				//uint8_t txt_size = utft.getStringWidth("Não Autorizado");
				//utft.print("Não Autorizado", (320-txt_size)/2, 50);
				tone(PIN_BUZZER,WRONG_TONE,300);
				delay(1000);
			}
			utft.clrScr();
			utft.setFont(temp);
			if (sleep) utft.lcdOff();
		}
	}
}

ISR(INT2_vect) {
	// check the value again - since it takes some time to
	// activate the interrupt routine, we get a clear signal.
	cli();
	if (seconds == TIMEOUT+1) utft.lcdOn();
	seconds = 0;
	Serial.println("EXT2 Trig!");
	//if (digitalRead(21) == LOW) key_pressed = true;
	key_pressed = true;
	sei();
}

ISR(TIMER1_COMPA_vect) {//timer1 interrupt 1Hz toggles pin 13 (LED)
	//generates pulse wave of frequency 1Hz/2 = 0.5kHz (takes two cycles for full wave- toggle high then toggle low)
	toggle1 = !toggle1;

	digitalWrite(13,toggle1 ? HIGH:LOW);
	
	if (seconds < TIMEOUT)
	{
		seconds++;
	}	
	else if (seconds == TIMEOUT)
	{
		seconds++;
		utft.lcdOff();
		myButtons.deleteAllButtons();
		utft.clrScr();
		sleep = true;
	}
}