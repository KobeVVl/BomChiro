#include <BombFunctions.h>
#include <BombDictionary.h>

#define ZETMOEILIJKHEID true

Preset previousPreset;

LiquidCrystal lcd(reset, enable, d4, d5, d6, d7);

int address = 0;

uint8_t amPresets;
uint8_t language = NL;
uint8_t CHECKMARK[8] = {0x0, 0x1, 0x3, 0x16, 0x1c, 0x8, 0x0};

enum ConfirmState
{
  Succes,
  Fail,
  Back
};

enum State
{
  Start,
  Settings,
  ChangeLanguage,
  CalibrateButtons,
  CalibrateWires,
  ZetUit,
  HergebruikInst,
  InstMode,
  InstTijd,
  InstLenCode,
  InstCode,
  InstDraad,
  InstBanana,
  InstAantalVragen,
  InstMoeilijkheidVragen,
  InstPlantcode,
  InstPlanttijd,
  CreatePreset,
  WachtOK,
  WachtStart,
  WachtPlantcode,
  WachtPlant,
  Main,
  BomOntmanteld,
  BomOntploft,
  Reset,
};

State currentState = Start;

void setup()
{
  lcd.begin(16, 2);
  prepare();
  lcd.clear();
  lcd.createChar(0, CHECKMARK);
  amPresets = EEPROM.read(address);
  address += sizeof(amPresets);
}

void setInstMode(uint16_t &mode);

void setAantalVragen(uint16_t mode, ConfirmState &s, uint8_t &amQuestions);

void setInstTijd(ConfirmState &s, unsigned int &time);

void setLenCode(ConfirmState &s, uint16_t mode, uint8_t &codeLength);

void setCode(ConfirmState &s, uint8_t codeLength, String &code);

void setDraad(ConfirmState &s, uint16_t mode, int &wire);

