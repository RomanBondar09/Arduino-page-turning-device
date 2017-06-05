#include <Servo.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

#define SERVO_FORWARD_PIN 3
#define SERVO_BACK_PIN 11
#define DELAY_PER_DEGREE 3
#define BUTTON_DELAY 200

#define ADDRESS_DELAY_DOWN 0
#define ADDRESS_DELAY_UP 2
#define ADDRESS_DOWN_POSITION 4
#define ADDRESS_UP_POSITION 6
#define ADDRESS_FORWARD_COUNT 8
#define ADDRESS_BACK_COUNT 10

Servo serv_forward;
Servo serv_back;

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);


enum status_servo {pause, move_down, down, move_up, up};
enum current_servo {forward, back};
enum pressed_button {BUTTON_NONE, BUTTON_RIGHT, BUTTON_UP, BUTTON_DOWN, BUTTON_LEFT, BUTTON_SELECT};

int delay_down; //задержка внизу
int delay_up; //задержка вверху
int down_position; //нижнее положение
int up_position; //верхнее положение
int forward_count; //количество листаний вперед, перед тем как начать листать назад
int back_count; //количество листаний назад, перед тем как начать листать вперед
int paged_forward; //всего было пролистано вперед 
int paged_back; //всего было пролистано назад
unsigned long previous_time = 0;
unsigned long pressed_button_time = 0;
unsigned long current_time = 10000;
status_servo status = pause; //состояние сервопривода
int delay_serv = (up_position - down_position) * DELAY_PER_DEGREE; //задержка на движение сервопривода

Servo *serv; //сервопривод, с которым работаем в данный момент
int current_paging_count; //счетчик для текущего сервопривода
int *count; //
current_servo current; //напрвление листания в текущий момент

const char TIME_UP[] = "Up delay:";
const char TIME_DOWN[] = "Down delay:";
const char POS_UP[] = "Up pos:";
const char POS_DOWN[] = "Down pos:";
const char PAGING_FORWARD[] = "Frd:";
const char PAGING_BACK[] = "Back";
const char COUNT_FORWARD[] = "Count frd";
const char COUNT_BACK[] = "Count back";

enum screen {screen0, screen1, screen2};

int current_screen = 0;
int position_cursor = 0;

void setup()
{
  EEPROM.get(ADDRESS_DELAY_DOWN, delay_down);
  EEPROM.get(ADDRESS_DELAY_UP, delay_up);
  EEPROM.get(ADDRESS_DOWN_POSITION, down_position);
  EEPROM.get(ADDRESS_UP_POSITION, up_position);
  EEPROM.get(ADDRESS_FORWARD_COUNT, forward_count);
  EEPROM.get(ADDRESS_BACK_COUNT, back_count);
  
  lcd.begin(16, 2);
  digitalWrite(10, LOW);
  serv_forward.attach(SERVO_FORWARD_PIN);
  serv_back.attach(SERVO_BACK_PIN);
  
  serv_forward.write(up_position);
  serv_back.write(up_position);
  delay(delay_serv);
  
  serv = &serv_forward;
  current_paging_count = 0;
  current = forward;
  
  print_menu();
  print_value();
}

void loop()
{
  move_servo();
  buttons_handle();
  current_time = millis();
}

void buttons_handle()
{
  static int previous_button, current_button;
  switch(current_button = getPressedButton())
  {
    case BUTTON_UP:
      if (BUTTON_UP != previous_button)
      {
        if (current_screen == 0)
        {
          current_screen = 3;
        }
        else
        {
          --position_cursor;
          if (position_cursor == -1)
          {
            position_cursor = 1;
            --current_screen;
            if (current_screen == -1)
              current_screen = 3;
          }
        }
        print_menu();
        print_value();
        move_cursor();
      }
      break;
    case BUTTON_DOWN:
      if (BUTTON_DOWN != previous_button)
      {
        if (current_screen == 0)
        {
          current_screen = 1;
        }
        else
        {
          ++position_cursor;
          if (position_cursor == 2)
          {
            position_cursor = 0;
            ++current_screen;
            if (current_screen == 4)
              current_screen = 0;
          }
        }
        print_menu();
        print_value();
        move_cursor();
      }
      break;
	case BUTTON_LEFT:
	  if (current_time - pressed_button_time >= BUTTON_DELAY)
	  {
      if (current_screen == 1 && position_cursor == 0 && forward_count != 0)
      {
        --forward_count;
        EEPROM.put(ADDRESS_FORWARD_COUNT, forward_count);
      }
	    else if (current_screen == 1 && position_cursor == 1 && back_count != 0)
      {
        --back_count;
        EEPROM.put(ADDRESS_BACK_COUNT, back_count);
      }
      else if (current_screen == 2 && position_cursor == 0 && delay_up != 0)
      {
        delay_up -= 1000;
        EEPROM.put(ADDRESS_DELAY_UP, delay_up);
      }
      else if (current_screen == 2 && position_cursor == 1 && delay_down != 0)
      {
        delay_down -= 1000;
        EEPROM.put(ADDRESS_DELAY_DOWN, delay_down);
      }
      else if (current_screen == 3 && position_cursor == 0 && up_position != 0)
      {
        --up_position;
        EEPROM.put(ADDRESS_UP_POSITION, up_position);
      }
      else if (current_screen == 3 && position_cursor == 1 && down_position != 0)
      {
        --down_position;
        EEPROM.put(ADDRESS_DOWN_POSITION, down_position);
      }
		  pressed_button_time = millis();
      print_value();
      move_cursor();
    }
	  break;
	case BUTTON_RIGHT:
    if (current_time - pressed_button_time >= BUTTON_DELAY)
    {
      if (current_screen == 1 && position_cursor == 0 && forward_count != 9999)
      {
        ++forward_count;
        EEPROM.put(ADDRESS_FORWARD_COUNT, forward_count);
      }
      else if (current_screen == 1 && position_cursor == 1 && back_count != 9999)
      {
        ++back_count;
        EEPROM.put(ADDRESS_BACK_COUNT, back_count);
      }
      else if (current_screen == 2 && position_cursor == 0 && delay_up != 99990000)
      {
        delay_up += 1000;
        EEPROM.put(ADDRESS_DELAY_UP, delay_up);
      }
      else if (current_screen == 2 && position_cursor == 1 && delay_down != 99990000)
      {
        delay_down += 1000;
        EEPROM.put(ADDRESS_DELAY_DOWN, delay_down);
      }
      else if (current_screen == 3 && position_cursor == 0 && up_position != 180)
      {
        ++up_position;
        EEPROM.put(ADDRESS_UP_POSITION, up_position);
      }
      else if (current_screen == 3 && position_cursor == 1 && down_position != 180)
      {
        ++down_position;
        EEPROM.put(ADDRESS_DOWN_POSITION, down_position);
      }
      pressed_button_time = millis();
      print_value();
      move_cursor();
    }
    break;
  case BUTTON_SELECT:
    if (BUTTON_SELECT != previous_button)
    {
      static status_servo previous_status = up;
      if (status == pause)
      {
        status = previous_status;
      }
      else
      {
        previous_status = status;
        status = pause;
      }
      Serial.println(status);
    }
    break;
  }
  previous_button = current_button;
}

