/*
Arduino LED Tester
DPIN--39R--+--10R---TESTLED---GND
           |      |         |
          470u    ATOP     ABOT
           |
          GND

 Measures LED characteristics by charging up the cap to deliver target current and find forward voltage
 From target current, we can calculate R to be used with a design supply voltage and a matching part number.
  */
#include <LiquidCrystal.h>
//pin defs to suit LCD Shield
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
//lcd update interval
#define LCDINT 500
//key check interval
#define KEYINT 200
//pin for buttons
#define KEYPIN A0
//button constants
#define btnRIGHT  6
#define btnUP     5
#define btnDOWN   4
#define btnLEFT   3
#define btnSELECT 2
#define btnNONE   (-1)

//Globals for display
long itest=10;    //test current, in mA
long vset =14000;  //display voltage in mV
long vled,vrr,irr,pset;  //LED voltage, Resistor voltage, resistor current, display power
//resistors in Jaycar 1/2 W range, part nos start at RR0524 for 10R
#define RCOUNT 121
long rvals[]={10,11,12,13,15,16,18,20,22,24,27,30,33,36,39,43,47,51,56,62,68,75,82,91,100,110,120,130,150,160,180,200,220,240,270,300,330,360,390,430,470,510,560,620,680,750,820,910,1000,1100,1200,1300,1500,1600,1800,2000,2200,2400,2700,3000,3300,3600,3900,4300,4700,5100,5600,6200,6800,7500,8200,9100,10000,11000,12000,13000,15000,16000,18000,20000,22000,24000,27000,30000,33000,36000,39000,43000,47000,51000,56000,62000,68000,75000,82000,91000,100000,110000,120000,130000,150000,160000,180000,200000,220000,240000,270000,300000,330000,360000,390000,430000,470000,510000,560000,620000,680000,750000,820000,910000,1000000};
long lastlcd=0;  //time of last lcd update
long lastkey=0;  //time of last key check
int lcdflash=0;  //lcd flashing phase variable
long pdes;
long rval;      //calculated resistor value for display
long rindex;    //index of selected resistor in rvals[]
int pwmout=0;    //pwm output of current driver
int rvalid=0;    //flag if resistor value is valid 
//pins for test interface
#define ATOP A2
#define ABOT A3
#define DPIN 3
#define OSAMP 16

void setup() {
  lcd.begin(16, 2);        //lcd 
  pinMode(DPIN,OUTPUT);    //pwm pin
}

void loop() {
  long atop,abot,arr;      //analog sample values
  rvalid=0;                //set flag to not valid
  atop=analogoversample(ATOP,OSAMP)/OSAMP;
  abot=analogoversample(ABOT,OSAMP)/OSAMP;  
  arr=atop-abot;      //this is the analog value across the 10R sense resistor
  if(arr<0){arr=0;}    //sanity check
  vled=abot*5000/1023;  //5000mV=1023 => voltage across LED
  vrr=arr*5000/1023;    //voltage across sense resistor
  irr=vrr/10;     //led and resistor current in mA (cos it's a 10R resistor)
    
  if(irr<itest){pwmout++;if(pwmout>255){pwmout=255;}}    //ramp up current if too low
  if(irr>itest){pwmout--;if(pwmout<0){pwmout=0;}}        //ramp down if too high
  if(irr>24){pwmout=pwmout-5;if(pwmout<0){pwmout=0;}}    //ramp down quick if too too high
  if(irr>itest*3){pwmout=pwmout-5;if(pwmout<0){pwmout=0;}}    //ramp down quick if too too high
  analogWrite(DPIN,pwmout);                              //output new PWM
  rval=(vset-vled)/itest;                                //mV/mV => ohms resistance of display resistor
  for(int i=0;i<RCOUNT;i++){                             //find next highest E24 value
    if(rvals[i]>=rval){rindex=i;rval=rvals[rindex];i=RCOUNT+1;rvalid=1;}
  }
  if(abs(irr-itest)>(itest/5)+1){rvalid=0;}              //has current settled within 20%?
  if(vled>vset){rvalid=0;}                               //if vled>vset, no valid resistor exists
  pset=0;        //work out dissipation in ballast resistor if valid)
  if(rvalid){pset=itest*itest*rval;}      //this will be in microwatts (milliamps squared)
  
  if(millis()-lastlcd>LCDINT){      //check if display due to be updated
    lastlcd=millis();
    dolcd();        //update display
    lcdflash=1-lcdflash;  //toggle flash variable
    }
  if(millis()-lastkey>KEYINT){    //check if keys due to be checked
    lastkey=millis();
    dobuttons();
    }
    delay(1);
}        //end of loop

