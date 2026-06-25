#include <EEPROM.h>

// gestion de memoria

//estructura fija de bytes para almacenar estados de los modos
struct ModoInteligente {
  char nombre [15];
//para todos los estados la condicion es 0 = OFF, 1 = ON
  byte estadoVentilador;
  byte sala;
  byte comedor;
  byte cocina;
  byte bano;
  byte habitacion;
};

// asignacion de direcciones fijas en EEPROM

const int DIR_FIESTA       = 0;
const int DIR_RELAJADO     = 20;
const int DIR_NOCHE        = 40;
const int DIR_ENCENDER_TODO = 60;
const int DIR_APAGAR_TODO   = 80;
const int DIR_CUSTOM_1     = 100;
const int DIR_CUSTOM_2     = 120;

void setup() {
  //inicializar puerto serial
  Serial.begin(9600);

  //guardar configuracion de fabrica
  inicializarEEPROMFabrica();

  Serial.println("sistema de EEPROM iniciado correctamente");
  Serial.println("Escribe un comando (ej: modo_fiesta, modo_noche) para probar...");
}

//crear bucle para simular recepcion de comandos por puerto serial (USB)
void loop() {
  // Revisa si hay texto disponible ingresado desde la PC
  if (Serial.available() > 0) {
    // Lee la línea de texto completa hasta que presionas Enter
    String comando = Serial.readStringUntil('\n');
    comando.trim(); // Limpia espacios o caracteres ocultos al final del texto

    Serial.println("\n-------------------------------------------");
    Serial.print("Comando recibido por Serial: "); Serial.println(comando);

    // Mapeo exhaustivo de cada comando para jalar los datos desde su direccion fija
    if (comando == "modo_fiesta") {
      aplicarModoDesdeEEPROM(DIR_FIESTA);
    } else if (comando == "modo_relajado") {
      aplicarModoDesdeEEPROM(DIR_RELAJADO);
    } else if (comando == "modo_noche") {
      aplicarModoDesdeEEPROM(DIR_NOCHE);
    } else if (comando == "encender_todo") {
      aplicarModoDesdeEEPROM(DIR_ENCENDER_TODO);
    } else if (comando == "apagar_todo") {
      aplicarModoDesdeEEPROM(DIR_APAGAR_TODO);
    } else if (comando == "modo_custom_1") {
      aplicarModoDesdeEEPROM(DIR_CUSTOM_1);
    } else if (comando == "modo_custom_2") {
      aplicarModoDesdeEEPROM(DIR_CUSTOM_2);
    } else {
      // Si el texto no encaja con ningun comando valido de la rubrica
      Serial.println("LCD -> ERROR: Modo invalido"); 
    }
    Serial.println("-------------------------------------------");
  }
}

// iniciar memoria de fabrica

void inicializarEEPROMFabrica() {
  ModoInteligente fiesta       = {"modo_fiesta", 1, 1, 1, 1, 1, 1}; // Ajustado para coincidir con comandos
  ModoInteligente relajado     = {"modo_relajado", 0, 0, 0, 0, 0, 0};
  ModoInteligente noche        = {"modo_noche", 0, 0, 0, 0, 0, 0};
  ModoInteligente encenderTodo = {"encender_todo", 1, 1, 1, 1, 1, 1};
  ModoInteligente apagarTodo   = {"apagar_todo", 0, 0, 0, 0, 0, 0};

// guardar usando .put()

EEPROM.put(DIR_FIESTA, fiesta);
EEPROM.put(DIR_RELAJADO, relajado);
EEPROM.put(DIR_NOCHE, noche);
EEPROM.put(DIR_ENCENDER_TODO, encenderTodo);
EEPROM.put(DIR_APAGAR_TODO, apagarTodo);
}

// agregar y guardar dos modos custom

void guardarModoCustom(int numeroCustom, char* nombreModo, byte vent, byte s, byte com, byte coc, byte b, byte hab) {
  ModoInteligente nuevoCustom;
  
// copiar nombre verificando que no exceda el tamaño permitido
  strncpy(nuevoCustom.nombre, nombreModo, 14); 
  nuevoCustom.nombre[14] = '\0'; 
  
  nuevoCustom.estadoVentilador = vent;
  nuevoCustom.sala = s;
  nuevoCustom.comedor = com;
  nuevoCustom.cocina = coc;
  nuevoCustom.bano = b;
  nuevoCustom.habitacion = hab;

  // seleccionar direccion correspondiente al custom 
  int direccionDestino = (numeroCustom == 1) ? DIR_CUSTOM_1 : DIR_CUSTOM_2;
  
  // guardar custom en la EEPROM
  EEPROM.put(direccionDestino, nuevoCustom);
}

// recuperar datos de la memoria EEPROM

ModoInteligente obtenerModoDeEEPROM(int direccionBase) {
  ModoInteligente modoRecuperado;
  
  // lee el bloque completo de la busqueda
  EEPROM.get(direccionBase, modoRecuperado);
  
  //validacion de memoria correcta o vacia
  if (modoRecuperado.nombre[0] == (char)255 || modoRecuperado.nombre[0] == '\0') {
    // si detecta algo malo muestra un mensaje de error
    strcpy(modoRecuperado.nombre, "ERROR");
  }
  
  return modoRecuperado;
}

// funcion para aplicar la configuracion recuperada y simular salidas hacia el LCD
void aplicarModoDesdeEEPROM(int direccionBase) {
  // llama funcion de busqueda
  ModoInteligente modo = obtenerModoDeEEPROM(direccionBase);
  
  // verifica o devuelve fallo
  if (strcmp(modo.nombre, "ERROR") == 0) {
    Serial.println("LCD -> ERROR: Fallo en EEPROM");
    Serial.println("LED ROJO -> ACTIVADO [Fallo de Sistema]");
    return;
  }
  
  Serial.print("LCD -> Modo: "); Serial.println(modo.nombre);
  Serial.print("LCD -> Ventilador: "); Serial.println(modo.estadoVentilador == 1 ? "ON" : "OFF");
  
  // simulacion en consola
  Serial.println("--- Cambiando estados de hardware en la maqueta ---");
  Serial.print("  [Sala]:       "); Serial.println(modo.sala == 1 ? "Focos ON" : "Focos OFF");
  Serial.print("  [Comedor]:    "); Serial.println(modo.comedor == 1 ? "Focos ON" : "Focos OFF");
  Serial.print("  [Cocina]:     "); Serial.println(modo.cocina == 1 ? "Focos ON" : "Focos OFF");
  Serial.print("  [Baño]:       "); Serial.println(modo.bano == 1 ? "Focos ON" : "Focos OFF");
  Serial.print("  [Habitación]: "); Serial.println(modo.habitacion == 1 ? "Focos ON" : "Focos OFF");
  Serial.print("  [Ventilador]: "); Serial.println(modo.estadoVentilador == 1 ? "GIRANDO" : "DETENIDO");
}