void loop()
{
  static Preset preset;
  static uint16_t mode;
  static unsigned int time = TIME;
  static uint8_t codeLength = 0;
  static String code = DEFAULT_CODE;
  static int wire = DEFAULT_WIRE;
  static int8_t bananaConf = DEFAULT_BANANA;
  static uint8_t amQuestions = DEFAULT_AMQUESTIONS;
  static Difficulty diff = EASY;
  static String plantCode = "";
  static unsigned int plantTime;

  ConfirmState s = Succes;
  switch (currentState)
  {
  case Start:
  {
    currentState = HergebruikInst;
    break;
  }
  case Settings:
  {
    setSettingMode();
    if (DEBUG)
    {
      Serial.println("Settings returned");
      Serial.print("Next state is: ");
      Serial.println(currentState);
    }
    break;
  }
  case ChangeLanguage:
  {
    changeLanguageGetPreset(&address);
    currentState = Settings;
    break;
  }
  case CalibrateButtons:
  {
    calibrateButtons();
    currentState = Settings;
    break;
  }
  case CalibrateWires:
  {
    calibrateWires();
    currentState = Settings;
    break;
  }
  case ZetUit:
  {
    if (DEBUG)
      Serial.println("Turning off?");
    s = waitForConfirm(languageDict[32][language]); // "Zet uit"
    if (s == Succes)
    {
      if (DEBUG)
        Serial.println("BOM UIT");
      digitalWrite(relayPin, HIGH);
    }
    else if (s == Back)
    {
    }
    else
    {
      if (DEBUG)
        Serial.println("Bom blijft aan");
    }
    currentState = Settings;
    break;
  }
  case HergebruikInst:
  {
    if (DEBUG)
      previousPreset.print();
    s = waitForConfirm(languageDict[0][language]); // "Hergebruik inst."
    if (s == Succes)
    {
      preset = previousPreset;
      currentState = WachtOK;
    }
    else if (s == Back)
      currentState = Settings;
    else
      currentState = InstMode;
    break;
  }
  case InstMode:
  {
    setInstMode(mode);
    break;
  }
  case InstTijd:
  {
    setInstTijd(s, time);
    break;
  }
  case InstLenCode:
  {
    setLenCode(s, mode, codeLength);
    break;
  }
  case InstCode:
  {
    setCode(s, codeLength, code);
    break;
  }
  case InstDraad:
  {
    setDraad(s, mode, wire);
    break;
  }
  case InstBanana:
  {
    if (mode == 0 || mode == 2)
    {
      lcd.clear();
      lcd.print(languageDict[10][language]); // "Zet ban cnf + OK"
      uint8_t nr = waitForNumber();
      while (nr != OK && nr != BACK)
      {
        nr = waitForNumber();
      }
      if (nr == OK)
      {
        bananaConf = getBanana();
        currentState = InstAantalVragen;
      }
      else
        currentState = InstDraad;
    }
    else
      currentState = InstAantalVragen;
    break;
  }
  case InstAantalVragen:
  {
    setAantalVragen(mode, s, amQuestions);
    break;
  }
  case InstMoeilijkheidVragen:
  {
    if (mode == 1 && ZETMOEILIJKHEID)
    {
      lcd.clear();
      lcd.print(languageDict[12][language]); // "Moeilijkheid vrg"
      String numberString;
      s = getNumberString(1, 0, 1, &numberString);
      if (s == Succes)
      {
        uint8_t diff_int = numberString.toInt();
        switch (diff_int)
        {
        case 3:
          diff = HARD;
          break;
        case 2:
          diff = MEDIUM;
          break;
        default:
          diff = EASY;
          break;
        }
        currentState = InstPlantcode;
      }
      else
        currentState = InstAantalVragen;
    }
    else
      currentState = InstPlantcode;
    break;
  }
  case InstPlantcode:
  {
    if (mode == 1)
    {
      lcd.clear();
      lcd.print(languageDict[13][language]); // "Stel plntcode in"
      s = getNumberString(16, 0, 1, &plantCode);
      if (s == Succes)
        currentState = InstPlanttijd;
      else
        currentState = InstMoeilijkheidVragen;
    }
    else
      currentState = InstPlanttijd;
    break;
  }
  case InstPlanttijd:
  {
    if (mode == 1)
    {
      lcd.clear();
      lcd.print(languageDict[14][language]); // "Stel plnttijd in"
      String numberString;
      s = getNumberString(2, 0, 1, &numberString);
      if (s == Succes)
      {
        plantTime = numberString.toInt();
        currentState = CreatePreset;
      }
      else
        currentState = InstPlantcode;
    }
    else
      currentState = CreatePreset;
    break;
  }
  case CreatePreset:
  {
    Preset pr(code, bananaConf, wire, amQuestions, diff, plantCode, plantTime, time);
    preset = pr;
    previousPreset = preset;
    writePresetToEEPROM(address, preset);
    if (DEBUG)
    {
      preset.print();
    }
    currentState = WachtOK;
    break;
  }
  case WachtOK:
  {
    lcd.clear();
    lcd.print(languageDict[15][language]); // "Druk op "
    lcd.write((byte)0);                    // "✓"
    lcd.setCursor(0, 1);
    lcd.print(languageDict[16][language]); // "om te starten"

    uint8_t d = getDigit();
    while (d != OK && d != BACK)
      d = getDigit();
    wait(d);
    currentState = d == OK ? WachtPlantcode : InstPlantcode;
    break;
  }
  case WachtStart:
    /* code */
    break;
  case WachtPlantcode:
  {
    if (waitForPlantCode(preset.plantCode) == Succes)
      currentState = WachtPlant;
    else
      currentState = WachtOK;
    break;
  }
  case WachtPlant:
  {
    if (plant(preset.plantTime) == Succes)
      currentState = Main;
    else
      currentState = WachtPlantcode;
    break;
  }
  case Main:
  {
    s = mainProgram(preset);
    if (DEBUG)
      Serial.println("Ran mainProgram");
    switch (s)
    {
    case Succes:
    {
      currentState = BomOntmanteld;
      break;
    }
    case Back:
    {
      currentState = Reset;
      break;
    }
    default:
    {
      currentState = BomOntmanteld;
      break;
    }
    }
    break;
  }
  case BomOntmanteld:
  {
    lcd.clear();
    lcd.print(languageDict[20][language]); // "Bom ontmanteld"
    controlRedLed(LOW);
    victory();
    currentState = Reset;
    break;
  }
  case BomOntploft:
  {
    lcd.clear();
    lcd.print(languageDict[21][language]); // "Bom ontploft"
    controlRedLed(LOW);
    defeat();
    currentState = Reset;
    break;
  }
  case Reset:
  {
    resetAll();
    currentState = HergebruikInst;
    break;
  }
  default:
    break;
  }
}

