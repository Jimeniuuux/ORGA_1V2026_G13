# Universidad de San Carlos de Guatemala
#### Facultad de Ingeniería
#### Escuela de Ciencias y Sistemas
#### Curso: Organización Computacional  
#### Catedrático: Ing. Carlos Lozano  
#### Auxiliar: Carlos José Blanco Guzmán  

---

## PROYECTO - SMARTHOME GT: SISTEMA DE CONTROL INTELIGENTE

### Integrantes
| Integrantes | Carné |
| :--- | :--- |
| Daniela Odeth Velásquez Solís | 202503522 |
| Jose Carlos González López | 202505069 |
| Angel Daniel Ruiz Ramos | 202505232 |
| Melanie Jimena Pichiyá Choc | 202502288 |
| Yosselin Aracely Oxlaj González | 202503415 |
| Pablo Javier Alvarez Marroquin | 202300502 |

---

## INTRODUCCIÓN
Este proyecto aborda el diseño e implementación de una maqueta funcional de una casa inteligente controlada mediante Arduino, la cual integra la gestión automatizada de iluminación por ambientes, un sistema de ventilación y el control de una puerta. También permite la personalización y persistencia de datos mediante el desarrollo de un intérprete de comandos seriales, el cual es capaz de procesar un archivo de configuración externo con extensión .org Esto permite al usuario programar escenas lumínicas a medida y almacenar dicha información de forma estructurada en la memoria no volátil (EEPROM) del microcontrolador, garantizando que los perfiles personalizados no se pierdan ante eventuales cortes de energía[cite: 625].

Asimismo, el prototipo combina la interacción física y la conectividad inalámbrica. La supervisión en tiempo real se realiza mediante una pantalla LCD I2C y diodos LED de estado, mientras que el control operativo se divide entre un botón local para el accionamiento de la puerta mediante un servomotor y un módulo Bluetooth (HC-06). Este último canaliza las instrucciones desde un dispositivo móvil para activar los diferentes modos ambientales (tanto predefinidos como personalizados), demostrando de manera práctica la convergencia entre la organización computacional, la programación de sistemas embebidos y el control de actuadores en el mundo real[cite: 628].

---

## OBJETIVOS

* Diseñar y construir un prototipo funcional de domótica residencial basado en microcontroladores que permita la gestión centralizada de accesos, ventilación e iluminación personalizada, integrando el almacenamiento persistente en memoria EEPROM y la activación remota mediante comunicación serial e inalámbrica. 

* Desarrollar un sistema de almacenamiento masivo y mapeo de memoria utilizando la EEPROM interna del Arduino, con el fin de resguardar la configuración de escenas personalizadas y garantizar la recuperación inmediata de los estados del sistema tras fallos de energía o reinicios de hardware.

* Diseñar e implementar un intérprete de comandos por puerto serial (USB) capaz de procesar y validar la sintaxis de un archivo de configuración externo .org, asegurando la correcta traducción de comandos de texto a estados binarios para los ambientes de la maqueta.

* Integrar módulos de comunicación inalámbrica Bluetooth (HC-06) junto con lógica digital programada para coordinar en tiempo real la activación de periféricos (LEDs, motor DC, servomotor y pantalla LCD), proporcionando una interfaz de control remota y retroalimentación visual interactiva.

---

## DIAGRAMA ESQUEMÁTICO DEL CIRCUITO

![](proteus1.jpeg)

![](proteus2.jpeg)


---

## CÓDIGO FUENTE (`.ino`)

