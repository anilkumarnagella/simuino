//================================================
//  Developed by Benny Saxen, ADCAJO
//================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <math.h> 
#include <termios.h>
#include <unistd.h>
#include <ncurses.h>

#define FREE   0
#define RX     3
#define TX     4

int   row,col;
int   graph_x = 10,graph_y = 10;
int   digPinPos[14];
int   anaPinPos[6];
char  appName[80];
int   analogPin[STEP_MAX][6];
int   digitalPin[STEP_MAX][14];
int   interrupt[STEP_MAX][2];
int   interruptMode[2];
int   digitalMode[100];
int   paceMaker = 0;
int   baud = 0;
int   error = 0;
int   logging = YES;
char  logBuffer[STEP_MAX][100];
int   logSize = 1;
int   serialSize = 1;
int   serialMode = OFF;
char  serialBuffer[100][100];
int   rememberNewLine;
char  textPinModeIn[14][80];
char  textPinModeOut[14][80];
char  textDigitalWriteLow[14][80];
char  textDigitalWriteHigh[14][80];
char  textAnalogWrite[14][80];
char  textAnalogRead[14][80];
char  textDigitalRead[14][80];
int   scenAnalog    = 0;
int   scenDigital   = 0;
int   scenInterrupt = 0;

int   conn;

// Configuration default values
int   confDelay   = 100;
int   confLogLev  =   1;
int   confLogFile =   0;

WINDOW *uno,*ser,*slog,*com;
static struct termios orig, nnew;
static int peek = -1;


//====================================
int __nsleep(const struct timespec *req, struct timespec *rem)  
//====================================
{  
  struct timespec temp_rem;  
  if(nanosleep(req,rem)==-1)  
    __nsleep(rem,&temp_rem);  
  else  
    return 1;  
}  

//====================================   
int msleep(unsigned long milisec)  
//====================================
{  
  struct timespec req={0},rem={0};  
  time_t sec=(int)(milisec/1000);  
  milisec=milisec-(sec*1000);  
  req.tv_sec=sec;  
  req.tv_nsec=milisec*1000000L;  
  __nsleep(&req,&rem);  
  return 1;  
}  

//====================================
void iDelay(int ms)
//====================================
{
  msleep(ms);
}

//====================================
void show(WINDOW *win)
//====================================
{
  wrefresh(win);
  iDelay(confDelay);
}

//====================================
void showError(const char *m, int value)
//====================================
{
  error = 1;
  wmove(com,0,0);
  wprintw(com,"                                       ");
  wmove(com,0,0);
  if(value == -1)
    wprintw(com,"Error %s",m);
  else
    wprintw(com,"Error %s %d",m,value);

  show(com);
}

//====================================
void resetFile(const char *filename)
//====================================
{
  FILE *out;
  time_t lt;

  out = fopen(filename,"w");
  if(out == NULL)
    {
      showError("Unable to reset file ",-1);
    }
  else
    {
      lt = time(NULL);
      fprintf(out,"# Simuino Log File %s",ctime(&lt));
    }

  fclose(out);
}

//====================================
void logFile(char *m)
//====================================
{
  FILE *out;

  out = fopen("log.txt","a");
  if(out == NULL)
    {
      showError("Unable to open log.txt",-1);
    }
  else
    {
      fprintf(out,"%s\n",m);
    }

  fclose(out);
}

//====================================
void wLog(const char *p, int value1, int value2)
//====================================
{
  int i;
  char temp[100],temp2[100];

  sprintf(temp," ");

  if(value2 > -2)
    {
        if(stepStep)
	  sprintf(temp,">%d,%d ",nloop,timeFromStart);
	else
	  sprintf(temp," %d,%d ",nloop,timeFromStart);
    }
  strcat(temp,p);
 

  if(value1 > -1)
    {
      sprintf(temp2," %d",value1);
      strcat(temp,temp2);
    }
  
  if(value2 > -1)
    {
      sprintf(temp2," %d",value2);
      strcat(temp,temp2);
    }

  if(error == 1)
    {
      temp[0]='*';
      error = 0;
    }

  for(i=logSize;i>0;i--)strcpy(logBuffer[i],logBuffer[i-1]);
  strcpy(logBuffer[0],temp);

  if(confLogFile==YES)logFile(temp);

  wclear(slog);
  for(i=0;i<logSize;i++)
    {
      wmove(slog,i,0);
      wprintw(slog,"%s",logBuffer[i]);
    } 
  wrefresh(slog);
}

