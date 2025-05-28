#define F_CPU 7372800UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>



#include "myLCD20x4.h"

#define KEYPAD_DDR DDRC
#define KEYPAD_PORT PORTC
#define KEYPAD_PIN PINC

uint8_t scan_code[4] = {0x0E, 0x0D, 0x0B, 0x07};
uint8_t ascii_code [4][4] =
{
	{'A','1','2','3'},
	{'B','4','5','6'},
	{'C','7','8','9'},
	{'D','*','0','#'}
};



uint8_t key;
uint8_t checkpad(){
	uint8_t i,j,keyin;
	for (i = 0; i < 4; i++)
	{
		KEYPAD_PORT = 0xFF - (1<<(i+4));
		keyin = KEYPAD_PIN & 0x0F;
		if (keyin != 0x0F)
		{
			for(j=0;j<4;j++)
			{
				if(keyin == scan_code[j])
				{return ascii_code[j][i];}
			}
		}
	}
	return 0;
}

uint16_t adc_val;
uint16_t read_adc(unsigned int adc_channel)
{
	ADMUX |= adc_channel;
	ADCSRA |=(1<<ADSC);
	while(bit_is_clear(ADCSRA,ADIF))
	{
		;
	}
	
	loop_until_bit_is_set(ADCSRA,ADIF);
	return ADCW;
}

void LOCK(); 

void UNLOCK();




char PassWord[5] = "1234"; //array save password
char RefPass[5]; // array save password that input from 4x4 keypad 
uint8_t lengthP = 1 ; //lenght of Refpass that will increase when have input from keypad
uint8_t Row =1 , Col =1 ; //using for move_LCD



//flag
volatile int8_t wrong = 0; //flag for limit of wrong input Refpass
volatile int8_t flag = 1; //mode flag. current have 2 mode Lock() and Unlock().
volatile int8_t checkP = 1;//flag for printf "Nhap mat khau" once every time that change mode

volatile int8_t counterLCD = 0; //using in Timer for freezing lcd every time that wrong input Refpass == 3 
volatile int8_t counterLock = 0;//using in Timer for auto change mode Unlock to Lock when in Unlock mode

 
volatile int8_t ourLock = 0 ; //flag indicate that Lock have unlock yet?
volatile int8_t visikey = 0; //flag for print '*' -> 'char' on LCD.

//Config TIME
const int8_t autoLockTime = 3;
const int8_t FreezingLCD = 10;

int main(void)
{
	KEYPAD_DDR = 0xF0;
	KEYPAD_PORT = 0xFF;
	
	DDRD = 0b00000001;
	PORTD = 0b00000001;
	
	DDRB = 0b10000001;
	DDRE = 0b00000100;
	
	
	DDRF = 0b11111110;
	PORTF = 0b00000001;
	
	ADMUX |= (1<<REFS0);
	ADCSRA = 0b10000111;
	

	EICRA = 0b00000011;
	EIMSK = 0b00000001;
	
	TCCR1B = 0b0000101;
	TCNT1 =  58336	;
	TIMSK = (1<<TOIE1);
	
	sei();
	
	
	init_LCD();
	clr_LCD();
	
	
	
	while (1)
	{

		if(flag == 1)
		{
			LOCK();
		}
		
		
		if(flag == 2)
		{
			UNLOCK();
		}
	}
}