void setDraad(ConfirmState &s, uint16_t mode, int &wire)
{
  s = Succes;
  if (mode == 0)
  {
    lcd.clear();
    lcd.print(languageDict[8][language]); // "Stel draad in:"
    String numberString;
    s = getNumberString(1, 0, 1, &numberString);
    if (s == Back)
      currentState = InstTijd;
    else
    {
      wire = numberString.toInt();
      while (wire <= 0 || wire >= (sizeof(wireValues) / sizeof(wireValues[0])))
      {
        lcd.clear();
        lcd.print(languageDict[9][language]); // "NUMMER TE HOOG"
        delay(2500);
        lcd.clear();
        lcd.print(languageDict[8][language]); // "Stel draad in:"
        s = getNumberString(1, 0, 1, &numberString);
        if (s == Back)
        {
          currentState = InstTijd;
          break;
        }
        wire = numberString.toInt();
      }
      lcd.clear();
      lcd.print(languageDict[33][language] + wire); // "Draad: "
      lcd.setCursor(0, 1);
      lcd.print(languageDict[34][language]); // "ingesteld"
      delay(2000);
    }
  }
  if (s == Back)
    currentState = InstTijd;
  else
    currentState = InstBanana;
}

void setCode(ConfirmState &s, uint8_t codeLength, String &code)
{
  s = Succes;
  if (codeLength > 0)
  {
    lcd.clear();
    lcd.print(languageDict[6][language]); // "Stel code in:"
    s = getNumberString(codeLength, 0, 1, &code);
    if (s == Back)
      currentState = InstLenCode;
    else
    {
      while (code.length() != codeLength)
      {
        lcd.clear();
        lcd.print(languageDict[7][language]); // "GEEN JUISTE LEN."
        delay(2500);
        lcd.clear();
        lcd.print(languageDict[6][language]); // "Stel code in:"
        s = getNumberString(codeLength, 0, 1, &code);
        if (s == Back)
        {
          currentState = InstLenCode;
          break;
        }
      }
    }
  }
  if (s == Back)
    currentState = InstLenCode;
  else
    currentState = InstDraad;
}

void setLenCode(ConfirmState &s, uint16_t mode, uint8_t &codeLength)
{
  s = Succes;
  if (mode == 0)
  {
    lcd.clear();
    lcd.print(languageDict[5][language]); // "Stel len code in"
    String numberString;
    s = getNumberString(2, 0, 1, &numberString);
    if (s == Back)
      currentState = InstTijd;
    else
    {
      codeLength = numberString.toInt();
      while (codeLength > 16)
      {
        lcd.clear();
        lcd.print(languageDict[9][language]);
        delay(2500);
        lcd.clear();
        lcd.print(languageDict[5][language]); // "Stel len code in"
        s = getNumberString(2, 0, 1, &numberString);
        if (s == Back)
        {
          currentState = InstTijd;
          break;
        }
        codeLength = numberString.toInt();
      }
    }
  }
  if (s == Back)
    currentState = InstTijd;
  else
    currentState = InstCode;
}

void setInstTijd(ConfirmState &s, unsigned int &time)
{
  lcd.clear();
  lcd.print(languageDict[4][language]); // "Geef totale tijd"
  lcd.setCursor(5, 1);
  lcd.print("sec.");
  String numberString;
  s = getNumberString(4, 0, 1, &numberString);
  if (s == Succes)
  {
    time = numberString.toInt();
    currentState = InstLenCode;
  }
  else
  {
    currentState = InstMode;
  }
}

void setAantalVragen(uint16_t mode, ConfirmState &s, uint8_t &amQuestions)
{
  if (mode == 1)
  {
    lcd.clear();
    lcd.print(languageDict[11][language]); // "Stel #vragen in:"
    String numberString;
    s = getNumberString(2, 0, 1, &numberString);
    if (s == Succes)
    {
      amQuestions = numberString.toInt();

      currentState = InstMoeilijkheidVragen;
    }
    else
      currentState = InstBanana;
  }
  else
    currentState = InstMoeilijkheidVragen;
}

