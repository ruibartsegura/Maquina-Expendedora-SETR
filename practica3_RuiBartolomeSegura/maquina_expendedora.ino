#include <LiquidCrystal.h>
#include <Thread.h>
#include "ultrasonido.h"
#include "dht_class.h"
#include "joystick_class.h"
#include <avr/wdt.h>

/*-----------------------------------------------------------------------------*/
// States of the program
const int ARRANQUE = 0;
const int SERVICIO = 1;
const int ADMIN = 2;

int state = 0; // Initial state

/*-----------------------------------------------------------------------------*/
Thread blink_thread = Thread();

/*-----------------------------------------------------------------------------*/
// LCD display
LiquidCrystal lcd(A3, 12, 4, 7, 6, 5);

/*-----------------------------------------------------------------------------*/
// Ultrasound sensor
const int ECHO_PIN = 8;
const int TRIG_PIN = A0;
UltraSoundClass ultrasound(ECHO_PIN, TRIG_PIN);

/*-----------------------------------------------------------------------------*/
// DHT sensor
const int DHT_PIN = 10;
const uint8_t DHT_TYPE = DHT11;
DHTClass dht(DHT_PIN, DHT_TYPE);

/*-----------------------------------------------------------------------------*/
// Joystic
const int pinJoyX = A1;
const int pinJoyY = A2;
JoystickClass joystick(pinJoyX, pinJoyY);

// Joystick buttom aparto to manage the interrumption
const int pinJoyButton = 3;
volatile bool joy_pressed = false;

/*-----------------------------------------------------------------------------*/
// Leds & buttoms
const int LED_1 = 11; // Analogico
const int LED_2 = 9; // Analogico

const int BUTTON_PIN = 2; // Digital - interruption

volatile bool rise_detected = false;
volatile bool fall_detected = false;

volatile unsigned long time_rise = -1;
volatile unsigned long time_fall = -1;

unsigned long last_time = -1; // Use just in interruption function (button_timer)
byte last_state = LOW; // Use just in interruption function (button_timer)

/*-----------------------------------------------------------------------------*/
// Param to use in Arranque
int counter = 0; // Will count blinks of the led
bool led_on = false; // Check if the led is on or off

/*-----------------------------------------------------------------------------*/
// Params to use in Servicio
// Cases
const int WAIT = 0;
const int METEOR = 1;
const int CHOOSE = 2;
const int MAKE = 3;
const int READY = 4;
int service_situation = 0;

/*----------------------------*/
// Distance to customer
bool less_one_meter = false;
const int DIST = 100; // 100cm = 1m

/*----------------------------*/
// Meteorology show
const int DHT_SHOW_TIME = 5; // Time to display the meteorology, 5s
long inital_time_dht = -1; // time when we start showing the dht data

// Time the buttom should be pressed to reset Servicio
const int RESET_SERVICE_LOWER_TIME = 2;
const int RESET_SERVICE_HIGHER_TIME = 3;


/*----------------------------*/
// Products
int product = 0; // Product to display
const int MAX_PRODUCTS = 5; // How many products there are

const int SOLO = 0;
float solo_price = 1.00;

const int CORTADO = 1;
float cortado_price = 1.10;

const int DOBLE = 2;
float doble_price = 1.25;

const int PREMIUM = 3;
float premium_price = 1.50;

const int CHOCOLATE = 4;
float choco_price = 2.00;

// Array of prices to easy acces
float prices[5] = {solo_price, cortado_price, doble_price, premium_price, choco_price};

/*----------------------------*/
// Timers
// A random time between 4 and 8 both included
const int LOWER_MAKING_TIME = 4;
const int MAX_MAKING_TIME = 9; // +1 of disered max time

long making_time = -1; // How much take to make the drink
long start_making_time = 0; // When the making time has started

const int PICKUP_TIME = 3; // 3 sec
float start_pickup_time = 0;

/*-----------------------------------------------------------------------------*/
// Params to use in ADMIN
// Time the buttom should be pressed to enter in this mode
const int ADMIN_MODE_TIME = 5;

bool change_price = false; // If it is in change price mode or not
float add_price = 0.00; // Will stack the amount of money to add to th price
const float ADD_PRICE = 0.05; // Each increase/decrease will add this value

/*----------------------------*/
// Admin menu
// This will control if display the menu or display the selected feature
const int MENU = 0;
const int FEATURE = 1;
int admin_pos = 0;

// This will control which feature show for selection
int admin_sel = 0;
const int MAX_ADMIN_MENU = 4; // How many admin modes are

const int TEMP_HUM = 0;
const int DISTANCE = 1;
const int CONT = 2;
const int MOD_PRICES = 3;

