// ===== Morse Messenger for Spy Communication =====

const int sensorPin=2;   //PIR signal
const int RPin     =4;   //LED
const int PiezoPin =6;   //buzzer

const unsigned long DASH_THRESHOLD  =1200;  //pulse >= this is a dash
const unsigned long LETTER_THRESHOLD=1800;  //silence this long ends a letter
const unsigned long MESSAGE_END_GAP =7000;  //silence this long ends the message

const int UNIT=300;                //1 Morse unit in ms

const int MAX_SYMBOLS=5;                    //longest Morse character(digits)
char buffer[MAX_SYMBOLS+1];                 //+1 for '\0'
int count=0;
bool ignoring=false;                        //true after an overflow, until the user pauses

const int MAX_MESSAGE=20;
char message[MAX_MESSAGE+1];
int msgLen=0;

int lastState=0;
unsigned long startTime=0;
unsigned long lastFallTime=0;

// ---- Morse table ----
const char* codes[]=
{
  ".-",    // A
  "-...",  // B
  "-.-.",  // C
  "-..",   // D
  ".",     // E
  "..-.",  // F
  "--.",   // G
  "....",  // H
  "..",    // I
  ".---",  // J
  "-.-",   // K
  ".-..",  // L
  "--",    // M
  "-.",    // N
  "---",   // O
  ".--.",  // P
  "--.-",  // Q
  ".-.",   // R
  "...",   // S
  "-",     // T
  "..-",   // U
  "...-",  // V
  ".--",   // W
  "-..-",  // X
  "-.--",  // Y
  "--..",  // Z
  "-----", // 0
  ".----", // 1
  "..---", // 2
  "...--", // 3
  "....-", // 4
  ".....", // 5
  "-....", // 6
  "--...", // 7
  "---..", // 8
  "----."  // 9
};
const char alphabets[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const int NUM_CODES=sizeof(codes)/sizeof(codes[0]);


char decode(char* pattern)	//decoding the message
{
  for(int i=0;i<NUM_CODES;i++)
  {
    if(strcmp(pattern,codes[i])==0)	//if code matches returns the correspnding alphbets
    {
      return alphabets[i];
    }
  }
  return '?';
}


void beep(int Time)		//signal playback
{
  digitalWrite(RPin,HIGH);
  tone(PiezoPin,3000);
  delay(Time);
  digitalWrite(RPin,LOW);
  noTone(PiezoPin);
  delay(UNIT);          //1-unit gap after every symbol
}

void playLetter(char c)
{
  for(int i=0;i<NUM_CODES;i++)
  {
    if(alphabets[i]==c)
    {
      const char* morse=codes[i];
      for(int j=0;morse[j]!='\0';j++)
      {
        if(morse[j]=='.')
        {
          beep(1*UNIT);   //dot =1 unit
        }
        else
        {
          beep(3*UNIT);   //dash=3 units
        }
      }
      delay(2 * UNIT);    //beep() already added 1, so 1+2=3-unit letter gap
      return;
    }
  }
}

void playMessage()
{
  Serial.print("Transmitting Message: ");
  Serial.println(message);
  for(int i=0;i<msgLen;i++)
  {
    Serial.println(message[i]);
    playLetter(message[i]);
  }
}


void setup()
{
  Serial.begin(9600);
  pinMode(sensorPin,INPUT);
  pinMode(RPin,OUTPUT);
  pinMode(PiezoPin,OUTPUT);
}


void loop()
{
  int state=digitalRead(sensorPin);

  if(state==1)	//Live feedback: LED + buzzer
  {
    digitalWrite(RPin,HIGH);
    tone(PiezoPin,3000);
  }
  else
  {
    digitalWrite(RPin,LOW);
    noTone(PiezoPin);
  }

  //1. Letter finished; decode and store it
  if(state==0 && count>0 && millis()-lastFallTime>LETTER_THRESHOLD)
  {
    char c=decode(buffer);
    Serial.print(buffer);
    Serial.print("\t-->\t");

    if(c=='?')
    {
      Serial.println("Unknown Code");
    }
    else
    {
      Serial.println(c);
      
      if(msgLen<MAX_MESSAGE)
      {
        message[msgLen++]=c;
        message[msgLen]='\0';
      }
      else
      {
        Serial.println("Message memory full, letter not stored.");
      }
    }
    count=0;
    buffer[0]='\0';
  }

  //2. Message finished: Playback: -... --- --- -- !!
  if(state==0 && count==0 && msgLen>0 && millis()-lastFallTime>MESSAGE_END_GAP)
  {
    playMessage();
    msgLen=0;
    message[0]='\0';
    lastState=digitalRead(sensorPin);   //resync: playback blocked loop() for seconds
    lastFallTime = millis();
  }

  //3. Recovery after an overflow: wait for a pause, then accept input again
  if(ignoring && state==0 && millis()-lastFallTime>LETTER_THRESHOLD)
  {
    ignoring = false;
    Serial.println("Ready for the next letter.");
  }

  //4. Rising edge: hand arrived
  if(state==1 && lastState==0)
  {
    startTime=millis();
  }

  //5. Falling edge: hand left
  if(state==0 && lastState==1)
  {
    unsigned long duration=millis()-startTime;
    lastFallTime=millis();

    if(!ignoring)
    {
      if(count>=MAX_SYMBOLS)
      {
        Serial.println("Too many symbols (max 5). Ignoring input until you pause.");
        count=0;
        buffer[0]='\0';
        ignoring=true;
      }
      else
      {
        buffer[count++]=(duration<DASH_THRESHOLD)? '.' : '-';
        buffer[count]='\0';
      }
    }
  }

  lastState = state;
  delay(10);
}