void LOCK()
{
	clr_LCD();
	move_LCD(Row,Col);
	PORTE &= ~0b00000100;
	PORTB = (1<<PB7);
	while(1){
		
		key = checkpad();
		_delay_ms(150);
		
		if(flag != 1)
		{
			break;
		}
		
		if(checkP == 1)
		{
			putStr_LCD("Nhap Mat Khau"); //Print on LCD "input password"
			Row = 3;
			checkP = 0;
		}
		
		move_LCD(Row,Col);
		
		if(key)
		{	
			/////switch key() start
			switch(key)
			{
				case 'A':
				{
					memset(RefPass, 0, sizeof(RefPass)); //reset Refpass to empty array
					{
						lengthP = 1;
						Row = 3;
						Col = 1;
					}
					clear_line(3);
					_delay_ms(20);
					continue;
				}
				
				case 'B':
				{
					clear_line(2);
					visikey = !visikey;
					move_LCD(3,1);
					for (uint8_t i = 0; i < lengthP - 1; i++)
					{
						if (visikey)
						{putChar_LCD(RefPass[i]);}
						else
						{putChar_LCD('*');}
						_delay_ms(20);
					}
					continue;
				}
				case '#':
				{
					
					//
					if (strcmp(RefPass,PassWord) == 0)
					{
						PORTD &= ~0b00000001;
						_delay_ms(50);
						PORTD = 0b00000001;
						_delay_ms(50);
						continue;
					}
					else if(strcmp(RefPass,PassWord) != 0)
					{
						wrong++;
						memset(RefPass, 0, sizeof(RefPass));
						lengthP = 1;
						
						clear_line(3);
						Col = 1;
						
						_delay_ms(100);
						move_LCD(Row,Col);
						putStr_LCD("Mat Khau Sai"); //print on LCD "Wrong password"
						move_LCD(4,1);
						printf_LCD("lan nhap con lai: %d", 3-wrong); //print on LCD "input trial left"
						_delay_ms(50);
						clear_line(3);
						if(wrong >=3)
						{
							counterLCD = 0;
							clr_LCD();
							home_LCD();
							putStr_LCD("MAT KHAU SAI 3 LAN");//print on LCD "Wrong 3 times"
							move_LCD(3,1);
							putStr_LCD("KHOA MAN HINH");//Freezing-LOCK LCD
							while(counterLCD < FreezingLCD)
							{
								PORTB = 0b00000001;
							};
							clr_LCD();
							home_LCD();
							
							
							{
								Row = 1;
								Col = 1;
								checkP = 1;
								wrong = 0;
							}
							return ;
						}
						continue;
					}
				}
				
				default:
					{
						if(key >= '0' && key <= '9')
						{
							if(lengthP >= 5)
							{
								continue;;
							}
							if (visikey == 0){putChar_LCD('*');}
							else if (visikey == 1){putChar_LCD(key);};
							_delay_ms(50);
							
							
							if (lengthP - 1 < 4) {
								RefPass[lengthP - 1] = (char)key;
								RefPass[lengthP] = '\0';
							}
							Col++;
							lengthP++;
							if (Col > 20) Col = 1;
						}
						continue;
					}
			}
		/////switch key() end	
		}
		
	}
	
}