void setSettingMode()
{
  lcd.clear();
  lcd.print(languageDict[35][language]); // "Selecteer met "
  lcd.write((byte)0);                    // "✓"
  uint16_t selected_mode = 0;
  lcd.setCursor(0, 1);
  lcd.print(settings[selected_mode][language]);
  uint8_t buttonCall = getDigit();
  while (buttonCall != OK && buttonCall != BACK)
  {
    if (buttonCall == LEFT)
    {
      selected_mode = ((selected_mode + AMOUNT_SETTINGS) - 1) % AMOUNT_SETTINGS;
      lcd.setCursor(0, 1);
      lcd.print(settings[selected_mode][language]);
      wait(LEFT);
    }
    else if (buttonCall == RIGHT)
    {
      selected_mode = (selected_mode + 1) % AMOUNT_SETTINGS;
      lcd.setCursor(0, 1);
      lcd.print(settings[selected_mode][language]);
      wait(RIGHT);
    }
    buttonCall = getDigit();
  }
  wait(buttonCall);
  if (DEBUG)
  {
    Serial.print("Buttoncall: ");
    Serial.println(buttonCall);
    Serial.print("Selected mode: ");
    Serial.println(selected_mode);
  }
  if (buttonCall == BACK)
    currentState = HergebruikInst;
  else if (buttonCall == OK)
  {
    switch (selected_mode)
    {
    case 0:
    {
      if (DEBUG)
        Serial.println("Selected change language");
      currentState = ChangeLanguage;
    }
    case 1:
    {
      if (DEBUG)
        Serial.println("Selected calibrate buttons");
      currentState = CalibrateButtons;
    }
    case 2:
    {
      if (DEBUG)
        Serial.println("Selected calibrate wires");
      currentState = CalibrateWires;
    }
    case 3:
    {
      if (DEBUG)
        Serial.println("Selected turn off");
      currentState = ZetUit;
    }
    }
  }
  if (DEBUG)
    Serial.println(currentState);
}

void setInstMode(uint16_t &mode)
{
  lcd.clear();
  lcd.print(languageDict[35][language]); // "Selecteer met "
  lcd.write((byte)0);                    // "✓"
  uint16_t selected_mode = 0;
  lcd.setCursor(0, 1);
  lcd.print(modes[selected_mode][language]);
  uint8_t buttonCall = getDigit();
  while (buttonCall != OK && buttonCall != BACK)
  {
    if (buttonCall == LEFT)
    {
      selected_mode = ((selected_mode + AMOUNT_MODES) - 1) % AMOUNT_MODES;
      lcd.setCursor(0, 1);
      lcd.print(modes[selected_mode][language]);
      wait(LEFT);
    }
    else if (buttonCall == RIGHT)
    {
      selected_mode = (selected_mode + 1) % AMOUNT_MODES;
      lcd.setCursor(0, 1);
      lcd.print(modes[selected_mode][language]);
      wait(RIGHT);
    }
    buttonCall = getDigit();
  }
  wait(buttonCall);
  if (buttonCall == BACK)
    currentState = HergebruikInst;
  else if (buttonCall == OK)
  {
    mode = selected_mode;
    currentState = InstTijd;
  }
}

ConfirmState waitForConfirm(String text)
{
  lcd.clear();
  lcd.print(text); // Print text that is passed
  lcd.setCursor(0, 1);
  lcd.print(languageDict[1][language]); // "NEE(X) JA("
  lcd.write((byte)0);                   // "✓"
  lcd.write(')');                       // ")"
  uint8_t buttonCall = getDigit();
  while (buttonCall != OK && buttonCall != BAD && buttonCall != BACK)
  {
    buttonCall = getDigit();
  }
  wait(buttonCall);
  if (buttonCall == OK)
    return Succes;
  else if (buttonCall == BACK)
    return Back;
  return Fail;
}

