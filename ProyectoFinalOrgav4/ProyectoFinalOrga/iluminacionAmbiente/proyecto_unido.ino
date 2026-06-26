#include <EEPROM.h>
#include <Servo.h> //libreria servomotor
#include "DFRobot_LCD.h"
#include <Wire.h>

// =========================================================================
//  GESTIÓN DE MEMORIA EEPROM
// =========================================================================
struct ModoInteligente {
  char nombre[15];       // Nombre exacto del modo
  byte estadoVentilador; // 0 = OFF, 1 = ON
  byte sala;             // 0 = OFF, 1 = ON
  byte comedor;          // 0 = OFF, 1 = ON
  byte cocina;           // 0 = OFF, 1 = ON
  byte bano;             // 0 = OFF, 1 = ON
  byte habitacion;       // 0 = OFF, 1 = ON
};

// Asignación de direcciones fijas en EEPROM (¡Corregido a 30 bytes de separación!)
const int DIR_FIESTA        = 0;
const int DIR_RELAJADO      = 30;
const int DIR_NOCHE         = 60;
const int DIR_ENCENDER_TODO = 90;
const int DIR_APAGAR_TODO   = 120;
const int DIR_CUSTOM_1      = 150;
const int DIR_CUSTOM_2      = 180;

// =========================================================================
// pantalla 
// =========================================================================
// Configuración de colores del fondo (RGB)
const int colorR = 255;
const int colorG = 0;
const int colorB = 0;

// Instancia de la pantalla LCD (16 columnas, 2 filas)
DFRobot_LCD lcd(16, 2);

// =========================================================================
// LEDs DE RETROALIMENTACIÓN (ESTADO DEL SISTEMA)
// =========================================================================
const int ledAzul = A0;   // Sistema activo y listo
const int ledVerde = A1;  // Éxito (Comando o Archivo .org)
const int ledRojo = A2;   // Error

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
const int pinBotonPuerta = 7; // pin fisico para la entrada
Servo servomotorPuerta;       
bool puertaAbierta = false;   
int ultimoEstadoBoton = HIGH;
unsigned long ultimoTiempoRebote = 0; // Guarda el tiempo de la última pulsación
int tiempoDebounce = 50;              // Tiempo de espera en milisegundos

unsigned long tiempoCambio = 0;
unsigned long tiempoFiesta = 0;
int estadoSistema = 0;
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

// =========================================================================
// void inicial
// =========================================================================
void setup() {
  servomotorPuerta.attach(PinServo, 1000, 2000);
  // Configuración de LEDs de estado
  pinMode(ledAzul, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledRojo, OUTPUT);

  // Estado inicial: Sistema listo y sin errores
  digitalWrite(ledAzul, HIGH); 
  digitalWrite(ledVerde, LOW);
  digitalWrite(ledRojo, LOW);

  Serial.begin(9600);
  inicializarPantalla(); 
  // Configuración de pines como salida
  pinMode(sala, OUTPUT);
  pinMode(comedor, OUTPUT);
  pinMode(cocina, OUTPUT);
  pinMode(bano, OUTPUT);
  pinMode(habitacion, OUTPUT);
  pinMode(ventilador, OUTPUT);
  
  pinMode(pinBotonPuerta, INPUT_PULLUP);
  servomotorPuerta.attach(PinServo); // El servomotor se conecta obligatoriamente en el pin 3

  // estado inicial
  apagarTodo();
  10; // 0 grados significa puerta cerrada
  puertaAbierta = false;

  // Cargamos y aseguramos los modos predefinidos en la EEPROM
  inicializarEEPROMFabrica();
  
  tiempoCambio = millis();
  Serial.println(">> Sistema SmartHome GT: EEPROM y Modos Integrados Correctamente.");
}