/*-----------------------------------------------------------------------------*/
// Independent vars
const int BOUNCING_THRESHOLD = 100;

/*-----------------------------------------------------------------------------*/
// Specials chars
#define A 0 // position where it will be stored
byte a[8] = {
  B11100,
  B10100,
  B11110,
  B00000,
  B11110,
  B00000,
  B00000,
  B00000,

};

#define EURO 1 // position where it will be stored
byte euro[8] = {
  B00000,
  B00110,
  B01001,
  B11110,
  B01000,
  B11110,
  B01001,
  B00110,
};

#define O 2 // position where it will be stored
byte o[8] = {
  B11100,
  B10100,
  B11100,
  B00000,
  B11110,
  B00000,
  B00000,
  B00000,

};
/*-----------------------------------------------------------------------------*/

// Get the diff of times between a pass time (t0) and a actual time (t1) in secs
// Return -1 if any time is illegal
float get_time_diff(long t0, long t1) {
  if (t0 < 0 || t1 < 0)
    return -1;
  else
    return (t1 - t0) / 1000;
}

// Interruption, when the button of the joysctick is been pressed the variable
// it is set to true
void joy_press() {
  joy_pressed = true;
}

// Thread turn on or off the led each sec
void blink_led() {
  if (led_on) {
    analogWrite(LED_1, 0);
    led_on = false;
    counter++;
  } else {
    analogWrite(LED_1, 255);
    led_on = true;
  }
}

// Interruption, count how many seconds the buttom is been pressed
// When both rise_detected & fall_detected are true we can check the
// time diff
void button_timer() {
  unsigned long now = millis();

  bool state = digitalRead(BUTTON_PIN);

  // If the state is the same as the last one avoid -> help bounce
  if (state == last_state) {
    return;
  } else {
    last_state = state;
  }

  // If the times between interruptions are lower than the threshold avoid -> bounce
  if (now - last_time < BOUNCING_THRESHOLD)
    return;
  last_time = now; // Save the time for next interruption

  // Get time of rise and fall
  if (state == HIGH) {
    time_rise = now;
    rise_detected = true;
  } else {
    time_fall = now;
    fall_detected = true;
  }
}

// Display temperature and humidity
void show_meteor_data() {
  if (inital_time_dht == -1) // The initial time is not initializated
    inital_time_dht = millis();
  
  // Temp
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T");
  lcd.write(byte(A));
  lcd.print(": ");

  lcd.print(dht.get_temp());
  lcd.print(" ");
  lcd.write(byte(O));
  lcd.print("C");

  // Hum
  lcd.setCursor(0, 1);
  lcd.print("Hum: ");

  lcd.print(dht.get_hum());
  lcd.print(" %");
}

// Move the display around the products
void product_place() {
  char* direction = joystick.get_dir();  

  if (strcmp(direction, "UP") == 0) {
    // Move one position forward, when its
    // in the last pos next time will be 0
    product = (product + 1) % MAX_PRODUCTS;
  } else if (strcmp(direction, "DOWN") == 0) {
    // Move one position backward, when its in the
    // first pos next time will be last the product
    product--;
    if (product == -1)
      product = MAX_PRODUCTS - 1; // MAX_PRODUCTS starts in 1
  } 
}

// Display the name of product and price
void print_price() {
  lcd.clear();
  switch(product) {
    case SOLO:
      {
        lcd.setCursor(0, 0);
        lcd.print("Cafe solo");
        break;
      }
    case CORTADO:
      {
        lcd.setCursor(0, 0);
        lcd.print("Cafe cortado");
        break;
      }
    case DOBLE:
      {
        lcd.setCursor(0, 0);
        lcd.print("Cafe doble");
        break;
      }
    case PREMIUM:
      {
        lcd.setCursor(0, 0);
        lcd.print("Cafe premium");
        break;
      }
    case CHOCOLATE:
      {
        lcd.setCursor(0, 0);
        lcd.print("Chocolate");
        break;
      }
  }
  lcd.setCursor(0, 1);
  lcd.print(prices[product]);
  lcd.write(byte(EURO));
}

// Check if the buttom time pressed is the necesary to
// enter in admin mode
void check_admin() {
  if (rise_detected && fall_detected) {
    rise_detected = false;
    fall_detected = false;

    float diff_time = get_time_diff(time_rise, time_fall);
    if (diff_time == -1) {
      lcd.print("TIME ERROR");
      return;
    }

    if (diff_time >= ADMIN_MODE_TIME) {
      state = ADMIN;

      // The joystick buttom is used in this mode
      attachInterrupt(digitalPinToInterrupt(pinJoyButton), joy_press, FALLING);

      // Service mode default values
      service_situation = WAIT; // restart the service for the return of the normal mode
      inital_time_dht = -1;
      product = 0; // Reset the display for next time
      start_making_time = 0;
      analogWrite(LED_2, 0);
      start_pickup_time = 0;
      lcd.clear();

      // Start mode default values
      counter = 0;
      analogWrite(LED_1, 0);
    }
  }
}

