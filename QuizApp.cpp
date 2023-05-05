#include "QuizApp.h"
#include "Arduino.h"
QuizApp::QuizApp() { 
  m_lastQuestion = 0;
  m_questionType = QuestionType::regularQuestion;
  m_p_lcd = nullptr;
  m_p_keypad = nullptr;
  m_buzzerPin = 0;
  m_questionCount = 0;
  memset(m_userInput,'\0', 17);
}

void QuizApp::setLCDPins(uint8_t rs, uint8_t enable, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7) {
  m_p_lcd = new LiquidCrystal(rs, enable, d4, d5, d6, d7);
}

void QuizApp::setKeypadPins(uint8_t row1, uint8_t row2, uint8_t row3, uint8_t row4, uint8_t col1, uint8_t col2, uint8_t col3, uint8_t col4) {
  static char hexaKeys[4][4] = {   // Map the buttons to an array for the Keymap instance
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
  };
  uint8_t static colPins[4] = {row4, row3, row2, row1}; // Pins used for the rows of the keypad
  uint8_t static rowPins[4] = {col4, col3, col2, col1}; // Pins used for the columns of the keypad
  m_p_keypad = new Keypad(makeKeymap(hexaKeys), rowPins, colPins, 4, 4);
}

void QuizApp::setBuzzerPin(uint8_t buzzerPin) {
  m_buzzerPin = buzzerPin;
  pinMode(m_buzzerPin, OUTPUT);
  digitalWrite(m_buzzerPin, LOW);  
}

void QuizApp::showStartingScreen() {
  m_p_lcd->clear();
  m_p_lcd->setCursor(0, 0);
  m_p_lcd->print("Press any key");
  m_p_lcd->setCursor(0, 1);
  m_p_lcd->print("to start");
}

void QuizApp::generateUserInput() {
  uint8_t charCounter = 0;
  do{ //The first char has already been submitted
    m_p_lcd->clear(); // Clears the LCD screen
    m_p_lcd->setCursor(0, 1);
    switch(m_questionType){
      case QuestionType::regularQuestion:      
        m_p_lcd->print("[B]ack [C]onfirm"); //This needs to be printed every time
        if(isNumeric(m_userInput[charCounter])) { //Everything is okay and working as intended
          if(charCounter >= 16)  //Maximum has been reached
            m_userInput[16] = '\0';
          else
            charCounter += 1;
        }
        else{ //A Letter has been submitted
          switch(m_userInput[charCounter]){
            case 'B':   //B stands for "Back" as in "remove last letter"
              m_userInput[charCounter] = '\0';
              if(charCounter > 0){  //I can remove the last input
                charCounter -= 1;
                m_userInput[charCounter] = '\0';
              }
            break;
            case 'C':  //C stands for "Confirm"
              m_userInput[charCounter] = '\0';
              if(charCounter > 0){  //I can confirm the answer                
                return;
              }
            break;

            default:
              m_userInput[charCounter] = '\0';  //The inputed character can be ignored
            break;
          }
        }
      break;
      case QuestionType::multipleChoiceQuestion:
        if(isLetter(m_userInput[charCounter])) { //Everything is okay and working as intended
          m_p_lcd->print("Again to confirm"); //This needs to be printed every time a character is pressed
          if(charCounter > 0)  //A character has already been submitted prior to this check

            if(m_userInput[0] == m_userInput[1]){  //User pressed the same button twice
              m_userInput[1] = '\0';  //m_userInput is now only a single character
              return;
            }
            else{ //User changed his previous answer
              m_userInput[0] = m_userInput[1];  //Overwrite first answer with second
              m_userInput[1] = '\0';
            }

          else{ //User Inputs a character for the first time
            charCounter = 1;
          }
        }
        else{ //User pressed a key thats not a letter
          charCounter = 0;
          m_userInput[0] = '\0';  //Reset everything againg
          m_userInput[1] = '\0';
        }
      break;
    }
    m_p_lcd->setCursor(0, 0);
    m_p_lcd->print(m_userInput);
    do{
      m_userInput[charCounter] = waitForUserInput(255);
    }while(m_userInput[charCounter] == 0);
    //Serial.println("end");
    //Serial.println(m_userInput);
    delay(50); //waiting a moment
  }while(true);

}

void QuizApp::initialiseFile(){
  while(!SD.begin(10)){
    //Serial.println("Error reading from SD");
    delay(5000);
  }
  while(!SD.exists("question.csv")){
    //Serial.println("Error finding csv file");
    delay(5000);
  }
  //Serial.println("SD initialised");
  countQuestions();
}

void QuizApp::countQuestions(){
  m_questionFile = SD.open("question.csv");
  if(!m_questionFile)  //Horrible failure
    exit(0);
  
  m_questionCount = 0;
  while(m_questionFile.available()){
    if(m_questionFile.read() == '\n') //Linefeed tells me that I reached the end of a question
      m_questionCount += 1;
  }

  m_questionFile.close();
}

