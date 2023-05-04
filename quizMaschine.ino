#include <LiquidCrystal.h> // includes the LiquidCrystal Library 
#include "QuizApp.h"

QuizApp quizHandler(A4, A5, A0, A1, A2, A3);

void setup() { 
 quizHandler.beginLcd(16,2); // Initializes the interface to the LCD screen, and specifies the dimensions (width and height) of the display } 
 //Serial.begin(9600);
 //while(!Serial)
  //delay(1);
 quizHandler.setBuzzerPin(1);
 quizHandler.showStartingScreen();
 quizHandler.initialiseFile();
}

void loop() {
  while(quizHandler.waitForUserInput(10) == 0)  //After 10 seconds, the screen will be refreshed
    quizHandler.showStartingScreen();

  if(quizHandler.displayQuestion())
    quizHandler.announceRightAnswer();
  else
    quizHandler.announceWrongAnswer();
}