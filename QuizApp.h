#ifndef QuizApp_h
#define QuizApp_h
#include <SD.h>
#include <SPI.h>
#include <LiquidCrystal.h> // includes the LiquidCrystal Library 
#include <Arduino.h>
#include <Keypad.h>

enum class QuestionType : uint8_t {regularQuestion, multipleChoiceQuestion};

// Defining frequency of each music note
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880
#define NOTE_B5 988

class QuizApp{
  private:
    Keypad* m_p_keypad;
    LiquidCrystal* m_p_lcd; // Creates an LCD object. Parameters: (rs, enable, d4, d5, d6, d7)
    File m_questionFile;
    QuestionType m_questionType;
    char m_userInput[17]; //16 Slots for the answer and 1 extra slot to store the current key (Like C: Confirm or B: Back)
    uint8_t m_lastQuestion; //To prevent the same question be displayed back to back
    uint8_t m_buzzerPin; //So the QuizApp knows which pin it needs to control to play sounds
  public:
    QuizApp(uint8_t rs, uint8_t enable, uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7);
    void setBuzzerPin(uint8_t buzzerPin);
    void initialiseFile();
    void showStartingScreen();
    inline void beginLcd(uint8_t width, uint8_t height) { m_p_lcd->begin(width, height); }
    bool displayQuestion(); //True: correct answer
    void generateUserInput(); //Will generate the input into the m_userInput array
    char* readQuestionFromFile();
    char waitForUserInput(uint8_t seconds); //Returns the inputed key of the user
    void announceRightAnswer();
    void announceWrongAnswer();
    void playHappyJingle();
    void playSadJingle();
    static uint8_t detectLineBreak(char* p_text, uint8_t desiredLength);
    static inline bool isNumeric(char character) { return(character >= '0' && character <= '9'); }
    static inline bool isLetter(char character) { return(character >= 'A' && character <= 'z'); }    
    //inline LiquidCrystal& getLcd() { return *m_p_lcd; }
};
#endif