//====================================
void passTime()
//====================================
{
  int i;

  if(timeFromStart >= STEP_MAX)
    {
      wLog("Log Limit reached",STEP_MAX,-1); 
      return;
    }

  timeFromStart++;


  i = timeFromStart;

  //  if(interruptMode[0] == LOW && interrupt[i][0] == 0)
  //    {
  //      if(confLogLev > 0)wLog("InterruptLOW",0,-1);
  //      interrupt0();
  //    }

  if(interruptMode[0] == RISING && interrupt[i][0] == 1 && interrupt[i-1][0] == 0)
    {
      if(confLogLev > 0)wLog("InterruptRISING",0,-1);
      interrupt0();
    }
  
  if(interruptMode[0] == FALLING && interrupt[i][0] == 0 && interrupt[i-1][0] == 1)
    {
      if(confLogLev > 0)wLog("InterruptFALLING",0,-1);
      interrupt0();
    }

  if(interruptMode[0] == CHANGE && interrupt[i][0] != interrupt[i-1][0])
    {
      if(confLogLev > 0)wLog("InterruptCHANGE",0,-1);
      interrupt0();
    }


  if(interruptMode[1] == RISING && interrupt[i][1] == 1 && interrupt[i-1][1] == 0)
    {
      if(confLogLev > 0)wLog("InterruptRISING",1,-1);
      interrupt1();
    }
  
  if(interruptMode[1] == FALLING && interrupt[i][1] == 0 && interrupt[i-1][1] == 1)
    {
      if(confLogLev > 0)wLog("InterruptFALLING",1,-1);
      interrupt1();
    }

  if(interruptMode[1] == CHANGE && interrupt[i][1] != interrupt[i-1][1])
    {
      if(confLogLev > 0)wLog("InterruptCHANGE",1,-1);
      interrupt1();
    }

}

//====================================
void wLogChar(const char *p, const char *value1, int value2)
//====================================
{
  int i;
  char temp[100],temp2[100];

  //sprintf(temp," ");
  strcpy(temp," ");
  if(value2 > -2)
    {
      if(stepStep)
	sprintf(temp,">%d,%d ",nloop,timeFromStart);
      else
	sprintf(temp," %d,%d ",nloop,timeFromStart);
    }  
  strcat(temp,p);
  

  if(value1)
    {
      sprintf(temp2," %s",value1);
      strcat(temp,temp2);
    }
  
  if(value2 > -1)
    {
      sprintf(temp2," %d",value2);
      strcat(temp,temp2);
    }

  if(error == 1)
    {
      temp[0]='*';
      error = 0;
    }

  for(i=logSize;i>0;i--)strcpy(logBuffer[i],logBuffer[i-1]);
  strcpy(logBuffer[0],temp);

  wclear(slog);
  for(i=0;i<logSize;i++)
    {
      wmove(slog,i,0);
      wprintw(slog,"%s",logBuffer[i]);
    } 
  wrefresh(slog);
}

//====================================
void showConfig()
//====================================
{
  wmove(com,2,0);
  wprintw(com,"---Scenario Steps---");               wmove(com,3,0);
  wprintw(com," Analog Pins    = %d",scenAnalog);    wmove(com,4,0);
  wprintw(com," Digital Pins   = %d",scenDigital);   wmove(com,5,0);
  wprintw(com," Interrupts     = %d",scenInterrupt); wmove(com,7,0);
  wprintw(com,"---Configuration---");                wmove(com,8,0);
  wprintw(com," Delay    = %3d",confDelay);           wmove(com,9,0);
  wprintw(com," LogLevel = %3d",confLogLev);          wmove(com,10,0);
  wprintw(com," LogFile  = %3d",confLogFile);
  show(com);
}