void dolcd(){
  lcd.setCursor(0,0);    //first line
  //milliamps
  if(lcdflash||rvalid){        //show if correct if on flashing phase
    if(itest>9){lcd.write(((itest/10)%10)+'0');}else{lcd.write(' ');}    //blank tens if zero
    lcd.write((itest%10)+'0');
  }
  else{      //blank if not
    lcd.write(' ');lcd.write(' ');
  }
    lcd.write('m');lcd.write('A');lcd.write(' ');
  //VLED
  lcd.write(((vled/1000)%10)+'0');
  lcd.write('.');
  lcd.write(((vled/100)%10)+'0');
  lcd.write('V');lcd.write(' ');

  //actual LED current
  if(irr>9){lcd.write(((irr/10)%10)+'0');}else{lcd.write(' ');}    //blank tens if zero
  lcd.write((irr%10)+'0');
  lcd.write('m');lcd.write('A');lcd.write(' ');
  if((pset>499999)&&(lcdflash)){lcd.write('P');}else{lcd.write(' ');} //flash P if power above 1/2 watt
  
  lcd.setCursor(0,1);   //second line
  //
  if(vset>9999){lcd.write(((vset/10000)%10)+'0');}else{lcd.write(' ');}    //tens of vset, blank if zero
  lcd.write(((vset/1000)%10)+'0');      //units of vset
  lcd.write('V');lcd.write(' ');
  
  if(rvalid){
    lcdprintrval(rval);  //resistor value (4 characters)
    lcd.write(' ');
    lcdprintpartno(rindex);    //resistor part no (6 characters)
    if(pset>499999){lcd.write('!');}else{lcd.write(' ');} //show ! if power above 1/2 watt
  }else{
    lcd.write(' ');
    lcd.write('-');
    lcd.write('-');
    lcd.write('-');
    lcd.write(' ');
    lcd.write('-');
    lcd.write('-');
    lcd.write('-');
    lcd.write('-');
    lcd.write('-');
    lcd.write('-');
    lcd.write(' ');
  }    
}

void lcdprintpartno(int index){
  //part number
  lcd.write('R');
  lcd.write('R');
  lcd.write('0');
  lcd.write((((index+524)/100)%10)+'0');      //part no's start at RR0524 for 10R
  lcd.write((((index+524)/10)%10)+'0');
  lcd.write((((index+524))%10)+'0');
}

void lcdprintrval(long rval){        //print a value in 10k0 format, always outputs 4 characters
  	long mult=1;
	long modval;
	if(rval>999){mult=1000;}
	if(rval>999999){mult=1000000;}
	modval=(10*rval)/mult;		//convert to final format, save a decimal place
	if(modval>999){		//nnnM
		lcd.write(((modval/1000)%10)+'0');
		lcd.write(((modval/100)%10)+'0');
		lcd.write(((modval/10)%10)+'0');
		lcdprintmult(mult);
			}else{
	if(modval>99){		//nnMn
		lcd.write(((modval/100)%10)+'0');
		lcd.write(((modval/10)%10)+'0');
		lcdprintmult(mult);
		lcd.write(((modval)%10)+'0');
			}else{	//_nMn
		lcd.write(' ');
		lcd.write(((modval/10)%10)+'0');
		lcdprintmult(mult);
		lcd.write(((modval)%10)+'0');
    }
  }
}
void lcdprintmult(long mult){      //helper function to print multiplier
	switch (mult){
	case 1:	lcd.print('R');break;
	case 1000:	lcd.print('k');break;
	case 1000000:	lcd.print('M');break;
	default:	lcd.print('?');break;
  }
}
int read_LCD_buttons(){
  int adc_key_in    = 0;
  adc_key_in = analogRead(KEYPIN);      // read the value from the sensor 
  delay(5); //switch debounce delay. Increase this delay if incorrect switch selections are returned.
  int k = (analogRead(KEYPIN) - adc_key_in); //gives the button a slight range to allow for a little contact resistance noise
  if (5 < abs(k)) return btnNONE;  // double checks the keypress. If the two readings are not equal +/-k value after debounce delay, it tries again.
  // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
  // we add approx 50 to those values and check to see if we are close
  if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
  if (adc_key_in < 50)   return btnRIGHT;  
  if (adc_key_in < 195)  return btnUP; 
  if (adc_key_in < 380)  return btnDOWN; 
  if (adc_key_in < 555)  return btnLEFT; 
  if (adc_key_in < 790)  return btnSELECT;   
  return btnNONE;  // when all others fail, return this...
}
void dobuttons(){      //updates variables. debounces by only sampling at intervals
    int key;
    key = read_LCD_buttons();
    if(key==btnLEFT){itest=itest-1;if(itest<1){itest=1;}}
    if(key==btnRIGHT){itest=itest+1;if(itest>20){itest=20;}}
    if(key==btnUP){vset=vset+1000;if(vset>99000){vset=99000;}}
    if(key==btnDOWN){vset=vset-1000;if(vset<0){vset=0;}}   
}
long analogoversample(int pin,int samples){      //read pin samples times and return sum
  long n=0;
  for(int i=0;i<samples;i++){
    n=n+analogRead(pin);
  }
  return n;
}

