#include <EEPROM.h>
#include <Servo.h> //libreria servomotor
#include <Wire.h>
#include <LiquidCrystal_I2C.h> 
#include <SoftwareSerial.h> 

// =========================================================================
// CONFIGURACIÓN DE SOFTWARE SERIAL PARA BLUETOOTH
// =========================================================================
SoftwareSerial miBluetooth(10, 11); 

const int EEPROM_FIRMA = 255;

// =========================================================================
//  GESTIÓN DE MEMORIA EEPROM
// =========================================================================
struct ModoInteligente {
  char nombre[15];
  byte estadoVentilador;
  byte sala;
  byte comedor;
  byte cocina;
  byte bano;
  byte habitacion;
  byte alternandose;
};

const int DIR_FIESTA        = 0;
const int DIR_RELAJADO      = 30;
const int DIR_NOCHE         = 60;
const int DIR_ENCENDER_TODO = 90;
const int DIR_APAGAR_TODO   = 120;
const int DIR_CUSTOM_1      = 150;
const int DIR_CUSTOM_2      = 180;

// =========================================================================
// pantalla  (CONFIGURACIÓN I2C MODIFICADA)
// =========================================================================
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// =========================================================================
// LEDs DE RETROALIMENTACIÓN (ESTADO DEL SISTEMA)
// =========================================================================
const int ledAzul = A0;   
const int ledVerde = A1;  
const int ledRojo = A2;   

// =========================================================================
// asignacion de luces y tiempo
// =========================================================================
const int sala = 2;
const int comedor = 4; 
const int cocina = 3; 
const int bano = 5;
const int habitacion = 6;
const int ventilador = 9;
const int PinServo = 8;

// control de puerta 
const int pinBotonPuerta = 7; 
Servo servomotorPuerta;       
bool puertaAbierta = false;   
int ultimoEstadoBoton = HIGH;
unsigned long ultimoTiempoRebote = 0; 
int tiempoDebounce = 50;
const int posicionCerrado = 0;
const int posicionAbierto = 90;            

unsigned long tiempoCambio = 0;
unsigned long tiempoFiesta = 0;
int estadoSistema = -1;
bool alternanciaFiesta = false;
bool mensajeFiesta = false;

// =========================================================================
// variables del .org
// =========================================================================
enum EstadoParser { ESPERANDO_INI, LEYENDO_ARCHIVO, LEYENDO_CUSTOM };
EstadoParser estadoActual = ESPERANDO_INI;

int indiceCustom = 1; 
String tempNombre = "";
bool tempVentilador = false;
int lineasLeidasCustom = 0;
String modoPredefinidoActual = "";
String tempLineaLeds = "";
bool tempAlternandose = false;

// =========================================================================
// FUNCIONES AUXILIARES PARA ENVIAR A AMBOS CANALES (PC Y TELEFONO)
// =========================================================================
void imprimirConsola(const __FlashStringHelper* texto) {
  Serial.println(texto);       // Envía a la PC desde la Flash
  miBluetooth.println(texto);  // Envía al Teléfono desde la Flash
}

void imprimirConsolaSinSalto(const __FlashStringHelper* texto) {
  Serial.print(texto);
  miBluetooth.print(texto);
}

// 2. Versiones para variables y textos combinados (String) [Soportan datos dinámicos]
void imprimirConsola(String texto) {
  Serial.println(texto);
  miBluetooth.println(texto);
}

void imprimirConsolaSinSalto(String texto) {
  Serial.print(texto);
  miBluetooth.print(texto);
}

// =========================================================================
// void inicial
// =========================================================================
void setup() {
  
  pinMode(ledAzul, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledRojo, OUTPUT);

  digitalWrite(ledAzul, HIGH); 
  digitalWrite(ledVerde, LOW);
  digitalWrite(ledRojo, LOW);

  Serial.begin(9600);        // Inicializa canal de la PC
  miBluetooth.begin(9600);   // Inicializa canal del Teléfono
  
  inicializarPantalla(); 
  
  pinMode(sala, OUTPUT);
  pinMode(comedor, OUTPUT);
  pinMode(cocina, OUTPUT);
  pinMode(bano, OUTPUT);
  pinMode(habitacion, OUTPUT);
  pinMode(ventilador, OUTPUT);
  
  pinMode(pinBotonPuerta, INPUT_PULLUP);
  servomotorPuerta.attach(PinServo); 

  apagarTodo();
  servomotorPuerta.write(0); 
  puertaAbierta = false;

  inicializarEEPROMFabrica();
  
  tiempoCambio = millis();
  imprimirConsola(F(">> Sistema SmartHome GT: EEPROM y Modos Integrados Correctamente."));
}