//====================================
void showSerial(const char *m, int newLine)
//====================================
{
  int i;

  if(serialMode == ON)
    {
      if(rememberNewLine == 1)
	{
	  for(i=1;i<serialSize;i++)strcpy(serialBuffer[i],serialBuffer[i+1]);
	  strcpy(serialBuffer[serialSize],m);
	}
      else
	strcat(serialBuffer[serialSize],m);
      
      wclear(ser);
      for(i=1;i<=serialSize;i++)
	{
	  wmove(ser,i-1,0);
	  wprintw(ser,"%s",serialBuffer[i]);
	} 
      wrefresh(ser);
      rememberNewLine = newLine;
    }
  else
    {
      showError("Serial output without Serial.begin",timeFromStart);
/*       wmove(ser,1,0); */
/*       wprintw(ser,"Try to write to serial output without Serial.begin"); */
/*       wrefresh(ser); */
    }
}

//====================================
void getAppName(char *app)
//====================================
{
  FILE *in;
  char row[80],junk[20],*p;

  strcpy(app,"unknown");
  in = fopen("sketch/sketch.pde","r");
  if(in == NULL)
    {
      printf("Unable to open sketch\n");
      exit(0);
    }
  else
    {
      while (fgets(row,80,in)!=NULL)
	{
	  if(p=strstr(row,"simuino:"))
	    {
	      p=p+9;
	      sscanf(p,"%s",app);
	    }
	}
    }
  fclose(in);  
}

//====================================
int wCustomLog(char *in, char *out)
//====================================
{
  char *q,*p;
  int pin;

  p = strstr(in,":");
  p++;
  sscanf(p,"%d",&pin);
  p = strstr(p,"\"");
  p++;
  q = strstr(p,"\"");
  strcpy(q,"\0");
  strcpy(out,p);

  return(pin);
}

//====================================
void readSketchInfo()
//====================================
{
  FILE *in;
  char row[80],res[40],*p,value[5];
  int pin;

  in = fopen("sketch/sketch.pde","r");
  if(in == NULL)
    {
      showError("Unable to open sketch",-1);
    }
  else
    {
      while (fgets(row,80,in)!=NULL)
	{
	  if(p=strstr(row,"PINMODE_IN:"))
	    {
	      pin = wCustomLog(p,res);
	      strcpy(textPinModeIn[pin],res);
	    }
	  if(p=strstr(row,"PINMODE_OUT:"))
	    {
	      pin = wCustomLog(p,res);
	      strcpy(textPinModeOut[pin],res);
	    }
	  if(p=strstr(row,"DIGITALWRITE_LOW:"))
	    {
	      pin = wCustomLog(p,res);
	      strcpy(textDigitalWriteLow[pin],res);
	    }
	  if(p=strstr(row,"DIGITALWRITE_HIGH:"))
	    {
	      pin = wCustomLog(p,res);
	      strcpy(textDigitalWriteHigh[pin],res);
	    }
	  if(p=strstr(row,"ANALOGREAD:"))
	    {
	      pin = wCustomLog(p,res);
	      strcpy(textAnalogRead[pin],res);
	    }
	  if(p=strstr(row,"DIGITALREAD:"))
	    {
	      pin = wCustomLog(p,res);
	      strcpy(textDigitalRead[pin],res);
	    }
	  if(p=strstr(row,"ANALOGWRITE:"))
	    {
	      pin = wCustomLog(p,res);
	      strcpy(textAnalogWrite[pin],res);
	    }
	}
    }
  fclose(in);  
}

//====================================
void boardInit()
//====================================
{
  int i,j;

  nloop = 0;
  for(i=0;i<14;i++)
    {
      digitalMode[i] = FREE;
      strcpy(textPinModeIn[i],"void");
      strcpy(textPinModeOut[i],"void");

      strcpy(textDigitalWriteLow[i],"void");
      strcpy(textDigitalWriteHigh[i],"void");

      strcpy(textAnalogWrite[i],"void");
      strcpy(textAnalogRead[i],"void");

      strcpy(textDigitalRead[i],"void");
    }
  for(i=0;i<14;i++)
    {
      for(j=0;j<LOOP_MAX;j++)
	{
          analogPin[j][i]   = 0;
          digitalPin[j][i]  = 0;
	}
    }
}