// =========================================================================
// void loop principal
// =========================================================================
void loop() {

  
  
  
  // busqueda constante de un archivo .org o comando manual
  leerArchivoOrg();

  unsigned long tiempoActual = millis();

  // Control manual local de la puerta mediante el botón físico

  if (tiempoActual - tiempoCambio >= 3000) {
      cambioEstadoPuerta();
    }

  // switch secuencial que extrae los datos directamente de EEPROM
  switch (estadoSistema) {
    case 0:
      modoFiesta(); // Modo dinámico especial con parpadeo
      if (tiempoActual - tiempoCambio >= 3000) {
        tiempoCambio = tiempoActual;
        mensajeFiesta = false;
        estadoSistema = 1;
      }
      break;

    case 1:
      aplicarModoDesdeEEPROM(DIR_RELAJADO);
      if (tiempoActual - tiempoCambio >= 3000) {
        tiempoCambio = tiempoActual;
        estadoSistema = 2;
      }
      break;

    case 2:
      aplicarModoDesdeEEPROM(DIR_NOCHE);
      if (tiempoActual - tiempoCambio >= 3000) {
        tiempoCambio = tiempoActual;
        estadoSistema = 3;
      }
      break;

    case 3:
      aplicarModoDesdeEEPROM(DIR_ENCENDER_TODO);
      if (tiempoActual - tiempoCambio >= 3000) {
        tiempoCambio = tiempoActual;
        estadoSistema = 4;
      }
      break;

    case 4:
      aplicarModoDesdeEEPROM(DIR_APAGAR_TODO);
      if (tiempoActual - tiempoCambio >= 3000) {
        tiempoCambio = tiempoActual;
        estadoSistema = 5;
      }
      break;

    case 5:
      aplicarModoDesdeEEPROM(DIR_CUSTOM_1); // Muestra el modo personalizado guardado
      if (tiempoActual - tiempoCambio >= 3000) {
        tiempoCambio = tiempoActual;
        estadoSistema = 6; // Pasa al segundo modo custom agregado
      }
      break;

    case 6:
      aplicarModoDesdeEEPROM(DIR_CUSTOM_2); // Muestra el segundo modo personalizado guardado
      if (tiempoActual - tiempoCambio >= 3000) {
        tiempoCambio = tiempoActual;
        estadoSistema = 0; // Reinicia el ciclo automático de demostración directo a modoFiesta
      }
      break;
  }
}

