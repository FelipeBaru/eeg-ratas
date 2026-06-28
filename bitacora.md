# Bitácora — EEG intracraneal 4 canales (ratas, detección de crisis)

Proyecto de Paz. Registro de qué hicimos, por qué, y en qué punto estamos.
Se actualiza a medida que avanzamos.

## El proyecto en una frase
Construir un electroencefalógrafo intracraneal de 4 canales para ratas, con el
objetivo final de detectar crisis epilépticas. Estamos en la etapa de primer
demo robusto (de banco, con cable), antes de la versión inalámbrica.

## Decisión de arquitectura (la más importante)
Hay dos formas de meter 4 canales al digitalizador:
- MUX-primero (la del PDF de ITBA): un solo circuito analógico + un multiplexor
  CD4051B que rota entre electrodos. Más barato, pero conmuta señales de
  microvoltios ANTES de amplificarlas (mete ruido) y no toma los 4 canales a la vez.
- 4 en paralelo (la que elegimos): un circuito analógico completo por canal
  (amplificador + filtros), y los 4 entran directo al digitalizador. Más
  componentes, pero señal mucho más limpia y 4 canales simultáneos.

Dato que cerró la decisión: el digitalizador que tenemos (NI USB-6003) YA trae
adentro su propio multiplexor + amplificador + ADC (16 bits, 100 kS/s, 4 entradas
diferenciales). Ponerle un CD4051B externo adelante, para 4 canales, sería
redundante. Por eso en la v1 el multiplexor NO se usa.

## Los dos caminos (en orden)
- v1 robusta (AHORA, sin wifi): electrodos -> 4x(AD620 + filtros Bessel 1-40 Hz)
  -> NI USB-6003 (4 entradas diff) -> PC (Python con nidaqmx). Es la mejor en
  calidad. El NI box va por USB, ideal para un demo de banco.
- v2 inalámbrica (DESPUÉS): se reemplaza el NI box por ESP8266 + ADS1115 y se
  transmite por wifi. Recién ahí el CD4051B sirve, y solo si se pasa de 4 canales.

## Banda de interés
Delta a Beta (~1-40 Hz). Gamma queda afuera (>30 Hz, <10 uV). Como cortamos en
40 Hz, el pasabajos ya atenúa el ruido de red de 50 Hz -> no hace falta filtro
notch. Nota a futuro: para detectar crisis, las espigas epileptiformes tienen
energía por encima de 40 Hz; quizás haya que ensanchar la banda en esa etapa.

## Alimentación
Con baterías (no de pared) para bajar ruido de línea. El front-end necesita
fuente partida ±5 V (2 pilas de 9 V en serie + reguladores 7805/7905). Punto
clave: durante la medición, la laptop también a batería (desenchufada), para no
reintroducir masa de red por el USB del NI box. Una sola masa (topología estrella).

---

## Registro cronológico

### 2026-06-28 — Gate 1: armado del proyecto y entorno
Qué hicimos:
- Repo `eeg-ratas` con estructura (firmware/ pc/ docs/ hardware/) bajo git.
- CLAUDE.md con las reglas del proyecto.
- Papers (ILAE/Moyer, ITBA) y datasheets (NI, CD4051B, ADS1115) en docs/.
- PlatformIO instalado; sketch de prueba del MUX compilado OK.
- .gitignore para no versionar artefactos de compilación.
Por qué: dejar una base ordenada y reproducible, que Paz pueda clonar y tener
todo (código + documentación + datasheets) en un solo lugar.
Pendiente: probar el MUX en hardware (opcional, no es parte de la v1); bajar el
datasheet del AD620.

### Estado actual
- Gate 1 cerrado (falta solo la prueba física opcional del MUX).
- Gate 2 sin empezar: es la v1 real -> fuente ±5 V + primera cadena analógica de
  1 canal medida con el NI box.

### 2026-06-28 — Opción 2: diseño y firmware inalámbrico (v0)
Qué hicimos:
- Documentamos la arquitectura de la opción 2 (inalámbrica) en docs/opcion-2/:
  README con las decisiones de diseño y firmware-explicado con el código comentado.
- Cadena: electrodo -> CD4051B (rota canal) -> AD620 -> filtros Bessel 1-40 Hz
  -> sumador (+1.65V offset) -> ADS1115 (A0) -> ESP8266 -> WiFi -> compu.
- Firmware v0 del ESP (firmware/src/eeg_esp8266.cpp): crea su propia red WiFi
  (AP "EEG_Paz", IP 192.168.4.1), rota los 4 canales leyendo el ADS1115, y sirve
  una pagina web con grafico en vivo. Sin instalar nada en la compu.
- platformio.ini ahora maneja dos entornos: uno (test del MUX) y nodemcuv2 (ESP).
Por que (decisiones clave):
- Un solo front-end + MUX que rota: menos componentes; contra: canales no
  simultaneos y velocidad limitada por el ADS1115 (~150 muestras/seg por canal).
- Offset de 1.65V: el ADS1115 single-ended solo mide 0-3.3V; la senal va en +-,
  se centra con offset y se resta en software.
- ADS1115 y logica del MUX a 3.3V (el ESP no tolera 5V); VEE del MUX a -5V para no
  recortar la parte negativa; ESP por power bank (aislacion, sin masa de red).
Pendiente: armar el hardware y subir el firmware (desde la PC de Paz, por el
adaptador USB que falta en el Mac). Mejora futura (Gate B): streaming por bloques
para reconstruir la onda fina de cada canal.

### Estado actual
- Opcion 1 (NI): pendiente de armado fisico (fuente +-5V + cadena analogica).
- Opcion 2 (inalambrica): diseno y firmware v0 documentados y en el repo;
  pendiente de armado y carga al ESP.
