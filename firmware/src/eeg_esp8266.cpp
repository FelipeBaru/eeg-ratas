#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// ---------- Pines del CD4051B (selección de canal) ----------
#define PIN_A D5   // GPIO14 -> CD4051B pin 11 (A, bit0)
#define PIN_B D6   // GPIO12 -> CD4051B pin 10 (B, bit1)
#define PIN_C D7   // GPIO13 -> CD4051B pin 9  (C, bit2)
// INH (pin 6) -> GND ; VEE (pin 7) -> -5V ; VDD (pin 16) -> 3.3V

// ---------- ADS1115 por I2C (SDA=D2/GPIO4, SCL=D1/GPIO5) ----------
Adafruit_ADS1115 ads;

// ---------- WiFi en modo Access Point (su propia red e IP) ----------
const char* AP_SSID = "EEG_Paz";
const char* AP_PASS = "eeg12345";   // mínimo 8 caracteres
ESP8266WebServer server(80);

const int NUM_CANALES = 4;
volatile float voltios[NUM_CANALES] = {0, 0, 0, 0};

void seleccionarCanal(int ch) {
  digitalWrite(PIN_A, bitRead(ch, 0));
  digitalWrite(PIN_B, bitRead(ch, 1));
  digitalWrite(PIN_C, bitRead(ch, 2));
}

// Página web embebida: gráfico en vivo, sin librerías externas (funciona sin internet)
const char PAGINA[] PROGMEM = R"HTML(
<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>EEG 4 canales</title>
<style>
body{font-family:sans-serif;margin:0;background:#111;color:#eee}
h1{font-size:16px;padding:8px 12px;margin:0}
canvas{width:100%;height:auto;display:block;background:#000}
.v{display:flex;gap:12px;padding:8px 12px;flex-wrap:wrap;font-size:13px}
</style></head><body>
<h1>EEG 4 canales - en vivo</h1>
<canvas id="g" width="800" height="400"></canvas>
<div class="v" id="vals"></div>
<script>
const COL=["#4da3ff","#ff6b6b","#3ddc97","#f7c948"];
const N=4, BUF=400;
let datos=[[],[],[],[]];
const c=document.getElementById("g"), ctx=c.getContext("2d");
const vals=document.getElementById("vals");
async function tick(){
  try{
    const j=await (await fetch("/data")).json();
    let html="";
    for(let i=0;i<N;i++){
      datos[i].push(j[i]);
      if(datos[i].length>BUF) datos[i].shift();
      html+=`<span style="color:${COL[i]}">Canal ${i}: ${j[i].toFixed(4)} V</span>`;
    }
    vals.innerHTML=html; dibujar();
  }catch(e){}
  setTimeout(tick,50);
}
function dibujar(){
  ctx.clearRect(0,0,c.width,c.height);
  const h=c.height/N;
  for(let i=0;i<N;i++){
    const base=h*i+h/2;
    ctx.strokeStyle="#333"; ctx.beginPath();
    ctx.moveTo(0,base); ctx.lineTo(c.width,base); ctx.stroke();
    ctx.strokeStyle=COL[i]; ctx.beginPath();
    for(let k=0;k<datos[i].length;k++){
      const x=k*c.width/BUF;
      const y=base-(datos[i][k]-1.65)*h*0.8; // resta el offset de 1.65V
      k?ctx.lineTo(x,y):ctx.moveTo(x,y);
    }
    ctx.stroke();
  }
}
tick();
</script></body></html>
)HTML";

void handleRoot(){ server.send_P(200, "text/html", PAGINA); }

void handleData(){
  String j = "[";
  for (int i=0;i<NUM_CANALES;i++){ j += String(voltios[i],4); if(i<NUM_CANALES-1) j += ","; }
  j += "]";
  server.send(200, "application/json", j);
}

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

void loop(){
  for(int ch=0; ch<NUM_CANALES; ch++){
    seleccionarCanal(ch);
    delayMicroseconds(200);                 // asentamiento del MUX
    int16_t raw = ads.readADC_SingleEnded(0);
    voltios[ch] = ads.computeVolts(raw);
  }
  server.handleClient();
}
