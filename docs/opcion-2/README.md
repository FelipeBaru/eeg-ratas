# Opción 2 — EEG inalámbrico (ESP8266 + ADS1115 + CD4051B)

Esta carpeta documenta las **decisiones de diseño de la opción 2** del proyecto y
explica el código que se sube al ESP8266. Es la rama **inalámbrica y escalable**
del electroencefalógrafo.

> La opción 1 (con el digitalizador NI USB-6003 conectado por cable USB) es la
> versión de banco, de máxima calidad de señal, y está descrita en la bitácora
> general del proyecto (`bitacora.md`). Esta opción 2 es la que permite sacar el
> cable y transmitir por WiFi, a costa de algo de fidelidad.

---

## Qué hace la opción 2

Toma la señal de 4 electrodos, la acondiciona (amplifica + filtra), la digitaliza
y la transmite por WiFi a la computadora, donde se ve **cada canal graficado en
función del tiempo**.

El ESP8266 crea su **propia red WiFi** con su propia dirección IP. La computadora
se conecta a esa red y abre una página web que sirve el mismo ESP. No hay ningún
cable entre el equipo de medición y la compu: esto da **aislación total** y evita
el ruido de red de 50 Hz (es lo que recomienda el paper de ILAE sobre EEG en
animales, ver `docs/ILAE-Moyer-2017.pdf`).

---

## La cadena de señal

```
electrodo --> CD4051B --> AD620 --> pasa-altos 1Hz --> pasa-bajos 40Hz --> sumador --> ADS1115 --> ESP8266 --> WiFi --> compu
              (rota)      (amplifica)        (filtros Bessel)            (+1.65V)     (A0, 16bit)
```

Hay **un solo circuito de acondicionamiento** (un AD620 + un juego de filtros). El
multiplexor CD4051B **rota** entre los 4 electrodos: el ESP le indica cuál conectar,
lo lee, y pasa al siguiente. Por eso el ADS1115 lee **siempre su entrada A0** (la
salida de ese único front-end): quien cambia de canal es el MUX, no el ADC.

---

## Componentes y su rol

| Componente | Rol |
|---|---|
| NodeMCU ESP8266 | Cerebro. Controla el MUX, lee el ADC por I2C y transmite por WiFi. |
| CD4051B | Multiplexor analógico. Rota entre los 4 electrodos. |
| AD620 (módulo) | Amplificador de instrumentación. Levanta la señal de microvoltios. |
| Filtros Bessel | Dejan pasar solo 1–40 Hz (las bandas Delta a Beta). |
| ADS1115 | Conversor analógico→digital de 16 bits, comunicación I2C. |

---

## Decisiones clave (y por qué)

1. **Un solo front-end + MUX que rota** (en vez de 4 circuitos en paralelo).
   Es más barato y usa menos componentes. La contra: los 4 canales **no se leen
   exactamente al mismo tiempo** (hay un pequeño desfase entre ellos) y la
   velocidad queda limitada. Para un primer demo inalámbrico es un intercambio
   aceptable. (En la opción 1, con el NI, sí van 4 cadenas en paralelo.)

2. **El ADS1115 lee siempre A0.** Como el MUX ya eligió el canal, el ADC solo
   tiene que leer la salida del front-end, que entra por A0.

3. **Offset de +1.65 V antes del ADS1115.** El ADS1115 en modo single-ended (A0
   respecto a GND) solo mide tensiones **positivas** (de 0 a 3.3 V). Pero la señal
   que sale de los filtros oscila en **±** (alrededor de 0 V). Por eso el front-end
   le suma un offset de ~1.65 V (la mitad del rango) para **centrar la señal** y
   que nunca sea negativa. En el software se le **resta** ese 1.65 V para
   reconstruir la señal real. Ese es el bloque "post amplificador y sumador" del
   diagrama de ITBA.

4. **El ADS1115 y la lógica del MUX van a 3.3 V, no a 5 V.** Las líneas I2C
   (SDA/SCL) del ADS1115 se conectan al ESP8266, que es de 3.3 V y **no tolera
   5 V** en sus pines. Alimentar el ADS a 5 V pondría esas líneas a 5 V y podría
   dañar el ESP.

5. **El VEE del CD4051B va a −5 V (no a GND).** La señal cruda del electrodo
   oscila por arriba y por debajo de cero. El CD4051B solo deja pasar tensiones
   que estén entre su VEE y su VDD; si VEE fuera GND (0 V), recortaría toda la
   parte negativa de la señal. Por eso VEE se conecta al riel **−5 V** de la fuente
   partida.

6. **El ESP se alimenta con power bank, no desde el USB de la compu.** Esta es la
   gracia de la versión inalámbrica: al transmitir por WiFi no hace falta cable a
   la compu, así que no se introduce masa de red ni lazos de masa.