ConfirmState waitForPlantCode(String code)
{
  bool plantedCodeDone = (code == "");
  String plantedCode = "";
  lcd.clear();
  lcd.print(languageDict[22][language]); // "Geef plntcode in"
  lcd.setCursor(0, 1);
  while (!plantedCodeDone)
  {
    int nr = getDigit();
    if (nr == BACK)
    {
      return Back;
    }
    else if (nr == BAD)
    {
      plantedCode = "";
      lcd.setCursor(0, 1);
      lcd.print("                ");
    }
    else if (nr != -1 && nr < 100)
    {
      plantedCode += String(nr);
      lcd.setCursor(0, 1);
      lcd.print(plantedCode);
      if (plantedCode.length() == code.length())
      {
        if (plantedCode == code)
        {
          plantedCodeDone = true;
        }
        else
        {
          tone(piezoPin, 500, 500);
          plantedCode = "";
          lcd.setCursor(0, 1);
          lcd.print("                ");
        }
      }
      wait(nr);
    }
  }
  return Succes;
}

ConfirmState plant(int pressPlant)
{
  bool planted = pressPlant <= 0;
  int buttonState = LOW;
  int prevState = LOW;
  unsigned long startTime;
  lcd.clear();
  lcd.print(languageDict[23][language]); // "Houd "
  lcd.write((byte)0);                    // "✓"
  char buffer[10];
  sprintf(buffer, " %d sec", pressPlant);
  lcd.write(buffer);
  lcd.setCursor(0, 1);
  lcd.print(languageDict[24][language]); // "om te planten"
  while (!planted)
  {
    int number = getDigit();
    if (number == BACK)
    {
      return Back;
    }
    else if (number == OK)
    {
      buttonState = HIGH;
    }
    else
    {
      buttonState = LOW;
    }
    if (buttonState == HIGH && prevState == LOW)
    { // Als de knop ingedrukt wordt (en dus nog niet ingedrukt was)
      lcd.clear();
      startTime = millis(); // Sla de tijd op wanneer de knop ingedrukt wordt
      prevState = HIGH;     // Onthoud dat de knop ingedrukt is
    }
    else if (buttonState == HIGH && prevState == HIGH)
    { // Als de knop nog steeds ingedrukt is
      unsigned long currentTime = millis();
      int count = (currentTime - startTime) / (pressPlant * 1000 / 32);
      if (count < 16)
      {
        lcd.setCursor(0, 0);
      }
      else
      {
        lcd.setCursor(0, 1);
      }
      repeatLcdPrint((char)255, count % 16 + 1);
      if (currentTime - startTime >= pressPlant * 1000)
      {
        planted = true;
      } // Als de knop voor `pressPlant` seconden is ingedrukt (verschil tussen tijdstip dat de knop werd ingedruk met tijdstip op dit moment) dan is programma klaar (done).
    }
    else if (buttonState == LOW && prevState == HIGH)
    { // Als knop losgelaten wordt (en dus ingedrukt was)
      prevState = LOW;
      lcd.clear();
      lcd.print(languageDict[23][language] + String(pressPlant) + " sec"); // "Houd "
      lcd.write((byte)0);                                                  // "✓"
      lcd.setCursor(0, 1);
      lcd.print(languageDict[24][language]); // "om te planten"
    }
  }
  wait(OK);
  return Succes;
}

