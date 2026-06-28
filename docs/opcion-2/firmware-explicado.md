# Firmware del ESP8266 — explicado bloque por bloque

Este documento explica el código que se sube al NodeMCU ESP8266 en la opción 2.
La copia que se compila está en `firmware/src/eeg_esp8266.cpp`. Acá está el mismo
código, con la explicación de qué hace cada parte.

## Qué hace el programa, en una frase

Crea una red WiFi propia, rota el multiplexor por los 4 canales leyendo el ADS1115
en cada uno, convierte cada lectura a voltios, y sirve una página web con un
gráfico en vivo de los 4 canales. No hay que instalar nada en la compu: alcanza
con conectarse a la red del ESP y abrir el navegador.

---

## 1. Librerías que usa

```cpp
#include <Arduino.h>
#include <ESP8266WiFi.h>        // para crear la red WiFi
#include <ESP8266WebServer.h>   // para servir la página web
#include <Wire.h>               // comunicación I2C (para el ADS1115)
#include <Adafruit_ADS1X15.h>   // driver del ADS1115
```

La librería del ADS1115 (`Adafruit ADS1X15`) la baja PlatformIO solo, porque está
declarada en `platformio.ini`. Las otras tres vienen con el ESP8266.

---

## 2. Pines del multiplexor CD4051B

```cpp
#define PIN_A D5   // GPIO14 -> CD4051B pin 11 (A, bit0)
#define PIN_B D6   // GPIO12 -> CD4051B pin 10 (B, bit1)
#define PIN_C D7   // GPIO13 -> CD4051B pin 9  (C, bit2)
```

Son las tres líneas de **selección de canal** del CD4051B. Con tres bits (A, B, C)
se eligen los 8 canales posibles del MUX; nosotros usamos del 0 al 3. El número de
canal en binario es C-B-A (por ejemplo: canal 2 = 010 -> C=0, B=1, A=0).

El resto de los pines del MUX van fijos al hardware (no se controlan por software):
INH (pin 6) a GND, VEE (pin 7) a −5 V, VDD (pin 16) a 3.3 V.

---

## 3. El ADS1115

```cpp
Adafruit_ADS1115 ads;
```

Crea el objeto que maneja el conversor. Se comunica por I2C, que en el NodeMCU usa
los pines D2 (SDA) y D1 (SCL); eso se configura más abajo en `setup()`.

---

## 4. La red WiFi (modo Access Point)

```cpp
const char* AP_SSID = "EEG_Paz";
const char* AP_PASS = "eeg12345";   // minimo 8 caracteres
ESP8266WebServer server(80);
```

"Access Point" significa que **el ESP crea su propia red**, en vez de conectarse a
una existente. Así no depende del WiFi de la casa y siempre tiene la misma
dirección. La compu se conecta a la red `EEG_Paz` con la clave `eeg12345`.

El `server(80)` es el servidor web que va a entregar la página (el 80 es el puerto
estándar de las páginas web).

---

## 5. Dónde se guardan las lecturas

```cpp
const int NUM_CANALES = 4;
volatile float voltios[NUM_CANALES] = {0, 0, 0, 0};
```

Un casillero por canal con el último valor leído, en voltios. Es lo que la página
web va a pedir y graficar.

---

## 6. Elegir un canal del MUX

```cpp
void seleccionarCanal(int ch) {
  digitalWrite(PIN_A, bitRead(ch, 0));   // bit 0
  digitalWrite(PIN_B, bitRead(ch, 1));   // bit 1
  digitalWrite(PIN_C, bitRead(ch, 2));   // bit 2
}
```

Toma el número de canal (0 a 3) y escribe sus tres bits en las patas A, B y C. A
partir de ese momento, el MUX conecta ese electrodo a la entrada del front-end.

---

## 7. La página web (gráfico en vivo)

```cpp
const char PAGINA[] PROGMEM = R"HTML( ... )HTML";
```

Es una página HTML completa guardada como texto dentro del ESP. No usa ninguna
librería de internet, así que **funciona aunque la compu no tenga internet**
(porque está conectada a la red del ESP, no a la web).