void UNLOCK()
{
	clr_LCD();
	move_LCD(Row,Col);
	_delay_ms(20);
	PORTE = 0b00000100;

		while(1)
		{
			if(flag != 2)
			{
				break;
			}
			
			if(checkP == 1)
			{
				putStr_LCD("UNLOCK!!!");
				_delay_ms(20);
				Row = 3;
				move_LCD(Row,Col);
				putStr_LCD("1: Doi Mat Khau");//Change password
				Row = 4;
				move_LCD(Row,Col);
				putStr_LCD("N: Khoa Cua");//Lock Door
				checkP = 0;
			}
			key = checkpad();
			_delay_ms(150);
			if(key)
			{
				/////switch key 1* start
				switch(key)
				{
					case '*':
					{
						
						PORTD &= ~0b00000001;
						_delay_ms(50);
						PORTD = 0b00000001;
						_delay_ms(50);
						return;
					}
					
					case '1':	
						{
							memset(RefPass, 0, sizeof(RefPass));
							{
								checkP = 1;
								lengthP = 1;
								Row = 1 ; Col = 1;
								ourLock = 0;
							}
							clr_LCD();
							move_LCD(Row,Col);
							_delay_ms(50);
							while(1)
							{
								key = checkpad();
								move_LCD(Row,Col);
								_delay_ms(150);
								
								if(flag != 2)
								{
									flag = 2;
									break;
								}
								
								if(checkP == 1)
								{
									putStr_LCD("Nhap Mat Khau Moi");//input new pass
									_delay_ms(50);
									Row = 3;
									checkP = 0;
									continue;
								}
								///// switch key 2* start
								switch (key)
								{
									case 'A':
									{
										memset(RefPass, 0, sizeof(RefPass));
										{
											lengthP = 1;
											Row = 3;
											Col = 1;
										}
										clear_line(3);
										_delay_ms(20);
										continue;
									}
									
									case '*':
										{
											memset(RefPass, 0, sizeof(RefPass));
											lengthP = 1;
											checkP = 1; 
											Row = 1; Col = 1;
											clr_LCD();
											move_LCD(Row,Col);
											flag = 0;
											break;
										}
									case '#':
									{
										if(lengthP-1 < 4 )
										{
											memset(RefPass, 0, sizeof(RefPass));
											{
												Row = 3; Col = 1;
												lengthP = 1;
											}
											clear_line(3);
											putStr_LCD("Mat Khau Qua Ngan");
											_delay_ms(200);
											clear_line(3);
											_delay_ms(20);
											continue;
										}
										if(lengthP-1 == 4)
										{
											for (int i = 0; i < 4; i++)
											{
												PassWord[i] = RefPass[i];
											}
											memset(RefPass, 0, sizeof(RefPass));
											lengthP = 1;
											
											clr_LCD();
											home_LCD();
											putStr_LCD("DA DOI MAT KHAU");//Changed pass
											_delay_ms(1000);
											Row = 1; Col = 1;
											clr_LCD();
											move_LCD(Row,Col);
											checkP = 1;
											ourLock = 1;
											flag = 0;
											break;
										}
									}
								
									case 'B':
									{
										clear_line(2);
										visikey = !visikey;

										move_LCD(3,1);
										for (uint8_t i = 0; i < lengthP - 1; i++)
										{
											if (visikey)
											putChar_LCD(RefPass[i]);
											else
											putChar_LCD('*');
											_delay_ms(50);
										}
										_delay_ms(50);
										continue;
									}
									default:
									{
										if(key >= '0' && key <= '9')
										{
											if(lengthP >= 5)
											{
												continue;;
											}
											if (visikey == 0){putChar_LCD('*');}
											else if (visikey == 1){putChar_LCD(key);};
											_delay_ms(50);
										
											
											if (lengthP - 1 < 4) {
												RefPass[lengthP - 1] = (char)key;
												RefPass[lengthP] = '\0';
											}
											Col++;
											lengthP++;
											if (Col > 20) Col = 1;
											continue;
										}
									}

								}
							///// switch key 2* end
							}
							
						}
					default:
					{
						continue;
					}
					}
				/////switch key 1* end	
				}
				
			}
}





ISR(INT0_vect)
{
	flag ^= 3;
	if(flag == 2)
	{
		memset(RefPass, 0, sizeof(RefPass));
		ourLock = 1;
		counterLock = 0;
		checkP = 1;
		Row = 1; Col = 1;
		lengthP = 1;
	}
	if (flag == 1)
	{ 
		memset(RefPass, 0, sizeof(RefPass));
		ourLock = 0;
		counterLock = 0;
		checkP = 1;
		Row = 1; Col = 1;
		wrong = 0;
		lengthP = 1;
	}
}




ISR(TIMER1_OVF_vect)
{
	TCNT1 =  58336;

	if(wrong < 3)
	{
		if(ourLock == 1)
		{
			
			if (counterLock >= autoLockTime)
			{
				PORTD &= ~0b00000001;
				_delay_ms(50);
				PORTD = 0b00000001;
				_delay_ms(50);
			}
			counterLock++;
		}
	}else if(wrong >= 3)
	{
		
		counterLCD++;
		if(counterLCD > FreezingLCD)
		{
			PORTB = 0b10000001;
			counterLCD = 0;
			wrong = 0;
		}
	}
	
	
	
}