ConfirmState mainProgram(Preset &preset)
{
  unsigned int totalTime = preset.time; // Total time to defuse

  bool questionsMode = preset.amQuestions > DEFAULT_AMQUESTIONS;

  bool codeDone = preset.code == DEFAULT_CODE;
  bool wireDone = preset.wire == DEFAULT_WIRE;
  bool bananaDone = preset.bananaConfig == DEFAULT_BANANA;
  bool questionsDone = !questionsMode;

  String trueCode = preset.code;
  uint8_t codeLength = trueCode.length();
  uint8_t trueWire = preset.wire;
  uint8_t trueBanana = preset.bananaConfig;

  uint8_t number;
  uint8_t selected = 0;
  bool pressed = false;
  String enteredCode = String("");
  uint8_t codeIndex = 0;

  String enteredAnswer = "";
  uint8_t answerIndex = 0;
  String question;

  uint8_t amountDone = 0;

  unsigned int showDoneTime = 2500; // SHOW MESSAGE "... done" for 2 seconds

  unsigned long codeDoneTime;
  bool printCodeDone = false;
  unsigned long wireDoneTime;
  bool printWireDone = false;
  unsigned long bananaDoneTime;
  bool printBananaDone = false;

  unsigned long startTime = millis();
  unsigned long turnOffTime = 0;
  unsigned long intervalTime = startTime;
  unsigned long prevIntervalTime = intervalTime;
  unsigned int interval = maxInterval;
  bool redLedOn = false;

  unsigned int timer = -1;
  serialDebugMain(trueCode, trueBanana, trueWire, totalTime, codeDone, wireDone, bananaDone, questionsDone);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(languageDict[27 + selected][language]);
  while ((!codeDone || !wireDone || !bananaDone || !questionsDone) && (millis() - startTime) <= totalTime * 1000L)
  {
    // ------------- WIRE PART ---------------
    if (!wireDone)
    {
      uint8_t wr = getWire();
      if (wr != -1 && wr != 255) // Als er een draad gevonden is
      {
        if (DEBUG)
          Serial.println(wr);
        if (wr == trueWire)
        {
          wireDone = true;
          wireDoneTime = millis();
          printWireDone = true;
        }
        else
        {
          totalTime = 0;
          wrongTune();
        }
      }
    }
    if (!codeDone || !bananaDone)
    {
      uint8_t nr = getDigit();
      if (nr != -1 && nr != 255) // Knop ingedrukt
      {
        if (!pressed) // Knop net ingedrukt
        {
          pressed = true;
          number = nr;
          if (nr == LEFT || nr == RIGHT)
          {
            selected = (selected == 0) ? 2 : 0;
            lcd.setCursor(0, 0);
            lcd.print(languageDict[27 + selected][language]);
          }
          else if (nr == OK)
          {
            // check selected part
            // ------------- CODE LOCK PART ---------------
            if (!codeDone && selected == 0)
            {
              // Compare entered code with rigth code
              if (enteredCode == trueCode)
              {
                codeDone = true;
                codeDoneTime = millis();
                printCodeDone = true;
              }
              else
              {
                codeIndex = 0;
                enteredCode = "";
                totalTime -= punishment(CODE_SEVERITY);
                wrongTune();
              }
            }
            // ------------- BANANA PART ---------------
            if (!bananaDone && selected == 2)
            {
              uint8_t bn = getBanana();
              Serial.print("Banana: ");
              Serial.println(bn);
              if (bn != -1 && bn != 255)
              {
                if (bn == trueBanana)
                {
                  bananaDone = true;
                  bananaDoneTime = millis();
                  printBananaDone = true;
                }
                else
                {
                  totalTime -= punishment(BANANA_SEVERITY);
                  wrongTune();
                }
              }
            }
          }
          else if (nr == BAD)
          {
            if (DEBUG)
            {
              Serial.println("BAD pressed");
            }
            codeIndex = 0;
            enteredCode = "";
          }
          else if (nr == BACK)
          {
            turnOffTime = millis();
            if (DEBUG)
            {
              Serial.println("BACK pressed");
            }
          }
          serialDebugPressed(number, pressed);
        }
        else if (turnOffTime > 0 && (millis() - turnOffTime) > 5000L)
        {
          // if (DEBUG)
          //   Serial.println("BOM UIT");
          // digitalWrite(relayPin, HIGH);
          return Back;
        }
      }
      else // Geen knop ingedrukt
      {
        if (pressed) // Knop net losgelaten
        {
          pressed = false;
          turnOffTime = 0;
          if (codeIndex == codeLength)
          {
            codeIndex = 0;
            enteredCode = "";
          }

          if (number < 10)
          {
            char charac = number + 48;
            enteredCode += charac; // Int word geconverteerd naar char dus +48 voor juiste conversie naar cijfer en niet naar random teken
            codeIndex++;

            serialDebugCode(charac, enteredCode, codeIndex, pressed);
          }
        }
      }
    }

    // ------------- QUESTIONS PART ---------------
    if (!questionsDone)
    {
      uint8_t nr = getDigit();
      if (nr != -1 && nr != 255)
      {
        if (!pressed)
        {
          pressed = true;
          number = nr;
          if (nr == OK)
          {
            // Compare entered code with rigth code
            if (preset.checkQuestion(enteredAnswer.toInt()))
            {
              questionsDone = preset.questionsDone();
              answerIndex = 0;
              enteredAnswer = "";
            }
            else
            {
              answerIndex = 0;
              enteredAnswer = "";
              wrongTune();
              totalTime -= punishment(CODE_SEVERITY);
            }
          }
          else if (nr == BAD)
          {
            answerIndex = 0;
            enteredAnswer = "";
          }
          serialDebugPressed(number, pressed);
        }
      }
      else
      {
        if (pressed)
        {
          pressed = false;

          if (answerIndex == 16)
          {
            answerIndex = 0;
            enteredAnswer = "";
          }

          if (number < 10 || ((number == LEFT || number == RIGHT) && answerIndex == 0))
          {
            char charac;
            if (number == LEFT || number == RIGHT)
              charac = '-';
            else
              charac = number + 48; // uint8_t word geconverteerd naar char dus +48 voor juiste conversie naar cijfer en niet naar random teken
            enteredAnswer += charac;
            answerIndex++;

            serialDebugCode(charac, enteredCode, codeIndex, pressed);
          }
        }
      }
    }

    // ------------- CONCLUSION: leds, display ---------------
    unsigned long currentTime = millis();
    // --- TIMER ---
    if ((currentTime - startTime) / 1000 != timer)
    {
      timer = (currentTime - startTime) / 1000;
      if (DEBUG)
      {
        Serial.print("Timer: ");
        Serial.println(totalTime - timer);
      }
      char buffer[4];
      lcd.setCursor(12, 0);
      sprintf(buffer, "%4d", totalTime - timer);
      lcd.print(buffer);

      interval = decreaseInterval(totalTime, timer);
    }
    // --- BEEP ---
    if ((currentTime - intervalTime) > interval)
    {
      prevIntervalTime = intervalTime;
      intervalTime = currentTime;
      if ((currentTime - prevIntervalTime) < (interval + beepLength))
      {
        beep();
        if (!redLedOn)
        {
          controlRedLed(HIGH);
          redLedOn = true;
        }
      }
    }
    else if ((currentTime - prevIntervalTime) >= (interval + beepLength))
    {
      if (redLedOn)
      {
        controlRedLed(LOW);
        redLedOn = false;
      }
    }

    if (!questionsMode)
    {
      uint8_t amount = codeDone + wireDone + bananaDone;
      lightGreenLeds(amount);
      lcd.setCursor(0, 1);
      if (printCodeDone)
      {
        if ((millis() - codeDoneTime) < showDoneTime)
        {
          lcd.print(languageDict[17][language]); // "Code klaar!"
        }
        else if ((millis() - codeDoneTime) < (showDoneTime + 50))
        {
          lcd.print("                ");
          printCodeDone = false;
        }
      }
      else if (!codeDone && !printWireDone && !printBananaDone)
        lcd.print(formatCode(enteredCode, codeLength));
      if (printWireDone)
      {
        if ((millis() - wireDoneTime) < showDoneTime)
          lcd.print(languageDict[18][language]); // "Draad klaar!"
        else if ((millis() - wireDoneTime) < (showDoneTime + 50))
        {
          lcd.print("                ");
          printWireDone = false;
        }
      }
      if (printBananaDone)
      {
        if ((millis() - bananaDoneTime) < showDoneTime)
          lcd.print(languageDict[19][language]); // "Banana klaar!"
        else if ((millis() - bananaDoneTime) < (showDoneTime + 50))
        {
          lcd.print("                ");
          printBananaDone = false;
        }
      }
    }
    else
    {
      if (!questionsDone)
      {
        if (question != preset.getQuestion())
        {
          question = preset.getQuestion();
          if (DEBUG)
            Serial.println(question);
          lcd.setCursor(0, 0);
          lcd.print(formatQuestion(question, 11));
        }
        lcd.setCursor(0, 1);
        lcd.print(formatCode(enteredAnswer, 16));
      }
    }
  }
  return ((millis() - startTime) < totalTime * 1000L) ? Succes : Fail;
}

