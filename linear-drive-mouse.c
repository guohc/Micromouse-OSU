//compile with: gcc linear-drive-mouse.c -I/usr/local/include -L/usr/local/lib -lwiringPi -lm -o linear-drive-mouse
//start with: sudo ./linear-drive-mouse
//Circuit:
//Raspberry P1-1 (3.3V) to VCC on BTS7960 H bridge
//Raspberry P1-6 (GND) to GND on BTS7960 H bridge
//Raspberry P1-8 to PWM_L on BTS7960 H bridge
//Raspberry P1-10 to PWM_R on BTS7960 H bridge
//Raspberry P1-12 to R_EN and L_EN on BTS7960 H bridge

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/input.h>
#include <wiringPi.h>
#include <math.h>
#include <termios.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MOUSEFILE "/dev/input/mouse0"

#define DIRECTION_PIN  16
#define PWM_PIN        15
#define ENABLE_PIN      1

//4.5311
#define PROP_FACTOR  4.5311
#define MAX_PWM         100
#define MIN_PWM          50
#define STEP_WINDOW       1



int  MaxRows = 24;
int  MaxCols = 80;
int  MessageX = 1;
int  MessageY = 24;

int fd;
struct input_event ie;
unsigned char button,bLeft,bMiddle,bRight;
signed char x = 0,y = 0;
int absolute_x = 0, absolute_y = 0;
long setPointY = 0;
long moveLength = 1;
int printCount=0;
int lastY[10000];
int countY = 0;

int testMode = 0;
//+++++++++++++++++++++++ Start gotoxy ++++++++++++++++++++++++++
//Thanks to 'Stack Overflow', found on http://www.daniweb.com/software-development/c/code/216326
int gotoxy(int x, int y) {
  char essq[100]; // String variable to hold the escape sequence
  char xstr[100]; // Strings to hold the x and y coordinates
  char ystr[100]; // Escape sequences must be built with characters
   
  //Convert the screen coordinates to strings.
  sprintf(xstr, "%d", x);
  sprintf(ystr, "%d", y);
   
  //Build the escape sequence (vertical move).
  essq[0] = '\0';
  strcat(essq, "\033[");
  strcat(essq, ystr);
   
  //Described in man terminfo as vpa=\E[%p1%dd. Vertical position absolute.
  strcat(essq, "d");
   
  //Horizontal move. Horizontal position absolute
  strcat(essq, "\033[");
  strcat(essq, xstr);
  // Described in man terminfo as hpa=\E[%p1%dG
  strcat(essq, "G");
   
  //Execute the escape sequence. This will move the cursor to x, y
  printf("%s", essq);
  return 0;
}
//------------------------ End gotoxy ----------------------------------

//+++++++++++++++++++++++ Start kbhit ++++++++++++++++++++++++++++++++++
//Thanks to Undertech Blog, http://www.undertec.de/blog/2009/05/kbhit_und_getch_fur_linux.html
int kbhit(void) {

   struct termios term, oterm;
   int fd = 0;
   int c = 0;
   
   tcgetattr(fd, &oterm);
   memcpy(&term, &oterm, sizeof(term));
   term.c_lflag = term.c_lflag & (!ICANON);
   term.c_cc[VMIN] = 0;
   term.c_cc[VTIME] = 0;
   tcsetattr(fd, TCSANOW, &term);
   c = getchar();
   tcsetattr(fd, TCSANOW, &oterm);
   if (c != -1)
   ungetc(c, stdin);

   return ((c != -1) ? 1 : 0);

}
//------------------------ End kbhit -----------------------------------

//+++++++++++++++++++++++ Start getch ++++++++++++++++++++++++++++++++++
//Thanks to Undertech Blog, http://www.undertec.de/blog/2009/05/kbhit_und_getch_fur_linux.html
int getch(){
   static int ch = -1, fd = 0;
   struct termios new, old;

   fd = fileno(stdin);
   tcgetattr(fd, &old);
   new = old;
   new.c_lflag &= ~(ICANON|ECHO);
   tcsetattr(fd, TCSANOW, &new);
   ch = getchar();
   tcsetattr(fd, TCSANOW, &old);

//   printf("ch=%d ", ch);

   return ch;
}
//------------------------ End getch -----------------------------------


//+++++++++++++++++++++++++ PrintMenue_01 ++++++++++++++++++++++++++++++
void PrintMenue_01(){  
   printf("\e[?25l"); //Cursor OFF
   gotoxy(1,1);
   printf("Set         %+5ld \n", setPointY);
   printf("Abs. %+5ld, %+5ld \n", absolute_x, absolute_y);
   printf("Rel. %+5ld, %+5ld \n", x, y);
   printf("m           %+5ld \n", moveLength);
   if (testMode!=0){
     printf("%+4d             ", testMode);
   }
   else{
     printf("                 ");
   }
   
   printf("\e[?25h"); //Cursor ON
}
//------------------------- PrintMenue_01 ------------------------------