//====================================
void unimplemented(const char *f)
//====================================
{
  char temp[200];
  sprintf(temp,"unimplemented: %s\n",f);
  wLog(temp,-1,-1);
}

//====================================
void readExt()
//====================================
{
  FILE *in;
  char row[120],*p,scenType[200], junk[20];
  int x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,x10,x11,x12,x13,temp;
  int i,state=0;

  in = fopen("scenario/scenario.txt","r");
  if(in == NULL)
    {
      showError("Unable to open scenario",-1);
    }
  else
    {
      while (fgets(row,80,in)!=NULL)
	{
	  if(row[0] == '#')
	    {	
	      if(p=strstr(row,"start_interrupts"))  state = 1;
	      if(p=strstr(row,"start_digital_pins"))state = 2;
	      if(p=strstr(row,"start_analog_pins")) state = 3;
	      if(p=strstr(row,"end_interrupts"))  state = 0;
	      if(p=strstr(row,"end_digital_pins"))state = 0;
	      if(p=strstr(row,"end_analog_pins")) state = 0;
	    }

	  if(row[0] != '#')
	    {	      
	      if(state==1)//Interrupts
		{
		  sscanf(row,"%d%d%d",&temp,&x0,&x1);
		  if(temp<STEP_MAX)
		    {
		      scenInterrupt++;
		      for(i=temp;i<STEP_MAX;i++)
			{
			  interrupt[i][0] = x0;
			  interrupt[i][1] = x1;
			}
		    }
		  else
		    showError("Interrupt scenario to long",temp);
		}
	      if(state==2) // Digital Pins
		{
		  sscanf(row,"%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d",&temp,&x0,&x1,&x2,&x3,&x4,&x5,&x6,&x7,&x8,&x9,&x9,&x10,&x11,&x12,&x13);
		  if(temp<STEP_MAX)
		    {
		      scenDigital++;
		      for(i=temp;i<STEP_MAX;i++)
			{
			  digitalPin[i][0]= x0;
			  digitalPin[i][1]= x1;
			  digitalPin[i][2]= x2;
			  digitalPin[i][3]= x3;
			  digitalPin[i][4]= x4;
			  digitalPin[i][5]= x5;
			  digitalPin[i][6]= x6;
			  digitalPin[i][7]= x7;
			  digitalPin[i][8]= x8;
			  digitalPin[i][9]= x9;
			  digitalPin[i][10]= x10;
			  digitalPin[i][11]= x11;
			  digitalPin[i][12]= x12;
			  digitalPin[i][13]= x13;
			}
		    }
		  else
		    showError("digital scenario to long",-1);
		}
	      if(state==3) // Analog Pins
		{
		  sscanf(row,"%d%d%d%d%d%d%d",&temp,&x0,&x1,&x2,&x3,&x4,&x5);
		  if(temp<STEP_MAX)
		    {
		      scenAnalog++;
		      for(i=temp;i<STEP_MAX;i++)
			{
			  analogPin[i][0]= x0;
			  analogPin[i][1]= x1;
			  analogPin[i][2]= x2;
			  analogPin[i][3]= x3;
			  analogPin[i][4]= x4;
			  analogPin[i][5]= x5;
			}
		    }
		  else
		    showError("analogPin scenario to long",-1);
		}	      
	    }
	}
    }
  fclose(in);
  return;
}

//====================================
void readConfig()
//====================================
{
  FILE *in;
  char row[80],*p,temp[40];
  int x;

  in = fopen("config.txt","r");
  if(in == NULL)
    {
      showError("Unable to open config.txt",-1);
    }
  else
    {
      while (fgets(row,80,in)!=NULL)
	{
	  if(row[0] != '#')
	    {
	      if(p=strstr(row,"LOG_LEVEL"))
		{
		  sscanf(p,"%s%d",temp,&confLogLev);
		}
	      if(p=strstr(row,"DELAY"))
		{
		  sscanf(p,"%s%d",temp,&confDelay);
		}
	      if(p=strstr(row,"LOG_FILE"))
		{
		  sscanf(p,"%s%d",temp,&confLogFile);
		}
	    }
	 
	}
    }
  fclose(in);
}
//====================================
// End of file
//====================================