uint8_t getDigitSerial()
{
  if (Serial.available() > 0)
  {
    uint8_t nr = Serial.readString().toInt();
    if (nr < 10 && nr >= 0)
    {
      return nr;
    }
    else
    {
      return -1;
    }
  }
  else
  {
    return -1;
  }
}

/**
  Returns a time punishment after lighting the red led and beeping a low beep
*/
unsigned int punishment(uint8_t severity)
{
  return 0;
}

String formatCode(String code, uint8_t length)
{
  String str = code;
  for (uint8_t i = 0; i < length - code.length(); i++)
  {
    str += " ";
  }
  return str;
}

String formatQuestion(String question, uint8_t length)
{
  String str = question;
  for (uint8_t i = 0; i < length - question.length(); i++)
  {
    str += " ";
  }
  return str;
}

ConfirmState getNumberString(uint8_t lengte, uint8_t posX, uint8_t posY, String *code)
{
  bool okay = false;
  String string = "";
  int waarde;
  while (!okay)
  {
    // while(analogRead(codePin) != 0){}
    waarde = getDigit();
    if (waarde != -1 && waarde < 100)
    {
      if (string == "" || string.length() == lengte)
      {
        string = String(waarde);
      }
      else
      {
        if (DEBUG)
          Serial.println(waarde);
        string = string + String(waarde);
      }
      String printStr = string;
      for (uint8_t i = 0; i < lengte - string.length(); i++)
      {
        printStr += " ";
      }
      if (DEBUG)
        Serial.println(printStr);
      lcd.setCursor(posX, posY);
      lcd.print(printStr);
      wait(waarde);
    }
    else if (waarde == OK && string != "")
    {
      okay = true;
      wait(waarde);
    }
    else if (waarde == BAD)
    {
      if (string == "")
      {
        okay = true;
      }
      else
      {
        string = "";
        String printStr = string;
        for (uint8_t i = 0; i < lengte - string.length(); i++)
        {
          printStr += " ";
        }
        if (DEBUG)
          Serial.println(printStr);
        lcd.setCursor(posX, posY);
        lcd.print(printStr);
      }
      wait(waarde);
    }
    else if (waarde == BACK)
      return Back;
  }
  if (DEBUG)
  {
    Serial.print("Waarde:");
    Serial.println(string);
  }
  *code = string;
  return Succes;
}