void loop()
{
    leerBluetooth();
    leerArchivoORG();

    unsigned long tiempoActual = millis();
    revisarBotonPuerta();

    switch (estadoSistema)
    {
        case 0:
            modoFiesta();
            break;
        case 1:
            mensajeFiesta = false;
            aplicarModoDesdeEEPROM(DIR_RELAJADO);
            break;
        case 2:
            mensajeFiesta = false;
            aplicarModoDesdeEEPROM(DIR_NOCHE);
            break;
        case 3:
            mensajeFiesta = false;
            aplicarModoDesdeEEPROM(DIR_ENCENDER_TODO);
            break;
        case 4:
            mensajeFiesta = false;
            aplicarModoDesdeEEPROM(DIR_APAGAR_TODO);
            break;
        case 5:
            mensajeFiesta = false;
            aplicarModoDesdeEEPROM(DIR_CUSTOM_1);
            break;
        case 6:
            mensajeFiesta = false;
            aplicarModoDesdeEEPROM(DIR_CUSTOM_2);
            break;
    }
}

// =========================================================================
// modos de EEPROM
// =========================================================================
void inicializarEEPROMFabrica()
{
    if(EEPROM.read(EEPROM_FIRMA) == 123)
    {
        imprimirConsola(F("EEPROM ya configurada"));
        return;
    }

    ModoInteligente fiesta       = {"Fiesta", 1, 1, 1, 1, 1, 1, 1};
    ModoInteligente relajado     = {"Relajado", 0, 0, 0, 0, 0, 0, 0};
    ModoInteligente noche        = {"Noche", 0, 0, 0, 0, 0, 0, 0};
    ModoInteligente encenderTodo = {"Todo ON", 1, 1, 1, 1, 1, 1, 0};
    ModoInteligente apagarTodo   = {"Todo OFF", 0, 0, 0, 0, 0, 0, 0};

    EEPROM.put(DIR_FIESTA, fiesta);
    EEPROM.put(DIR_RELAJADO, relajado);
    EEPROM.put(DIR_NOCHE, noche);
    EEPROM.put(DIR_ENCENDER_TODO, encenderTodo);
    EEPROM.put(DIR_APAGAR_TODO, apagarTodo);

    EEPROM.write(EEPROM_FIRMA, 123);

    imprimirConsola(F("EEPROM inicializada"));
}

void guardarModoCustom(int numeroCustom, char* nombreModo, byte vent, byte s, byte com, byte coc, byte b, byte hab) {
  ModoInteligente nuevoCustom;
  
  strncpy(nuevoCustom.nombre, nombreModo, 14); 
  nuevoCustom.nombre[14] = '\0'; 
  nuevoCustom.estadoVentilador = vent;
  nuevoCustom.sala = s;
  nuevoCustom.comedor = com;
  nuevoCustom.cocina = coc;
  nuevoCustom.bano = b;
  nuevoCustom.habitacion = hab;

  int direccionDestino = (numeroCustom == 1) ? DIR_CUSTOM_1 : DIR_CUSTOM_2;
  EEPROM.put(direccionDestino, nuevoCustom);
}

void guardarModoPredefinido( int direccion, char* nombreModo, byte vent, byte s, byte com, byte coc, byte b, byte hab, byte alterna)
{
  ModoInteligente modo;

  strncpy(modo.nombre, nombreModo, 14);
  modo.nombre[14] = '\0';

  modo.estadoVentilador = vent;
  modo.sala = s;
  modo.comedor = com;
  modo.cocina = coc;
  modo.bano = b;
  modo.habitacion = hab;
  modo.alternandose = alterna;

  EEPROM.put(direccion, modo);
}

ModoInteligente obtenerModoDeEEPROM(int direccionBase) {
  ModoInteligente modoRecuperado;
  EEPROM.get(direccionBase, modoRecuperado);
  
  if (modoRecuperado.nombre[0] == (char)255 || modoRecuperado.nombre[0] == '\0') {
    strcpy(modoRecuperado.nombre, "ERROR");
  }
  return modoRecuperado;
}

