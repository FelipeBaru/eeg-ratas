# Opción 1 — EEG de banco (NI USB-6003 + 4 cadenas en paralelo)

Esta carpeta documenta las **decisiones de diseño de la opción 1** del proyecto.
Es la versión de **banco, con cable USB y máxima calidad de señal**: el primer
demo robusto.

> La opción 2 (inalámbrica, con ESP8266 + ADS1115 + CD4051B) está documentada en
> `docs/opcion-2/`. Permite sacar el cable y transmitir por WiFi, a costa de algo
> de fidelidad. Las dos ramas comparten el mismo front-end analógico (amplificador
> + filtros); cambia cómo se digitaliza y se transmite la señal.

---

## Qué hace la opción 1

Toma la señal de 4 electrodos, la acondiciona (amplifica + filtra) con **un
circuito completo por canal**, y entrega los 4 canales **directo al digitalizador
NI USB-6003**, que los lee y los manda a la computadora por USB. En la compu se
grafica y se guarda cada canal en función del tiempo (con Python o LabVIEW).

Es la versión de mejor calidad: cada canal tiene su propia cadena de amplificación
y los 4 se leen prácticamente al mismo tiempo.

---

## La cadena de señal

```
electrodo 1 --> AD620 + filtros Bessel 1-40 Hz --> entrada A0 del NI USB-6003
electrodo 2 --> AD620 + filtros Bessel 1-40 Hz --> entrada A1 del NI USB-6003
electrodo 3 --> AD620 + filtros Bessel 1-40 Hz --> entrada A2 del NI USB-6003
electrodo 4 --> AD620 + filtros Bessel 1-40 Hz --> entrada A3 del NI USB-6003
                                                          |
                                                        USB --> compu (Python / LabVIEW)
```

Hay **4 circuitos de acondicionamiento en paralelo**, uno por electrodo. No hay
multiplexor: cada canal entra por su propia entrada diferencial del NI.

---

## Componentes y su rol

| Componente | Rol |
|---|---|
| NI USB-6003 | Digitalizador profesional: 16 bits, 4 entradas diferenciales. Lee y manda a la compu por USB. |
| AD620 (x4) | Amplificador de instrumentación, uno por canal. Levanta la señal de microvoltios. |
| Filtros Bessel (x4) | Dejan pasar solo 1-40 Hz (bandas Delta a Beta), uno por canal. |
| Fuente partida +-5 V | Alimenta los amplificadores y filtros. 2 baterías de 9 V + reguladores 7805/7905. |

---

## Decisión de arquitectura (la más importante)

Había dos formas de meter 4 canales al digitalizador:

- **MUX-primero (la del PDF de ITBA, es la opción 2):** un solo circuito analógico
  + un multiplexor CD4051B que rota entre electrodos. Más barato, pero conmuta
  señales de microvoltios **antes** de amplificarlas (mete ruido) y no toma los 4
  canales a la vez.
- **4 en paralelo (la que elegimos para la opción 1):** un circuito analógico
  completo por canal, y los 4 entran directo al digitalizador. Más componentes,
  pero señal mucho más limpia y 4 canales simultáneos.

**Dato que cerró la decisión:** el NI USB-6003 ya trae adentro su propio
multiplexor + amplificador + ADC (16 bits, 100 kS/s, 4 entradas diferenciales).
Ponerle un CD4051B externo adelante, para 4 canales, sería redundante. Por eso en
la opción 1 el multiplexor **no se usa**.

(Detalle técnico: el NI no es de muestreo estrictamente simultáneo, pero a
100 kS/s repartidos en 4 canales el desfase entre canales es de microsegundos:
despreciable para EEG.)

---

## Ganancia: por qué es obligatoria

El NI USB-6003, en su rango de +-10 V con 16 bits, resuelve pasos de ~300
microvoltios, y su exactitud ronda las decenas de milivoltios. La señal cruda del
electrodo (microvoltios) sería **invisible** para el digitalizador. Por eso el
front-end **tiene que amplificar fuerte** (ganancia ~1000-5000 con el AD620), para
llevar la señal a escala de voltios antes de que entre al NI. Esto no es opcional.

---

## Alimentación

El front-end (los 4 AD620 + los filtros) se alimenta con **fuente partida +-5 V**.
Se arma con 2 baterías de 9 V en serie (el punto medio es GND) + reguladores
7805 (+5 V) y 7905 (-5 V). El diagrama está en la carpeta `hardware/` (si se
guardó) y los pasos de armado y verificación, en la bitácora.

**Punto clave de aislación:** durante la medición, la laptop también a batería
(desenchufada de la pared). El NI se alimenta por USB desde la compu; si la compu
está enchufada, se reintroduce masa de red y reaparece el ruido de 50 Hz. Con la
laptop a batería, ese problema desaparece. Y una sola masa para todo (topología
estrella).

---

## Front-end analógico (acondicionamiento)

Igual que en la opción 2, pero replicado 4 veces. El diseño de los filtros está en
`docs/ITBA-EEG-telemetria.pdf`. Por canal:

- **AD620:** ganancia ~1000-5000, ajustable con el trimpot del módulo. Ajustar
  midiendo que no sature.
- **Pasa-altos Bessel, 3er orden, 1 Hz, Sallen-Key:** elimina offset de continua y
  deriva de baja frecuencia.
- **Pasa-bajos Bessel, 3er orden, 40 Hz, Sallen-Key:** deja pasar hasta Beta y
  descarta Gamma. Como corta en 40 Hz, ya atenúa los 50 Hz de red (no hace falta notch).

A diferencia de la opción 2, **acá no hace falta el sumador con offset**: el NI
mide tensiones positivas y negativas, así que la señal entra centrada en cero tal
como sale de los filtros.

---

## Cómo se usa (una vez armado el hardware)

1. Instalar en la compu el driver NI-DAQmx (de National Instruments).
2. Conectar las 4 salidas del front-end a las 4 entradas diferenciales del NI
   (A0 a A3) y el NI a la compu por USB.
3. Leer y graficar los 4 canales con Python (librería `nidaqmx`) o con LabVIEW.
   El código de adquisición de la compu irá en la carpeta `pc/`.

---

## Estado y próximos pasos

- **Pendiente:** armado físico. Primero la fuente +-5 V (medir que dé +-5 V con el
  multímetro), después una cadena analógica de un canal (AD620 + filtros) y
  validarla con el NI, y recién ahí replicar x4.
- El script de adquisición en Python (carpeta `pc/`) se arma cuando haya hardware
  para probarlo contra el NI.

---

## Estructura de archivos

- `README.md` (este archivo): decisiones de diseño y panorama de la opción 1.
- Diseño de los filtros: `docs/ITBA-EEG-telemetria.pdf`.
- Datasheet del digitalizador: `docs/datasheets/NI-USB-6003-specs.pdf`.
- Diagrama de la fuente +-5 V: carpeta `hardware/` (si se guardó la imagen).
