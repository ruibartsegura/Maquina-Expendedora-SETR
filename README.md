# Maquina-Expendedora-SETR
## Rui Bartolomé Segura
Controlador para una máquina expendedora que esté basado en Arduino UNO y en los sensores/actuadores que se proporcionan en el kit Arduino

## 1.Organización del código
Para controlar el código he diseñado una máquina de estados con los 3 estados propuestos, Arranque, Servicio y Admin. Dentro de cada etado se ejecutan las funcionalidades descritas en el [enunciado](...)

## 2.Esquema del montaje

## 3.Herramientas Usadas en el código
### Threads:
Para el control del encendido y apagado del led he usado un thread, con un periodo de 1 segundo.
``` c
void setup() {
  // Blink thread
  blink_thread.enabled = true;
  blink_thread.setInterval(1000);
  blink_thread.onRun(blink_led);
}

```
En este thread se comprueba si el led esta encendido para apagarlo y sumar uno al contador que al llegar a 3 hará que pase el programa a la siguiente función. De estar apagado lo encenderá.
``` c
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
```
La comprobación de si toca llamar a este thread se hace en en el loop en el primer caso, Arranque el cual una vez se ha encendido y apagado el LED 3 veces pasará al siguiente caso.
``` c
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
```
<br>

### Interrupciones:
Ambos botones de la implementación funcionan con interrupciones.

#### Botón normal:
El botón normal está activo siempre, configurado en modo CHANGE para detectar los cambios de estado tanto de subida como de bajada.
``` c
// Attach the interruption to the buttom
attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_timer, CHANGE);
```
Está vinculado a una función que guarda el momento en el que se pulsa y en el que se suelta permitiendo asi obtener el tiempo total que estuvo pulsado.
Para evitar los rebotes en está función se compara que el estado en el que esté el botón es distinto al anterior, vease si estaba en HIGH ahora esté en LOW y que el tiempo trnascurrido entre pulsación y pulsación sea mayor que el umbral definido, en mi caso este es de 100ms.
``` c
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
```
<br>

#### Botón del joystick:
Este botón se activa y desactiva cuando se necesita, en mi caso es cuando dentro del caso Servicio paso de mostrar la temperatura y humedad a la selección de bebida, una vez seleccionada se desvincula. Lo vuelvo a vincular cuando entro dentro del caso Admin para las selecciones de modo de este caso, una vez sale del modo Administrador se desvincula. Añadir que está configurado en modo Falling.<br>
La función vinculada a esta interrupción es sencilla, pone una variable a true, así el programa principal sabe si fue pulsado o no, al terminar cada iteración el programa pone esta msima variable a false.
No hace falta controlar los rebotes en etá función ya que solo va a detectar los cambios de caída y nos da igual que haya más de uno en un corto periodo de tiempo debido a que solo se necesita saber si se pulsó o no.
``` c
// Interruption, when the button of the joysctick is been pressed the variable
// it is set to true
void joy_press() {
  joy_pressed = true;
}
```
<br>

### Whatchdog:
El programa cuenta con un watchdog definido en 8 segundos