```cpp
#include <EEPROM.h>
#include <Servo.h> //libreria servomotor
#include <Wire.h>
#include <LiquidCrystal_I2C.h> // <-- Librería I2C agregada

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

// Asignación de direcciones fijas en EEPROM
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
// Configura la dirección tentativa 0x27, 16 columnas y 2 filas
LiquidCrystal_I2C lcd(0x27, 16, 2);

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

  inicializarPantalla(); // <-- Aquí se llamará al escáner I2C

  // Configuración de pines como salida
  pinMode(sala, OUTPUT);
  pinMode(comedor, OUTPUT);
  pinMode(cocina, OUTPUT);
  pinMode(bano, OUTPUT);
  pinMode(habitacion, OUTPUT);
  pinMode(ventilador, OUTPUT);

  pinMode(pinBotonPuerta, INPUT_PULLUP);
  servomotorPuerta.attach(PinServo); // El servomotor se conecta obligatoriamente en el pin 8

  // estado inicial
  apagarTodo();
  servomotorPuerta.write(0); // 0 grados significa puerta cerrada (corregido el '10;')
  puertaAbierta = false;

  // Cargamos y aseguramos los modos predefinidos en la EEPROM
  inicializarEEPROMFabrica();

  tiempoCambio = millis();
  Serial.println(">> Sistema SmartHome GT: EEPROM y Modos Integrados Correctamente.");
}

// =========================================================================
// void loop principal
// =========================================================================
void loop()
{
  leerSerial();

  unsigned long tiempoActual = millis();

  revisarBotonPuerta();

  switch (estadoSistema)
  {
    case 0:
      modoFiesta();
      break;
    case 1:
      mensajeFiesta = false;
      applyModoDesdeEEPROM(DIR_RELAJADO);
      break;
    case 2:
      mensajeFiesta = false;
      applyModoDesdeEEPROM(DIR_NOCHE);
      break;
    case 3:
      mensajeFiesta = false;
      applyModoDesdeEEPROM(DIR_ENCENDER_TODO);
      break;
    case 4:
      mensajeFiesta = false;
      applyModoDesdeEEPROM(DIR_APAGAR_TODO);
      break;
    case 5:
      mensajeFiesta = false;
      applyModoDesdeEEPROM(DIR_CUSTOM_1);
      break;
    case 6:
      mensajeFiesta = false;
      applyModoDesdeEEPROM(DIR_CUSTOM_2);
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

  ModoInteligente comprobador;
  ModoInteligente modoNulo = {"vacio", 0, 0, 0, 0, 0, 0};

  EEPROM.get(DIR_CUSTOM_1, comprobador);
  if (comprobador.nombre[0] == (char)255 || comprobador.nombre[0] == '\0') {
    EEPROM.put(DIR_CUSTOM_1, modoNulo);
  }

  EEPROM.get(DIR_CUSTOM_2, comprobador);
  if (comprobador.nombre[0] == (char)255 || comprobador.nombre[0] == '\0') {
    EEPROM.put(DIR_CUSTOM_2, modoNulo);
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

  if (lecturaBoton == LOW && ultimoEstadoBoton == HIGH) {
    Serial.println("boton precionado");
    if ((millis() - ultimoTiempoRebote) > tiempoDebounce) {
      puertaAbierta = !puertaAbierta;
      if (puertaAbierta) {
        servomotorPuerta.write(90);
      } else {
        servomotorPuerta.write(0);
      }
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

void leerSerial() {
  while (Serial.available() > 0) {
    String linea = Serial.readStringUntil('\n');
    linea.replace("\r", "");
    linea.trim();

    if (linea == "modo_fiesta" || linea == "modo_relajado" || linea == "modo_noche" ||
        linea == "encender_todo" || linea == "apagar_todo" || linea == "modo_custom_1" || linea == "modo_custom_2") {
      if (linea == "modo_fiesta") estadoSistema = 0;
      if (linea == "modo_relajado") estadoSistema = 1;
      if (linea == "modo_noche") estadoSistema = 2;
      if (linea == "encender_todo") estadoSistema = 3;
      if (linea == "apagar_todo") estadoSistema = 4;
      if (linea == "modo_custom_1") estadoSistema = 5;
      if (linea == "modo_custom_2") estadoSistema = 6;
      tiempoCambio = millis();
      parpadeoExito();
      Serial.println("OK");
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
        } else {
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

// =========================================================================
// PANTALLA LCD (Módulo I2C Integrado)
// =========================================================================
void inicializarPantalla() {
  Serial.println("\n--- ESCANEO Y VERIFICACIÓN DE PUERTOS I2C ---");
  Wire.begin();

  byte error, direccion;
  bool dispositivoEncontrado = false;

  Serial.println("Escaneando bus I2C...");

  for(direccion = 1; direccion < 127; direccion++ ) {
    Wire.beginTransmission(direccion);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("-> ¡DISPOSITIVO ENCONTRADO en la direccion: 0x");
      if (direccion < 16) Serial.print("0");
      Serial.println(direccion, HEX);
      dispositivoEncontrado = true;
    }
    else if (error == 4) {
      Serial.print("-> Error desconocido en la direccion 0x");
      if (direccion < 16) Serial.print("0");
      Serial.println(direccion, HEX);
    }
  }
  Serial.println("Escaneo finalizado.");
  Serial.println("---------------------------------------------");

  if (dispositivoEncontrado) {
    Serial.println("Estado del LCD: Inicializando...");
    lcd.init();          // Inicializa el LCD
    lcd.backlight();     // Enciende la retroiluminación
    Serial.println("Estado del LCD: Mensaje enviado con exito.");

    // Muestra el estado inicial del sistema en pantalla
    actualizarPantalla("APAGAR TODO", "OFF", "OFF");
  } else {
    Serial.println("ALERTA: No se detecto ningun dispositivo I2C.");
    Serial.println("Por favor, revisa los cables: VCC, GND, SDA y SCL.");
  }
  Serial.println("---------------------------------------------");
}

void actualizarPantalla(String modo, String ventilador, String led) {
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");

  lcd.setCursor(0, 0);
  lcd.print("Modo: " + modo);

  lcd.setCursor(0, 1);
  lcd.print("V:" + ventilador + " LED:" + led);

  Serial.println("--------------------");
  Serial.println("Modo: " + modo);
  Serial.println("Ventilador: " + ventilador);
  Serial.println("LEDS: " + led);
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

```
---