void move_servo()
{
  if (*count)
  {
    if (status == up && current_time - previous_time >= delay_up)
    {
      previous_time = millis();
      (*serv).write(down_position);
      status = move_down;
    }
    else if (status == move_down && current_time - previous_time >= delay_serv)
    {
      previous_time = millis();
      status = down;
      if (current == forward)
        ++paged_forward;
      else
        ++paged_back;
      if (current_screen == 0)
        print_value();
    }
    else if (status == down && current_time - previous_time >= delay_down)
    {
      previous_time = millis();
      (*serv).write(up_position);
      status = move_up;
      ++current_paging_count;
    }
    else if (status == move_up && current_time - previous_time >= delay_serv)
    {
      previous_time = millis();
      status = up;
    }
  }
  if (current_paging_count == *count)
  {
    current_paging_count = 0;
    if (current == forward)
    {
      serv = &serv_back;
      count = &back_count;
      current = back;
    }
    else
    {
      serv = &serv_forward;
      count = &forward_count;
      current = forward;
    }
  }
}

void print_menu()
{
  lcd.clear();
  switch(current_screen)
  {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print(COUNT_FORWARD);
      lcd.setCursor(0, 1);
      lcd.print(COUNT_BACK);
      break;
    case 1:
      lcd.setCursor(0, 0);
      lcd.print(PAGING_FORWARD);
      lcd.setCursor(0, 1);
      lcd.print(PAGING_BACK);
      break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print(TIME_UP);
      lcd.setCursor(0, 1);
      lcd.print(TIME_DOWN);
      break;
    case 3:
      lcd.setCursor(0, 0);
      lcd.print(POS_UP);
      lcd.setCursor(0, 1);
      lcd.print(POS_DOWN);
      break;
  }
}

void print_value()
{
  switch(current_screen)
  {
    case 0:
      lcd.noCursor();
      lcd.setCursor(12, 0);
      print_number(paged_forward);
      lcd.setCursor(12, 1);
      print_number(paged_back);
      break;
    case 1:
      lcd.cursor();
      lcd.setCursor(12, 0);
      print_number(forward_count);
      lcd.setCursor(12, 1);
      print_number(back_count);
      break;
    case 2:
      lcd.cursor();
      lcd.setCursor(12, 0);
      print_number(delay_up / 1000);
      lcd.setCursor(12, 1);
      print_number(delay_down / 1000);
      break;
    case 3:
      lcd.cursor();
      lcd.setCursor(12, 0);
      print_number(up_position);
      lcd.setCursor(12, 1);
      print_number(down_position);
      break;
  }
}

int get_digit(int number)
{
  int i = 1;
  while (number /= 10)
    ++i;
  return i;
}

void print_number(int number)
{
  int digit = get_digit(number);
  for (int amout_space = 4 - digit; amout_space > 0; --amout_space)
    lcd.print(" ");
  lcd.print(number);
}

void move_cursor()
{
  lcd.setCursor(15, position_cursor);
}

int getPressedButton()
{
  int buttonValue = analogRead(0); // считываем значения с аналогового входа(A0)
  if (buttonValue >= 800)
  {
    return BUTTON_NONE;
  }
  else if (buttonValue < 100)
  {
    return BUTTON_RIGHT;
  }
  else if (buttonValue < 200)
  {
    return BUTTON_UP;
  }
  else if (buttonValue < 400)
  {
    return BUTTON_DOWN;
  }
  else if (buttonValue < 600)
  {
    return BUTTON_LEFT;
  }
  else if (buttonValue < 800)
  {
    return BUTTON_SELECT;
  }
}