// Display the admin menu
void admin_menu() {
  lcd.clear();
  switch(admin_sel) {
    case TEMP_HUM:
      {
        lcd.setCursor(0, 0);
        lcd.print("Ver temperatura");
        break;
      }
    case DISTANCE:
      {
        lcd.setCursor(0, 0);
        lcd.print("Ver distancia");
        lcd.setCursor(0, 1);
        lcd.print("sensor");
        break;
      }
    case CONT:
      {
        lcd.setCursor(0, 0);
        lcd.print("Ver contador");
        break;
      }
    case MOD_PRICES:
      {
        lcd.setCursor(0, 0);
        lcd.print("Modificar precios");
        break;
      }
  }
}

// Control joystick dir to change the price
void change_prices() {
  char* direction = joystick.get_dir();

  // If we are in feature selector then move up down, if not be able to return
  if (strcmp(direction, "UP") == 0) {
    add_price += ADD_PRICE;

  } else if (strcmp(direction, "DOWN") == 0) {
    add_price -= ADD_PRICE;

  } else if (strcmp(direction, "LEFT") == 0) {
    // Exit change price mode
    change_price = false;
    add_price = 0.0; // Restart

  } else if (joy_pressed) {
    // Add price and exit mode
    prices[product] += add_price; // Add the value
    if (prices[product] <= 0.00) // Check no negative price
      prices[product] = 0.00;

    add_price = 0.00;
    change_price = false;    
  }

  print_price();
  lcd.setCursor(4, 1);
  lcd.print(" + ");
  lcd.print(add_price);
  lcd.write(byte(EURO));

}

// Display the selected mode in admin mode
void feature_show() {
  switch(admin_sel) {
    case TEMP_HUM:
      {
        show_meteor_data();
        inital_time_dht = -1; // To avoid problems
        break;
      }
    case DISTANCE:
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Distancia: ");
        lcd.setCursor(0, 1);
        lcd.print(ultrasound.get_dist());
        lcd.print(" cm");
        break;
      }
    case CONT:
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Contador: ");
        lcd.print(get_time_diff(0, millis()));
        lcd.print(" s");
        break;
      }
    case MOD_PRICES:
      {
        // Two ways, display product to select
        // when selected be able to change the price
        if (!change_price) {
          product_place();
          print_price();
          if (joy_pressed)
            change_price = true;
        } else {
          change_prices();
        }
        break;
      }
  }
}

// Move the display around the admin modes
void admin_menu_place() {
  char* direction = joystick.get_dir();

  // If we are in feature selector then move up down, if not be able to return
  if (admin_pos == 0) {
    if (strcmp(direction, "UP") == 0) {
      admin_sel = (admin_sel + 1) % MAX_ADMIN_MENU;
    } else if (strcmp(direction, "DOWN") == 0) {
      admin_sel--;
      if (admin_sel == -1)
        admin_sel = MAX_ADMIN_MENU - 1; // MAX_ADMIN_MENU starts in 1 -> make it 0
    }
  } else {
    if (strcmp(direction, "LEFT") == 0)
      admin_pos = 0;
  }
}

/*-----------------------------------------------------------------------------*/

void setup() {
  Serial.begin(9600);
  wdt_disable();
  wdt_enable(WDTO_8S);

  // LEDS
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);

  // LCD and the special chars
  lcd.begin(16,2); // Init lcd
  lcd.createChar(A, a);
  lcd.createChar(O, o);
  lcd.createChar(EURO, euro);

  // Blink thread
  blink_thread.enabled = true;
  blink_thread.setInterval(1000);
  blink_thread.onRun(blink_led);

  // Buttoms
  pinMode(pinJoyButton, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Attach the interruption to the buttom
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_timer, CHANGE);

  // Random seed to use, pin unused
  randomSeed(analogRead(A5));
}

/*-----------------------------------------------------------------------------*/