## EXPLICACIÓN DEL FORMATO .org

El archivo con extensión .org es un formato de texto plano diseñado específicamente para este sistema domótico. Su objetivo es permitir al usuario definir parámetros de iluminación y ventilación en la PC y transferirlos al Arduino mediante el puerto serial. 

### 1. Sincronización e Inicio de Sesión
* Detección: Al recibir la cadena exacta conf_ini, el Arduino detiene cualquier rutina de iluminación activa y cambia su estado interno a Modo Configuración.
* Indicadores: El LED Azul permanece encendido, indicando que el canal de comunicación está abierto y el sistema está listo para recibir el flujo de datos.

### 2. Extracción de Modos Estáticos (Predefinidos)
* Búsqueda: El parser busca palabras clave reservadas como modo_fiesta, modo_relajado o modo_noche.
* Procesamiento: Al encontrar una de estas etiquetas, el sistema lee las líneas subsecuentes para identificar parámetros fijos de control (como el estado del ventilador ON/OFF y el comportamiento de las luces).
* Caso Especial: En el caso específico de modo_fiesta, el sistema configura internamente las banderas necesarias para ejecutar la secuencia de intermitencia (Alternándose) en los pines correspondientes.

### 3. Parseo y Descomposición de Modos Personalizados
Cada vez que el archivo declara la directiva modo_custom, el Arduino realiza una operación de división de cadenas (parsing):
* Extracción del Nombre: Filtra los caracteres dentro de las comillas. Este texto se almacena en una matriz de caracteres destinados.
* Mapeo de Actuadores: Analiza la línea del ventilador (ON/OFF) y la traduce a un estado binario (1 o 0).
* Decodificación de la Matriz de Iluminación: Analiza la cadena de texto de los ambientes (sala, comedor, cocina, baño, habitación) y extrae el estado booleano asignado a cada uno. El Arduino traduce esta línea de texto en un mapa de bits, donde cada bit representa si el pin digital de ese ambiente debe encenderse o apagarse.

### 4. Validación de Cierre y Grabado en EEPROM
El proceso de carga no se consolida hasta que el microcontrolador lee de forma exitosa la instrucción de cierre.
* Confirmación (conf:fin): Al detectar esta línea, el Arduino verifica que no se hayan levantado banderas de error de sintaxis durante el trayecto. Si todo es correcto, escribe los bloques de datos estructurados (nombres, estados del ventilador y mapas de bits de luces) en las direcciones físicas de la memoria EEPROM.
* Finalización Exitosa: El sistema sale del modo configuración, el LED Verde parpadea 3 veces como confirmación visual, y la pantalla LCD muestra de forma fija el mensaje "Configuracion guardada", regresando al modo de control normal (espera de comandos Bluetooth).