// =========================================================================
// modos de EEPROM
// =========================================================================
void inicializarEEPROMFabrica() {
  ModoInteligente fiesta       = {"Fiesta", 1, 1, 1, 1, 1, 1}; 
  ModoInteligente relajado     = {"Relajado", 0, 0, 0, 0, 0, 0};
  ModoInteligente noche        = {"Noche", 0, 0, 0, 0, 0, 0};
  ModoInteligente encenderTodo = {"Todo ON", 1, 1, 1, 1, 1, 1};
  ModoInteligente apagarTodo   = {"Todo OFF", 0, 0, 0, 0, 0, 0};

  EEPROM.put(DIR_FIESTA, fiesta);
  EEPROM.put(DIR_RELAJADO, relajado);
  EEPROM.put(DIR_NOCHE, noche);
  EEPROM.put(DIR_ENCENDER_TODO, encenderTodo);
  EEPROM.put(DIR_APAGAR_TODO, apagarTodo);

  // ==========================================
  // CREACIÓN DE MODO NULO PARA ESPACIOS VACÍOS
  // ==========================================
  ModoInteligente comprobador;
  ModoInteligente modoNulo = {"vacio", 0, 0, 0, 0, 0, 0}; // Todo apagado por defecto
  
  // Revisamos si el Custom 1 está vacío o corrupto (255)
  EEPROM.get(DIR_CUSTOM_1, comprobador);
  if (comprobador.nombre[0] == (char)255 || comprobador.nombre[0] == '\0') {
    EEPROM.put(DIR_CUSTOM_1, modoNulo); // Guardamos el modo nulo
  }

  // Revisamos si el Custom 2 está vacío o corrupto (255)
  EEPROM.get(DIR_CUSTOM_2, comprobador);
  if (comprobador.nombre[0] == (char)255 || comprobador.nombre[0] == '\0') {
    EEPROM.put(DIR_CUSTOM_2, modoNulo); // Guardamos el modo nulo
  }
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
  static int ultimaDireccion = -1; // Memoria anti-spam
  
  if (strcmp(modo.nombre, "ERROR") == 0) {
    // Verificamos si es un error nuevo para no hacer spam
    if (direccionBase != ultimaDireccion) {
      actualizarPantalla("ERROR", "OFF", "FALLO EEPROM");
      ultimaDireccion = direccionBase;
    }
    return; // Salimos de la función
  }
  
  // Cambiar los pines físicos
  digitalWrite(sala, modo.sala);
  digitalWrite(comedor, modo.comedor);
  digitalWrite(cocina, modo.cocina);
  digitalWrite(bano, modo.bano);
  digitalWrite(habitacion, modo.habitacion);
  digitalWrite(ventilador, modo.estadoVentilador);
  
  // Solo imprime si es un modo válido y diferente al anterior
  if (direccionBase != ultimaDireccion) {
    
    // --- NUEVA LÓGICA PARA EL TEXTO DE LOS LEDS ---
    String textoLeds = "";
    int totalLedsEncendidos = modo.sala + modo.comedor + modo.cocina + modo.bano + modo.habitacion;
    
    if (totalLedsEncendidos == 0) {
      textoLeds = "OFF";
    } else if (totalLedsEncendidos == 5) {
      textoLeds = "ON";
    } else {
      textoLeds = "CUSTOM";
    }
    // ----------------------------------------------

    // Actualizamos la pantalla con el nuevo texto dinámico
    actualizarPantalla(String(modo.nombre), modo.estadoVentilador == 1 ? "ON" : "OFF", textoLeds);
    ultimaDireccion = direccionBase; // Actualizamos el registro
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
  digitalWrite(ventilador, HIGH);
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

  // 1. Comprobamos si el botón cambió de estado (gracias a la pulsación o al ruido)
  if (lecturaBoton == LOW && ultimoEstadoBoton == HIGH) {
    
    // 2. Comprobamos si ha pasado suficiente tiempo desde el último cambio detectado
    if ((millis() - ultimoTiempoRebote) > tiempoDebounce) {
      
      puertaAbierta = !puertaAbierta;
      
      if (puertaAbierta) {
        servomotorPuerta.write(90);
      } else {
        servomotorPuerta.write(0);
      }
      
      // Actualizamos el tiempo del último cambio válido
      ultimoTiempoRebote = millis(); 
    }
  }
  
  ultimoEstadoBoton = lecturaBoton;
}
void cambioEstadoPuerta() {
    puertaAbierta = !puertaAbierta;
    if (puertaAbierta) {
      servomotorPuerta.write(90);
    } else {
      servomotorPuerta.write(0);
    }
}

// lee archivo .org compatible con EEPROM 
void leerArchivoOrg() {
  while (Serial.available() > 0) {
    String linea = Serial.readStringUntil('\n');
    linea.replace("\r", ""); 
    linea.trim();
    
    // Si la línea es un comando directo del puerto serial en lugar de una estructura .org
    if (linea == "modo_fiesta" || linea == "modo_relajado" || linea == "modo_noche" || 
        linea == "encender_todo" || linea == "apagar_todo" || linea == "modo_custom_1" || linea == "modo_custom_2") {
       if (linea == "modo_fiesta") estadoSistema = 0;
       if (linea == "modo_relajado") estadoSistema = 1;
       if (linea == "modo_noche") estadoSistema = 2;
       if (linea == "encender_todo") estadoSistema = 3;
       if (linea == "apagar_todo") estadoSistema = 4;
       if (linea == "modo_custom_1") estadoSistema = 5;
       if (linea == "modo_custom_2") estadoSistema = 6;
       tiempoCambio = millis(); // Resetea el tiempo para mantener congelado el modo seleccionado
       parpadeoExito();
       continue;
    }

    int posComentario = linea.indexOf("//");
    if (posComentario != -1) {
      linea = linea.substring(0, posComentario);
      linea.trim();
    }
    
    if (linea.length() == 0) continue;

    switch(estadoActual) {
      case ESPERANDO_INI:
        if (linea == "conf_ini") {
           estadoActual = LEYENDO_ARCHIVO;
           indiceCustom = 1; 
           restablecerSistemaListo();
        }else{
          mostrarError();
        }
        break;

      case LEYENDO_ARCHIVO:
        if (linea == "conf:fin") {
           estadoActual = ESPERANDO_INI;
           Serial.println("ÉXITO: Configuracion guardada en EEPROM");
           parpadeoExito();
        } 
        else if (linea.startsWith("modo_custom:")) {
           int primerComilla = linea.indexOf('"');
           int segundaComilla = linea.lastIndexOf('"');
           
           if(primerComilla != -1 && segundaComilla != -1 && segundaComilla > primerComilla) {
             tempNombre = linea.substring(primerComilla + 1, segundaComilla);
             estadoActual = LEYENDO_CUSTOM;
             lineasLeidasCustom = 0;
           } else {
             Serial.println("Error: Archivo.org");
             estadoActual = ESPERANDO_INI; 
             mostrarError();
           }
        }
        break;

      case LEYENDO_CUSTOM:
        if (linea.startsWith("Ventilador:")) {
           tempVentilador = (linea.indexOf("ON") != -1);
           lineasLeidasCustom++;
        } 
        else if (linea.startsWith("LED'S:")) {
           bool tSala = (linea.indexOf("sala:ON") != -1);
           bool tComedor = (linea.indexOf("comedor:ON") != -1);
           bool tCocina = (linea.indexOf("cocina:ON") != -1);
           bool tBano = (linea.indexOf("baño:ON") != -1 || linea.indexOf("bano:ON") != -1);
           bool tHab = (linea.indexOf("habitacion:ON") != -1);
           
           lineasLeidasCustom++;
           
           if (lineasLeidasCustom == 2) {
             // convertir string a array y guardar en EEPROM
             char nombreBuf[15];
             tempNombre.toCharArray(nombreBuf, 15);
             
             if (indiceCustom == 1) {
                guardarModoCustom(1, nombreBuf, tempVentilador ? 1 : 0, tSala ? 1 : 0, tComedor ? 1 : 0, tCocina ? 1 : 0, tBano ? 1 : 0, tHab ? 1 : 0);
                indiceCustom++;
             } else if (indiceCustom == 2) {
                guardarModoCustom(2, nombreBuf, tempVentilador ? 1 : 0, tSala ? 1 : 0, tComedor ? 1 : 0, tCocina ? 1 : 0, tBano ? 1 : 0, tHab ? 1 : 0);
             }
             estadoActual = LEYENDO_ARCHIVO; 
           }
        }
        break;
    }
  }
}
void inicializarPantalla() {
  lcd.init();
  lcd.setRGB(colorR, colorG, colorB);
  actualizarPantalla("APAGAR TODO", "OFF", "OFF");
}
void actualizarPantalla(String modo, String ventilador, String led) {
    // Limpia las dos líneas imprimiendo espacios en blanco
    lcd.setCursor(0, 0);
    lcd.print("                "); 
    lcd.setCursor(0, 1);
    lcd.print("                ");

    // Imprime el modo actual en la primera línea
    lcd.setCursor(0, 0);
    lcd.print("Modo: " + modo);
    
    // Imprime el estado del ventilador y los LEDs en la segunda línea
    lcd.setCursor(0, 1);
    lcd.print("V:" + ventilador + " LED:" + led);

    Serial.println("--------------------");
    Serial.println("Modo: " + modo);
    Serial.println("Ventilador: " + ventilador);
    Serial.println("LEDS: " + led);
}
void restablecerSistemaListo() {
  digitalWrite(ledAzul, HIGH); // Sistema en línea
  digitalWrite(ledRojo, LOW);  // Apaga el error si estaba encendido
}

void parpadeoExito() {
  restablecerSistemaListo(); // Limpiamos cualquier error anterior
  
  // Parpadea 3 veces rápidamente
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledVerde, HIGH);
    delay(150);
    digitalWrite(ledVerde, LOW);
    delay(150);
  }
}

void mostrarError() {
  digitalWrite(ledAzul, LOW);  // El sistema deja de estar "listo/normal"
  digitalWrite(ledRojo, HIGH); // Enciende alerta permanente
}