//+++++++++++++++++++++++++ getMouse +++++++++++++++++++++++++++++++++++
void getMouse(){
  unsigned char *ptr = (unsigned char*)&ie;
  int maxY = 0;
  int i;

  if(read(fd, &ie, sizeof(struct input_event))!=-1){
    button=ptr[0];
    bLeft = button & 0x1;
    bMiddle = ( button & 0x4 ) > 0;
    bRight = ( button & 0x2 ) > 0;
    x=(signed char) ptr[1];y=(signed char) ptr[2];
    // computes absolute x,y coordinates
    absolute_x += x;
    absolute_y += y;
    
    countY++;
    if(countY = 10000){
      countY = 0;
    }
    lastY[countY] = y;
    
    for(i=0; i<10000;i++){
      if(abs(maxY) < abs(lastY[i])){
        maxY = y;
      }
    }
    if(testMode != 0){
      y = maxY;
    }


    if (bRight==1){
      absolute_x = 0;
      absolute_y = 0;
      printf("Absolute x,y coords origin recorded\n");
    }
    fflush(stdout);
    if(y!=0){
      PrintMenue_01();
    }
    printCount=0;
  }  
  else{
    if(testMode!=0){
      y=maxY;
    }
    
    if(x!=0 || y!=0){
      printCount++;
      if(printCount > 10000){
        printCount=0;
        x=0;
        y=0;
        PrintMenue_01();
      }
    }
  }
  
}
//--------------------- getMouse ---------------------------------------

//######################################################################
//######################################################################
//######################################################################
int main(){
  int stepWindow = STEP_WINDOW;
  double propFactor = PROP_FACTOR;
  int maxPWM = MAX_PWM;
  int minPWM = MIN_PWM;
  int testModeStep = 2000;
  int testModeStepCount = 0;
  int setOK = 0;

  struct winsize terminal;

  int KeyHit = 0;
  int KeyCode[5];
  int SingleKey=0;
  int pwm = 5;
  int i;

  if (wiringPiSetup () == -1){
    printf("Could not run wiringPiSetup!");
    exit(1);
  }

  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, 0);
  
  pinMode(DIRECTION_PIN, OUTPUT);
  digitalWrite(DIRECTION_PIN, 0);

  softPwmCreate(PWM_PIN, 0, 100);
  softPwmWrite(PWM_PIN, 0);


  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal)<0){
    printf("Can't get size of terminal window");
  }
  else{
    MaxRows = terminal.ws_row;
    MaxCols = terminal.ws_col;
    MessageY = MaxRows-3;
  }

    if((fd = open(MOUSEFILE, O_RDONLY | O_NONBLOCK )) == -1){
      printf("NonBlocking %s open ERROR\n",MOUSEFILE);
      exit(EXIT_FAILURE);
    }