### 5. Cierre y Confirmación de Éxito
* Línea clave:conf:fin (Fin del bloque de configuración)
* Acción del Arduino: Al recibir la directiva de cierre, el Arduino finaliza la escritura.
* Retroalimentación física: El LED Verde parpadea 3 veces consecutivas.
* Mensaje en LCD: Muestra fijamente "Configuracion guardada". El sistema sale del modo configuración y regresa al modo de escucha listo para recibir comandos por Bluetooth (como modo_custom_1 que activará la escena de "Cena" o modo_custom_2 para "Cine").


### EJEMPLO DE ARCHIVO DE PRUEBA  .org

```text
conf_ini

modo_custom: "PlantaBaja"
Ventilador: OFF
LED'S: sala:ON, comedor:ON, cocina:ON, bano:OFF, habitacion:OFF

modo_custom: "PlantaAlta"
Ventilador: ON
LED'S: sala:OFF, comedor:OFF, cocina:OFF, bano:ON, habitacion:ON

conf:fin

```

---

## TABLA DE DIRECCIONES DE LA MEMORIA EEPROM

| Modo del Sistema | Byte inicial (dirección inicial) | Rango de celdas de EEPROM ocupadas | Espacio Reservado | Descripción del Uso en el Sistema |
| :--- | :---: | :---: | :---: | :--- |
| `modo fiesta` | 0 | 0 - 20 | 30 bytes | Almacena la escena predefinida de Fiesta (Ventilador ON y LEDs activos). |
| `modo relajado` | 30 | 30 - 50 | 30 bytes | Almacena la escena predefinida de Relajado (Ventilador OFF y LEDs OFF). |
| `modo noche` | 60 | 60 - 80 | 30 bytes | Almacena la escena predefinida de Noche (Ventilador OFF y LEDs OFF). |
| `encender todo` | 90 | 90 - 110 | 30 bytes | Almacena el estado global de pánico o encendido total (Todo ON). |
| `apagar todo` | 120 | 120 - 140 | 30 bytes | Almacena el estado de apagado general de la vivienda (Todo OFF). |
| `modo custom 1` | 150 | 150 - 170 | 30 bytes | Bloque dinámico asignado al primer modo personalizado cargado desde el archivo .org |
| `modo custom 2` | 180 | 180 - 200 | 30 bytes | Bloque dinámico asignado al segundo modo personalizado cargado desde el archivo .org |

---

## PRESUPUESTO ESTIMADO


| Componentes | Precio |
| :--- | :---: |
| Pantalla LCD con modulo I2C | Q51,00 |
| Cable para conexion arduino | Q15,00 |
| 12 LEDS | Q12,00 |
| 12 Resistencias 220 | Q9,00 |
| Botones | Q4,00 |
| Motor DC | Q9,00 |
| Cable para uso electrónico | Q15,50 |
| Aislante | Q12,00 |
| Cartón Ilustración, cartón chip | Q70,00 |
| Hojas decorativas, materiales de maqueta | Q37,00 |
| Sujetador de 2 baterías AA | Q5,00 |
| **TOTAL** | **Q239,50** |

---

## FOTOGRAFIAS

![](1.jpeg)

![](2.jpeg)

![](3.jpeg)

![](4.jpeg)

![](5.jpeg)

![](6.jpeg)

![](7.jpeg)

---
## CONCLUSIONES

* Persistencia de datos eficiente: El uso de la memoria EEPROM demostró ser una solución eficiente para la persistencia de datos en sistemas embebidos, logrando que el microcontrolador recupere configuraciones personalizadas de forma autónoma y sin perder la información crítica al quedarse sin energía. 

* Robustez y validación de datos: La implementación de la lógica de validación para el archivo .org permitió filtrar con éxito errores de sintaxis antes de guardar los datos, asegurando que solo las configuraciones correctas alteren el comportamiento del sistema y notificando cualquier fallo en tiempo real mediante la pantalla LCD y los LEDs de estado. 

* Control inalámbrico centralizado: La integración del módulo HC-06 con los distintos actuadores demostró que es posible centralizar el control residencial de forma inalámbrica y segura, logrando una respuesta inmediata en los ambientes (luces, ventilador y puerta) junto con un diagnóstico visual constante para el usuario.

---
## ANEXOS

* **Librería LCD RGB Backlight:** [DFRobot_LCD (GitHub)](https://github.com/DFRobotdl/DFRobot_LCD)
* **Video de funcionalidad completa:** [Video en YouTube](https://www.youtube.com/watch?v=HC-gvH02Au8)