void loop() {
  //delay(100);
  switch (state) {
    case ARRANQUE:
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("CARGANDO...");
        if(blink_thread.shouldRun())
          blink_thread.run();

        // Go to the next state
        if (counter >= 3) {
          state = SERVICIO;
          analogWrite(LED_1, 0);
        }
        check_admin(); // Check if enter admin mode
        break; // END ARRANQUE
      }

    case SERVICIO:
      {
        switch(service_situation) {
          case WAIT:
            {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("ESPERANDO");
              lcd.setCursor(0, 1);
              lcd.print("CLIENTE");

              // Go to the next situation
              if (ultrasound.get_dist() < DIST) {
                service_situation = METEOR;
                lcd.clear();
              }
              break; // END WAIT
            }
          case METEOR:
            {
              show_meteor_data();

              float duration = get_time_diff(inital_time_dht, millis());

              // Go to the next situation
              if (duration != -1 && duration > DHT_SHOW_TIME) {
                service_situation = CHOOSE;

                inital_time_dht = -1;
                lcd.clear();

                // Setup next case
                attachInterrupt(digitalPinToInterrupt(pinJoyButton), joy_press, FALLING);
              }
              break; // END METEOR 
            }

          case CHOOSE:
            {
              // Check if the joystic is changing the position
              product_place();
              // Display product and price
              print_price();

              // Go to the next situation
              if (joy_pressed == true) {
                service_situation = MAKE;

                // Stop detecting interruption while not necesary
                detachInterrupt(digitalPinToInterrupt(pinJoyButton));

                product = 0; // Reset the display for next time
                lcd.clear();

                // Setup next case
                making_time = random(LOWER_MAKING_TIME, MAX_MAKING_TIME); // Get random time
                start_making_time = millis(); // get actual time
              }
              break; // END CHOOSE
            }

          case MAKE:
            {
              float duration = get_time_diff(start_making_time, millis()); // In sec
              if (duration == -1 | making_time == -1) {
                lcd.print("TIME ERROR");
                break;
              }

              if (duration < making_time) {
                // LED intensity depends of progress
                analogWrite(LED_2, duration * (255 / making_time));
                lcd.clear();
                lcd.print("Preparando Cafe...");

              } else { // Go to the next situation
                service_situation = READY;
                
                // Turn off led and timer
                start_making_time = 0;
                analogWrite(LED_2, 0);
                lcd.clear();
                
                // Start next timer
                start_pickup_time = millis();
              }
              break; // END MAKE
            }

          case READY:
            {
              float duration = get_time_diff(start_pickup_time, millis());
              if (duration == -1) {
                lcd.print("TIME ERROR");
                break;
              }

              if (duration < PICKUP_TIME) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("RETIRE BEBIDA");

              } else { // Go to the next situation
                service_situation = WAIT;
                start_pickup_time = 0;
                lcd.clear();
              }
              break; // END READY
            }
        }

        // Restart mode
        // rise_detected & fall_detected will be reset in check_admin()
        if (rise_detected && fall_detected) {
          float diff_time = get_time_diff(time_rise, time_fall);
          if (diff_time == -1) {
            lcd.print("TIME ERROR");
            break;
          }
          
          if (diff_time >= RESET_SERVICE_LOWER_TIME && diff_time <= RESET_SERVICE_HIGHER_TIME) {
            service_situation = WAIT; // restart the service

            // Make sure everything is "off" for the restart
            inital_time_dht = -1;
            detachInterrupt(digitalPinToInterrupt(pinJoyButton));
            product = 0; // Reset the display for next time
            start_making_time = 0;
            analogWrite(LED_2, 0);
            start_pickup_time = 0;
            lcd.clear();
          }
        }

        check_admin(); // Check if enter admin mode

        joy_pressed = false; // Reset value
        break; // END servicio
      }

    case ADMIN:
      {
        analogWrite(LED_1, 255);
        analogWrite(LED_2, 255);

        switch (admin_pos) {
          case MENU:
            {
              admin_menu_place();
              admin_menu();

              if (joy_pressed)
                admin_pos = 1;
              break; // END MENU
            }
          case FEATURE:
            {
              feature_show();
              admin_menu_place(); // To change admin_pos case

              break; // END FEATURE
            }

        }

        joy_pressed = false; // restart value

        // Check if exit of admin mode
        if (rise_detected && fall_detected) {
          rise_detected = false;
          fall_detected = false;

          float diff_time = get_time_diff(time_rise, time_fall);
          if (diff_time == -1) {
            lcd.println("TIME ERROR");
            return;
          }

          if (diff_time >= ADMIN_MODE_TIME) {
            state = SERVICIO;

            // Service mode default values
            service_situation = WAIT; // restart the service for the return of the normal mode
            // Make sure everything is "off" for the restart of normal mode 
            inital_time_dht = -1;
            product = 0; // Reset the display for next time
            start_making_time = 0;
            analogWrite(LED_2, 0);
            start_pickup_time = 0;
            lcd.clear();

            // Start mode default values
            counter = 0;
            analogWrite(LED_1, 0);
          }
        }

        break;
      } // END ADMIN
  }
  wdt_reset();
  delay(100);
}