bool QuizApp::displayQuestion(){
  memset(m_userInput,'\0', 17); //I delete the last input of the user
  char* questionParameters = readQuestionFromFile();
  while(questionParameters[0] != ';'){ //Cut off the id
    questionParameters += 1;
  }  
  if(!strncmp(";RQ;", questionParameters, 4)){
    m_questionType = QuestionType::regularQuestion;
    questionParameters += 4;
  }
  else if(!strncmp(";MCQ;", questionParameters, 5)){
    m_questionType = QuestionType::multipleChoiceQuestion;
    questionParameters += 5;
  }
  char* questionText = questionParameters;
  while(questionParameters[0] != ';'){  //Find the end of the question text
    questionParameters += 1;
  }
  questionParameters[0] = '\0';
  questionParameters += 1;

  char tmpText[17]; //Stores the text which will be displayed on the lcd 
  uint8_t textLength = strlen(questionText);

  uint8_t lcdLayer = 0; //On which layer will the text be displayed
  uint8_t charsDisplayed = 0;
  m_p_lcd->clear(); // Clears the LCD screen 
  while(true){
      for(uint8_t currentIndex = 0; currentIndex < textLength; currentIndex += charsDisplayed){
      m_p_lcd->setCursor(0, lcdLayer);  //Cursor is always on the left side
      charsDisplayed = detectLineBreak(&questionText[currentIndex], 16);
      memset(tmpText,'\0', 17);   //Empty the array
      strncpy(tmpText, &questionText[currentIndex], charsDisplayed);  //Copy everything that I want to display
      //Serial.println(tmpText);
      m_p_lcd->print(tmpText);
      if(lcdLayer == 1 || currentIndex + charsDisplayed >= textLength){  //The second if clause makes sure that I even print out the last line on the lcd
        lcdLayer = 0;
        m_userInput[0] = waitForUserInput(4); // 4 seconds delay 
        if(m_userInput[0])
          goto endLoop;
        m_p_lcd->clear(); // Clears the LCD screen 
      }
      else{
        lcdLayer = 1;
      }
    }
  }
  endLoop:
  generateUserInput();
  if(strncmp(m_userInput, questionParameters, strlen(questionParameters)))
    return false;
  return true;
}

char* QuizApp::readQuestionFromFile(){
  static char question[100];  //This is where I am going to store the question
  memset(question,'\0', 100); //Clearing the array
  uint8_t questionID = millis() % m_questionCount + 1;

  while(questionID == m_lastQuestion) //To make sure you dont receive the same question back to back
    questionID = millis() % m_questionCount + 1;
  m_lastQuestion = questionID;

  m_questionFile = SD.open("question.csv");
  if(!m_questionFile)  //Horrible failure
    exit(0);

  uint8_t countedQuestions = 1; //I start at the first question and count until I reach the desired question
  while(questionID > countedQuestions){
    if(m_questionFile.read() == '\n') //Linefeed tells me that I reached the end of a question
      countedQuestions += 1;
  }
  char currentIndex = 0;
  while(true){    //Copy each character belonging to the question
    char nextCharacter = m_questionFile.read();
    if(nextCharacter == '\r') //'\r' shows me that I reached the end
      break;
    question[currentIndex] = nextCharacter;
    currentIndex += 1;
  }
  m_questionFile.close();
  return question;
}

char QuizApp::waitForUserInput(uint8_t seconds){
  unsigned long endTime = millis() + (unsigned long)seconds * 1000;
  while(millis() < endTime){
    delay(1);
    char button = m_p_keypad->getKey();
    if (button) {
      //Serial.print(button);
      tone(m_buzzerPin, NOTE_B4, 125);  
      delay(125);
      noTone(m_buzzerPin);
      return button;
    }
  }
  return 0;
}

void QuizApp::announceRightAnswer(){
  m_p_lcd->clear();
  m_p_lcd->setCursor(0, 0);
  m_p_lcd->print(m_userInput);
  m_p_lcd->setCursor(0, 1);
  m_p_lcd->print("Correct answer!");
  playHappyJingle();
}

void QuizApp::announceWrongAnswer(){
  m_p_lcd->clear();
  m_p_lcd->setCursor(0, 0);
  m_p_lcd->print(m_userInput);
  m_p_lcd->setCursor(0, 1);
  m_p_lcd->print("Wrong answer!");
  playSadJingle();
}

void QuizApp::playHappyJingle(){
  tone(m_buzzerPin, NOTE_B5, 125);  
  delay(125);
  noTone(m_buzzerPin);
}

void QuizApp::playSadJingle(){
  tone(m_buzzerPin, NOTE_C4, 375);  
  delay(375);
  noTone(m_buzzerPin);
}

uint8_t QuizApp::detectLineBreak(char* p_text, uint8_t desiredLength){
  if(p_text[desiredLength] == ' ' || p_text[desiredLength] == '\0')    //The current line doenst need to be trimmed
    return desiredLength;
  uint8_t actualLength;
  for(actualLength = desiredLength - 1; actualLength > 0; actualLength--){
    if(p_text[actualLength] == ' ' || p_text[actualLength] == '\0'){  //Detecting the end of a word
      return actualLength + 1;  //I need to increment by one, because im not dealing with the index anymore but instead with the actual count of chars
    }
  }  
  return desiredLength; //If there is no possibility for a regular linebreak, I simply force one
}