La página hace tres cosas:
- Cada 50 milisegundos le pide los datos al ESP (a la dirección `/data`).
- Guarda los últimos 400 valores de cada canal.
- Los dibuja en un `canvas` (una zona de dibujo), un canal arriba del otro, y
  muestra el valor numérico de cada uno.

Detalle importante: al dibujar, la página **resta 1.65 V** a cada valor
(`datos[i][k] - 1.65`). Eso deshace el offset que el circuito le sumó a la señal,
así el gráfico muestra la señal centrada en cero, como es en realidad.

---

## 8. Las dos "rutas" que responde el ESP

```cpp
void handleRoot(){ server.send_P(200, "text/html", PAGINA); }
```

Cuando el navegador entra a `192.168.4.1`, el ESP le manda la página.

```cpp
void handleData(){
  String j = "[";
  for (int i=0;i<NUM_CANALES;i++){ j += String(voltios[i],4); if(i<NUM_CANALES-1) j += ","; }
  j += "]";
  server.send(200, "application/json", j);
}
```

Cuando la página pide `/data`, el ESP le manda los 4 valores actuales en formato
de lista (por ejemplo `[1.65,1.71,1.60,1.66]`). La página los usa para seguir
dibujando.

---

## 9. setup() — se ejecuta una vez al encender

```cpp
void setup(){
  Serial.begin(115200);
  pinMode(PIN_A, OUTPUT); pinMode(PIN_B, OUTPUT); pinMode(PIN_C, OUTPUT);

  Wire.begin(D2, D1);                       // SDA, SCL
  if(!ads.begin()) Serial.println("ADS1115 no encontrado - revisar I2C");
  ads.setGain(GAIN_ONE);                    // rango +-4.096 V
  ads.setDataRate(RATE_ADS1115_860SPS);     // lo mas rapido posible

  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("Red WiFi creada. Entra a la IP: ");
  Serial.println(WiFi.softAPIP());          // normalmente 192.168.4.1

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}
```

Paso por paso:
- Configura las tres patas del MUX como salidas.
- Arranca el I2C y el ADS1115. Si no lo encuentra, lo avisa por el monitor serie
  (suele ser un problema de cableado del I2C o de alimentación del ADS).
- `setGain(GAIN_ONE)` fija el rango de medición del ADS en ±4.096 V (cómodo para
  nuestra señal de 0 a 3.3 V).
- `setDataRate(...860SPS)` lo pone a su máxima velocidad.
- Crea la red WiFi y muestra la IP por el monitor serie (casi siempre `192.168.4.1`).
- Registra las dos rutas (`/` y `/data`) y enciende el servidor.

---

## 10. loop() — se repite todo el tiempo

```cpp
void loop(){
  for(int ch=0; ch<NUM_CANALES; ch++){
    seleccionarCanal(ch);
    delayMicroseconds(200);                 // asentamiento del MUX
    int16_t raw = ads.readADC_SingleEnded(0);
    voltios[ch] = ads.computeVolts(raw);
  }
  server.handleClient();
}
```

En cada vuelta:
- Recorre los 4 canales. Para cada uno: selecciona el canal en el MUX, espera
  200 microsegundos a que la señal se estabilice (el MUX necesita un instante para
  "asentar"), lee el ADS1115 por A0 y guarda el valor convertido a voltios.
- Después atiende al navegador (`handleClient`), por si pidió la página o los datos.

Ese pequeño `delayMicroseconds(200)` es importante: sin esa espera, se leería la
señal del canal anterior antes de que termine de cambiar.

---

## Cómo compilarlo y subirlo

- Compilar (verifica que el código cierra): `pio run -e nodemcuv2`
- Subir al NodeMCU por USB: `pio run -e nodemcuv2 -t upload`
- En Windows, si el NodeMCU trae el chip CH340, instalar antes su driver.
- El monitor serie (`pio device monitor -e nodemcuv2`) muestra la IP y los avisos.