void aplicarModoDesdeEEPROM(int direccionBase) {
  ModoInteligente modo = obtenerModoDeEEPROM(direccionBase);
  static int ultimaDireccion = -1; 
  
  if (strcmp(modo.nombre, "ERROR") == 0) {
    if (direccionBase != ultimaDireccion) {
      actualizarPantalla("ERROR", "OFF", "FALLO EEPROM");
      ultimaDireccion = direccionBase;
    }
    return; 
  }
  
  digitalWrite(sala, modo.sala);
  digitalWrite(comedor, modo.comedor);
  digitalWrite(cocina, modo.cocina);
  digitalWrite(bano, modo.bano);
  digitalWrite(habitacion, modo.habitacion);
  digitalWrite(ventilador, modo.estadoVentilador);
  
  if (direccionBase != ultimaDireccion) {
    String textoLeds = "";
    int totalLedsEncendidos = modo.sala + modo.comedor + modo.cocina + modo.bano + modo.habitacion;
    
    if (totalLedsEncendidos == 0) {
      textoLeds = "OFF";
    } else if (totalLedsEncendidos == 5) {
      textoLeds = "ON";
    } else {
      textoLeds = "CUSTOM";
    }

    actualizarPantalla(String(modo.nombre), modo.estadoVentilador == 1 ? "ON" : "OFF", textoLeds);
    ultimaDireccion = direccionBase; 
  }
}

void apagarTodo() {
  digitalWrite(sala, LOW);
  digitalWrite(comedor, LOW);
  digitalWrite(cocina, LOW);
  digitalWrite(bano, LOW);
  digitalWrite(habitacion, LOW);
  digitalWrite(ventilador, LOW);
}

void modoFiesta() {

  ModoInteligente fiesta = obtenerModoDeEEPROM(DIR_FIESTA);

    digitalWrite(ventilador, fiesta.estadoVentilador);

    if(fiesta.alternandose == 0)
    {
        aplicarModoDesdeEEPROM(DIR_FIESTA);
        return;
    }

    if (!mensajeFiesta) {
      actualizarPantalla("fiesta", "ON", "ALTERNANDO");
      mensajeFiesta = true;
    }

  unsigned long tiempoActual = millis();
  if (tiempoActual - tiempoFiesta >= 100) {
    tiempoFiesta = tiempoActual;
    alternanciaFiesta = !alternanciaFiesta;
    
    if (alternanciaFiesta) {
      digitalWrite(sala, HIGH);
      digitalWrite(cocina, HIGH);
      digitalWrite(habitacion, HIGH);
      digitalWrite(comedor, LOW);
      digitalWrite(bano, LOW);
    } else {
      digitalWrite(sala, LOW);
      digitalWrite(cocina, LOW);
      digitalWrite(habitacion, LOW);
      digitalWrite(comedor, HIGH);
      digitalWrite(bano, HIGH);
    }
  }
}

void revisarBotonPuerta() {
  int lecturaBoton = digitalRead(pinBotonPuerta);

  if (lecturaBoton == LOW && ultimoEstadoBoton == HIGH) {
    if ((millis() - ultimoTiempoRebote) > tiempoDebounce) {
      
      puertaAbierta = !puertaAbierta;
      imprimirConsola(F("Boton presionado: Activando Servo..."));
      
      servomotorPuerta.attach(PinServo); 
      if (puertaAbierta) {
        servomotorPuerta.write(posicionAbierto); // <-- Usa la variable de apertura
      } else {
        servomotorPuerta.write(posicionCerrado); // <-- Usa la variable de cierre
      }
      
      delay(150); 
      servomotorPuerta.detach(); 

      ultimoTiempoRebote = millis(); 
    }
  }
  ultimoEstadoBoton = lecturaBoton;
}

void cambioEstadoPuerta() {
    servomotorPuerta.attach(PinServo); // 1. Activamos el pin del servo
    
    puertaAbierta = !puertaAbierta;
    if (puertaAbierta) {
      servomotorPuerta.write(90);
    } else {
      servomotorPuerta.write(0);
    }
    
    delay(300);                        // 2. Esperamos a que el motor termine de girar
    servomotorPuerta.detach();         // 3. ¡Lo apagamos para quitar el ruido!
}

void leerBluetooth()
{
  while (miBluetooth.available() > 0)
  {
      String linea =
          miBluetooth.readStringUntil('\n');

      linea.replace("\r", "");
      linea.trim();

      if (linea == "modo_fiesta")
      {
          estadoSistema = 0;
          parpadeoExito();
      }

      else if (linea == "modo_relajado")
      {
          estadoSistema = 1;
          parpadeoExito();
      }

      else if (linea == "modo_noche")
      {
          estadoSistema = 2;
          parpadeoExito();
      }

      else if (linea == "encender_todo")
      {
          estadoSistema = 3;
          parpadeoExito();
      }

      else if (linea == "apagar_todo")
      {
          estadoSistema = 4;
          parpadeoExito();
      }

      else
      {
          ModoInteligente c1 =
              obtenerModoDeEEPROM(DIR_CUSTOM_1);

          ModoInteligente c2 =
              obtenerModoDeEEPROM(DIR_CUSTOM_2);

          if(linea == String(c1.nombre))
          {
              estadoSistema = 5;
              parpadeoExito();
          }

          else if(linea == String(c2.nombre))
          {
              estadoSistema = 6;
              parpadeoExito();
          }
      }
  }
}