/*
    //Set DPI (not working...)
    char dpiByte = 0x03;//0x00(1 count/mm), 0x01(2 count/mm), 0x02(4 count/mm), 0x03(8 count/mm), 
    char commandByte = 0xE8;
    char receiveByte = 0x00;
    
    write(fd, &commandByte, 1);
    read(fd, &receiveByte, 1);
    printf("Received from mouse %d\n", receiveByte);
    write(fd, &dpiByte, 1);
    read(fd, &receiveByte, 1);
    printf("Received from mouse %d\n", receiveByte);
    
    //Set scaling
    commandByte = 0xE7;//0xE6(Set Scaling 1:1), 0xE7 (Set Scaling 2:1)
    write(fd, &commandByte, 1);
    read(fd, &receiveByte, 1);
    printf("Received from mouse %d\n", receiveByte);
*/
  
  
  //printf("right-click to set absolute x,y coordinates origin (0,0)\n");
  
  gotoxy (1, 1);
  printf("                                                           ");
  printf("                                                           ");
  printf("                                                           ");
  printf("                                                           ");
  printf("                                                           ");
  printf("                                                           ");
  printf("                                                           ");
  printf("                                                           ");
  printf("                                                           ");
  printf("                                                           ");
  printf("                                                           ");
  
  PrintMenue_01();
  
  for(i=0; i<10000; i++){
    lastY[i] = 0;
  }
  while(1){
    

    getMouse();
    
    i = 0;
    SingleKey = 1;
    KeyCode[0] = 0;
    KeyCode[1] = 0;
    KeyCode[2] = 0;
    KeyCode[3] = 0;
    KeyCode[4] = 0;
    KeyHit = 0;
    while (kbhit()){
      KeyHit = getch();
      KeyCode[i] = KeyHit;
      i++;
      if(i == 5){
        i = 0;
      }
      if(i > 1){
        SingleKey = 0;
      }
    }
    if(SingleKey == 0){
      KeyHit = 0;
    }

    //Move drive
    if(KeyCode[0] == 27 && KeyCode[1] == 91 && KeyCode[2] == 68 && KeyCode[3] == 0 && KeyCode[4] == 0){
      setPointY += moveLength;
      PrintMenue_01();
      gotoxy (1, 5);
    }

    if(KeyCode[0] == 27 && KeyCode[1] == 91 && KeyCode[2] == 67 && KeyCode[3] == 0 && KeyCode[4] == 0){
      setPointY -= moveLength;
      PrintMenue_01();
      gotoxy (1, 5);
    }

    if(KeyHit == 'm'){
      moveLength *= 2;
      if(moveLength >= 10000){
        moveLength = 1;
      }
      PrintMenue_01();
      gotoxy (1, 5);
    }

    if(KeyHit == '0'){
      absolute_x = 0;
      absolute_y = 0;
      setPointY = 0;
      testMode = 0;
      PrintMenue_01();
      gotoxy (1, 5);
    }

    if(KeyHit == 27){
      exit(0);
    }


   
    pwm = (int)((double)(abs(setPointY - absolute_y)) * propFactor);
    if(pwm < minPWM) pwm = minPWM;
    if(pwm > maxPWM) pwm = maxPWM;

    if(absolute_y > setPointY + stepWindow){
      digitalWrite(ENABLE_PIN, 1);
      digitalWrite(DIRECTION_PIN, 0);
      softPwmWrite(PWM_PIN, pwm);
      //printf("> \r");
    }
    if(absolute_y < setPointY - stepWindow){
      digitalWrite(ENABLE_PIN, 1);
      digitalWrite(DIRECTION_PIN, 1);
      softPwmWrite(PWM_PIN, 100 - pwm);
      //printf("< \r");
    }

    if(absolute_y >= setPointY - stepWindow && absolute_y <= setPointY + stepWindow){
      digitalWrite(ENABLE_PIN, 0);
      digitalWrite(DIRECTION_PIN, 0);
      softPwmWrite(PWM_PIN, 0);
      maxPWM = MAX_PWM;
      minPWM = MIN_PWM;
      setOK = 1;
      stepWindow = STEP_WINDOW;
      //printf("= \r");
    }
 
    if(testMode != 0){
      testModeStepCount++;
      if(testModeStepCount > testModeStep){
        testModeStepCount = 0;
        digitalWrite(ENABLE_PIN, 0);
        sleep(1);
        digitalWrite(ENABLE_PIN, 1);   
      }
    }

    if(KeyHit == 'T'){
      digitalWrite(ENABLE_PIN, 1);
      digitalWrite(DIRECTION_PIN, 0);
      softPwmWrite(PWM_PIN, 100);
      
      setPointY = -100;
      while(absolute_y > -100){
        getMouse();
      }      
      setPointY = 0;
      
      digitalWrite(ENABLE_PIN, 0);
      digitalWrite(DIRECTION_PIN, 0);
      softPwmWrite(PWM_PIN, 0);

      maxPWM = 10;
      minPWM = 10;
      stepWindow = 0;
    }

    if(KeyHit == 'R'){
      digitalWrite(ENABLE_PIN, 1);
      digitalWrite(DIRECTION_PIN, 1);
      softPwmWrite(PWM_PIN, 0);

      setPointY = 100;
      while(absolute_y < 100){
        getMouse();
      }      
      setPointY = 0;
      
      digitalWrite(ENABLE_PIN, 0);
      digitalWrite(DIRECTION_PIN, 0);
      softPwmWrite(PWM_PIN, 0);

      maxPWM = 10;
      minPWM = 10;
      stepWindow = 0;
    }

    if(KeyHit == 'A'){
      testMode = 100;
    }

    if(KeyHit == 'B'){
      testMode = -100;
    }

    if(testMode > 0 && setOK == 1){
      sleep(5);
      digitalWrite(ENABLE_PIN, 1);
      digitalWrite(DIRECTION_PIN, 0);
      softPwmWrite(PWM_PIN, 100);

      setPointY = -200;
      while(absolute_y > -200){
        getMouse();
      }
      setPointY = 0;
      
      digitalWrite(ENABLE_PIN, 0);
      digitalWrite(DIRECTION_PIN, 0);
      softPwmWrite(PWM_PIN, 0);
      testMode--;
      testModeStepCount = 0;
      setOK = 0;
    }

    if(testMode < 0 && setOK == 1){
      sleep(5);
      digitalWrite(ENABLE_PIN, 1);
      digitalWrite(DIRECTION_PIN, 1);
      softPwmWrite(PWM_PIN, 0);

      setPointY = 200;
      while(absolute_y < 200){
        getMouse();
      }
      setPointY = 0;
      
      digitalWrite(ENABLE_PIN, 0);
      digitalWrite(DIRECTION_PIN, 0);
      softPwmWrite(PWM_PIN, 0);
      testMode++;
      testModeStepCount = 0;
      setOK = 0;
    }

    
    
  }

return 0;
}