7. **Una sola masa (topología estrella).** El GND de la fuente partida, el VSS del
   CD4051B, el GND del ADS1115 y el GND del ESP se unen todos en un único punto.

---

## Alimentación

Cada chip a su tensión. **Esto es lo más importante de no equivocar:**

| Parte | Tensión | De dónde sale |
|---|---|---|
| AD620 + opamps de los filtros | ±5 V | Fuente partida (baterías) |
| CD4051B — VDD (pin 16) | +3.3 V | Pin 3V3 del NodeMCU |
| CD4051B — VSS (pin 8) | GND | Masa común |
| CD4051B — VEE (pin 7) | **−5 V** | Riel negativo de la fuente partida |
| CD4051B — INH (pin 6) | GND | Masa común (deja el MUX habilitado) |
| ADS1115 — VDD | **+3.3 V** | Pin 3V3 del NodeMCU |
| NodeMCU ESP8266 | 5 V | **Power bank / batería** (NO la compu) |

La fuente partida ±5 V (2 baterías de 9 V en serie + reguladores 7805/7905) está
documentada en la bitácora general; ver también el diagrama en la carpeta
`hardware/` si se guardó.

---

## Front-end analógico (acondicionamiento)

El diseño detallado de los filtros está en el PDF de ITBA
(`docs/ITBA-EEG-telemetria.pdf`, páginas del esquemático). Resumen:

- **AD620 (amplificador):** la ganancia se ajusta con el trimpot del módulo. Para
  EEG intracraneal (señal más grande que la de cuero cabelludo) conviene arrancar
  con ganancia ~1000 e ir ajustando mientras se mide, cuidando que no sature.
- **Pasa-altos Bessel, 3er orden, corte 1 Hz, topología Sallen-Key.** Elimina el
  offset de continua y la deriva de baja frecuencia.
- **Pasa-bajos Bessel, 3er orden, corte 40 Hz, topología Sallen-Key.** Deja pasar
  hasta Beta y descarta Gamma. Como corta en 40 Hz, **ya atenúa los 50 Hz de red**,
  así que no hace falta un filtro notch.
- **Sumador con offset (+1.65 V):** centra la señal en el medio del rango del ADS1115.
- **Regla de seguridad:** la señal que entra al ADS1115 **nunca** debe superar 3.3 V
  ni bajar de 0 V. Ganancia + offset tienen que mantenerla dentro de esa ventana.

---

## Cómo se usa (una vez armado el hardware)

1. Compilar el firmware del ESP: `pio run -e nodemcuv2` (desde la carpeta `firmware/`).
2. Subir el firmware al NodeMCU por USB: `pio run -e nodemcuv2 -t upload`.
   - El NodeMCU se programa por USB igual que el Arduino Uno.
   - Muchos NodeMCU traen el chip **CH340**, que en Windows necesita instalar su driver.
3. En la computadora, conectarse a la red WiFi **`EEG_Paz`** (clave `eeg12345`).
4. Abrir el navegador en **`192.168.4.1`**.
5. Se ven los 4 canales graficándose en vivo. Al tocar un electrodo, ese canal se mueve.

---

## Limitaciones conocidas (versión v0)

- **Velocidad:** el ADS1115 llega a 860 muestras/seg **totales**. Repartido en 4
  canales y con el tiempo de conmutación del MUX, quedan ~150 muestras/seg por
  canal. Alcanza para la banda de hasta 40 Hz (Nyquist pide >80/seg), pero queda
  cerca del límite.
- **Onda fina vs. respuesta:** esta v0 actualiza el gráfico ~20 veces por segundo,
  así que se ve que cada canal **responde**, pero todavía no la forma de onda con
  fidelidad. Eso se mejora en el siguiente paso (streaming por bloques).
- **Canales no simultáneos:** como el MUX rota, hay un pequeño desfase temporal
  entre un canal y otro.

---

## Próximos pasos

- **Gate B — streaming por bloques:** que el ESP junte las muestras en paquetes y
  las transmita seguidas, para reconstruir la onda real de cada canal con fidelidad.
- **Guardado de datos** para análisis posterior.
- **Detección de crisis** (más adelante): puede requerir ensanchar la banda por
  encima de 40 Hz, porque las espigas epileptiformes tienen energía más alta.

---

## Estructura de archivos

- `README.md` (este archivo): decisiones de diseño y panorama general.
- `firmware-explicado.md`: el código del ESP, explicado bloque por bloque.
- Código ejecutable (la copia que se compila): `firmware/src/eeg_esp8266.cpp` y
  `firmware/platformio.ini`.
- Diseño de los filtros: `docs/ITBA-EEG-telemetria.pdf`.