void leerArchivoORG() {
  while (Serial.available() > 0) {
    String linea = Serial.readStringUntil('\n');
    linea.replace("\r", ""); 
    linea.trim();

    // Notifica a ambos lados lo que acaba de entrar
    imprimirConsolaSinSalto(F("Recibido por USB: "));
    imprimirConsola(linea);

    int posComentario = linea.indexOf("//");
    if (posComentario != -1) {
      linea = linea.substring(0, posComentario);
      linea.trim();
    }
    
    if (linea.length() == 0) continue;

    switch(estadoActual) {
      case ESPERANDO_INI:

      if (linea == "conf_ini")
      {
          estadoActual = LEYENDO_ARCHIVO;

          indiceCustom = 1;

          modoPredefinidoActual = "";
          tempNombre = "";
          tempLineaLeds = "";
          tempVentilador = false;
          tempAlternandose = false;
          lineasLeidasCustom = 0;

          restablecerSistemaListo();

          imprimirConsola(F("Inicio de carga ORG"));
      }

      break;

      case LEYENDO_ARCHIVO:

      if (linea == "conf:fin")
      {
          estadoActual = ESPERANDO_INI;

          imprimirConsola(F("Configuracion guardada"));

          parpadeoExito();

          actualizarPantalla("CONFIG", "OK", "GUARDADA");
      }

      else if (
          linea == "modo_fiesta" ||
          linea == "modo_relajado" ||
          linea == "modo_noche" ||
          linea == "encender_todo" ||
          linea == "apagar_todo")
      {
          modoPredefinidoActual = linea;

          tempVentilador = false;
          tempLineaLeds = "";
          lineasLeidasCustom = 0;
          tempAlternandose = false;

          estadoActual = LEYENDO_CUSTOM;
      }

      else if (linea.startsWith("modo_custom:"))
      {
          int primerComilla = linea.indexOf('"');
          int segundaComilla = linea.lastIndexOf('"');

          if(primerComilla != -1 &&
            segundaComilla != -1 &&
            segundaComilla > primerComilla)
          {
              tempNombre =
                  linea.substring(primerComilla + 1,
                                  segundaComilla);

              tempVentilador = false;
              tempLineaLeds = "";
              lineasLeidasCustom = 0;
              tempAlternandose = false;

              estadoActual = LEYENDO_CUSTOM;
          }
          else
          {
              mostrarError();
              estadoActual = ESPERANDO_INI;
          }
      }

      break;

      case LEYENDO_CUSTOM:

      if (linea.startsWith("Ventilador:"))
      {
          tempVentilador =
              (linea.indexOf("ON") != -1);

          lineasLeidasCustom++;
      }

      else if (linea.startsWith("LED'S:"))
      {
          tempLineaLeds = linea;

          tempAlternandose = (linea.indexOf("Alternandose") != -1);

          lineasLeidasCustom++;
      }

      if(lineasLeidasCustom >= 2)
      {
          bool tSala =
              (tempLineaLeds.indexOf("sala:ON") != -1);

          bool tComedor =
              (tempLineaLeds.indexOf("comedor:ON") != -1);

          bool tCocina =
              (tempLineaLeds.indexOf("cocina:ON") != -1);

          bool tBano =
              (tempLineaLeds.indexOf("baño:ON") != -1 ||
              tempLineaLeds.indexOf("bano:ON") != -1);

          bool tHab =
              (tempLineaLeds.indexOf("habitacion:ON") != -1);

          if(modoPredefinidoActual != "")
          {
              int dir = DIR_FIESTA;

              if(modoPredefinidoActual == "modo_fiesta")
                  dir = DIR_FIESTA;

              else if(modoPredefinidoActual == "modo_relajado")
                  dir = DIR_RELAJADO;

              else if(modoPredefinidoActual == "modo_noche")
                  dir = DIR_NOCHE;

              else if(modoPredefinidoActual == "encender_todo")
                  dir = DIR_ENCENDER_TODO;

              else if(modoPredefinidoActual == "apagar_todo")
                  dir = DIR_APAGAR_TODO;

              char nombreBuf[15];

              modoPredefinidoActual.toCharArray(
                  nombreBuf,
                  15);

              guardarModoPredefinido(
              dir,
              nombreBuf,
              tempVentilador,
              tSala,
              tComedor,
              tCocina,
              tBano,
              tHab,
              tempAlternandose);

              imprimirConsola(
                  "Guardado: " +
                  modoPredefinidoActual);

              modoPredefinidoActual = "";
          }

          else
          {
              char nombreBuf[15];

              tempNombre.toCharArray(
                  nombreBuf,
                  15);

              if(indiceCustom == 1)
              {
                  guardarModoCustom(
                      1,
                      nombreBuf,
                      tempVentilador,
                      tSala,
                      tComedor,
                      tCocina,
                      tBano,
                      tHab);

                  indiceCustom++;
              }
              else
              {
                  guardarModoCustom(
                      2,
                      nombreBuf,
                      tempVentilador,
                      tSala,
                      tComedor,
                      tCocina,
                      tBano,
                      tHab);
              }

              imprimirConsola(
                  "Guardado Custom: " +
                  tempNombre);
          }

          tempNombre = "";
          tempLineaLeds = "";
          tempVentilador = false;
          lineasLeidasCustom = 0;
          tempAlternandose = false;

          estadoActual = LEYENDO_ARCHIVO;
      }

      break;
    }
  }
}