void resetAll()
{
  controlRedLed(LOW);
  lightGreenLeds(0);
  noTone(piezoPin);
  lcd.clear();
}

ConfirmState changeLanguageGetPreset(int *addr)
{
  uint8_t old_lang = language;
  int langAddress = *addr;
  uint8_t lang = EEPROM.read(langAddress);
  *addr += sizeof(lang);
  language = (lang < AMOUNT_LANGUAGES) ? lang : EN;
  if (DEBUG)
  {
    Serial.print("Amount of presets stored: ");
    Serial.println(amPresets);
  }
  readPresetFromEEPROM(*addr, &previousPreset);
  lcd.clear();

  lcd.clear();
  lcd.print(languageDict[25][language]); // "VERANDER TAAL:"
  lcd.setCursor(14, 1);
  lcd.write('<');
  lcd.write('>');
  lcd.setCursor(0, 1);
  lcd.print(languages[language]);
  uint8_t nr = getDigit();
  while (nr != OK && nr != BACK)
  {
    if (nr == LEFT || nr == RIGHT)
    {
      if (nr == LEFT)
      {
        language--;
        if (language == 255 || language < 0)
        {
          language = AMOUNT_LANGUAGES - 1;
        }
      }
      else if (nr == RIGHT)
      {
        language++;
        if (language == AMOUNT_LANGUAGES)
        {
          language = 0;
        }
      }
      wait(nr);
      lcd.clear();
      lcd.print(languageDict[25][language]); // "VERANDER TAAL:"
      lcd.setCursor(14, 1);
      lcd.write('<');
      lcd.write('>');
      lcd.setCursor(0, 1);
      lcd.print(languages[language]);
    }
    nr = getDigit();
  }
  wait(nr);
  if (nr == OK)
  {
    EEPROM.update(langAddress, language);
    return Succes;
  }
  else
  {
    language = old_lang;
    return Back;
  }
}