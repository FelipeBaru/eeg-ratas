# EEG ratas 4 canales — detección de crisis epilépticas

Proyecto de Paz. Se desarrolla en Mac y se comparte vía GitHub (Paz lo clona).

## Estado
- Fase: diseño + primer demo robusto. Arquitectura v1: 4 cadenas analógicas en
  paralelo (AD620 + filtros Bessel 1-40 Hz) -> NI USB-6003 (4 entradas diff) -> PC.
- El CD4051B y el ESP8266 quedan para la v2 inalámbrica (>4 canales / wifi).

## Reglas
- Responder en español rioplatense, directo, sin relleno.
- Toda afirmación técnica sobre un componente se ancla al datasheet en
  docs/datasheets/ citando archivo y página.
- Progresión por gates: no avanzar al siguiente sin cerrar el anterior.

## Documentos
- docs/ILAE-Moyer-2017.pdf — estándar EEG en animales.
- docs/ITBA-EEG-telemetria.pdf — molde de diseño (bloques, esquemático, filtros).
- docs/datasheets/ — NI USB-6003, CD4051B, ADS1115, AD620, ESP8266.

## Estructura
- firmware/  -> código del micro (PlatformIO, Arduino/ESP8266)
- pc/        -> adquisición + análisis en Python (nidaqmx)
- hardware/  -> esquemáticos, BOM, notas de diseño
- docs/      -> papers + datasheets