// =========================================================================
// PANTALLA LCD (Módulo I2C Integrado)
// =========================================================================
void inicializarPantalla() {
  imprimirConsola(F("\n--- ESCANEO Y VERIFICACIÓN DE PUERTOS I2C ---"));
  Wire.begin(); 
  
  byte error, direccion;
  bool dispositivoEncontrado = false;

  imprimirConsola(F("Escaneando bus I2C..."));

  for(direccion = 1; direccion < 127; direccion++ ) {
    Wire.beginTransmission(direccion);
    error = Wire.endTransmission();

    if (error == 0) {
      imprimirConsolaSinSalto(F("-> ¡DISPOSITIVO ENCONTRADO en la direccion: 0x"));
      if (direccion < 16) imprimirConsolaSinSalto(F("0"));
      // Conversión manual para imprimir el HEX con la función combinada
      String dirHex = String(direccion, HEX);
      dirHex.toUpperCase();
      imprimirConsola(dirHex);
      dispositivoEncontrado = true;
    }
    else if (error == 4) {
      imprimirConsolaSinSalto(F("-> Error desconocido en la direccion 0x"));
      if (direccion < 16) imprimirConsolaSinSalto(F("0"));
      String dirHex = String(direccion, HEX);
      dirHex.toUpperCase();
      imprimirConsola(dirHex);
    }    
  }

  imprimirConsola(F("Escaneo finalizado."));
  imprimirConsola(F("---------------------------------------------"));

  if (dispositivoEncontrado) {
    imprimirConsola(F("Estado del LCD: Inicializando..."));
    lcd.init();          
    lcd.backlight();     
    imprimirConsola(F("Estado del LCD: Mensaje enviado con exito."));
    
    actualizarPantalla("APAGAR TODO", "OFF", "OFF"); 
  } else {
    imprimirConsola(F("ALERTA: No se detecto ningun dispositivo I2C."));
    imprimirConsola(F("Por favor, revisa los cables: VCC, GND, SDA y SCL."));
  }
  imprimirConsola(F("---------------------------------------------"));
}

void actualizarPantalla(String modo, String ventilador, String led) {
    // Creamos las dos líneas de texto exactas combinando las variables
    String linea1 = "Modo: " + modo;
    String linea2 = "V:" + ventilador + " LED:" + led;

    // Rellenamos con espacios al final para que siempre tengan 16 caracteres
    // Esto borra lo que hubiera antes de forma invisible y sin parpadeos
    while (linea1.length() < 16) linea1 += " ";
    while (linea2.length() < 16) linea2 += " ";

    // Imprimimos directo encima de lo anterior de un solo golpe
    lcd.setCursor(0, 0);
    lcd.print(linea1);
    
    lcd.setCursor(0, 1);
    lcd.print(linea2);

    // Mantenemos tu telemetría serial en orden
    imprimirConsola(F("--------------------"));
    imprimirConsola("Modo: " + modo);
    imprimirConsola("Ventilador: " + ventilador);
    imprimirConsola("LEDS: " + led);
}

void restablecerSistemaListo() {
  digitalWrite(ledAzul, HIGH); 
  digitalWrite(ledRojo, LOW);  
}

void parpadeoExito() {
  restablecerSistemaListo(); 
  
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledVerde, HIGH);
    delay(150);
    digitalWrite(ledVerde, LOW);
    delay(150);
  }
}

void mostrarError() {
  digitalWrite(ledAzul, LOW);  
  digitalWrite(ledRojo, HIGH); 
}