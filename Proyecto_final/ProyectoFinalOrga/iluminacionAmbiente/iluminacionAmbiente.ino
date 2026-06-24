
const int sala = 2;
const int comedor = 3;
const int cocina = 4;
const int bano = 5;
const int habitacion = 6;
const int ventilador = 9;

unsigned long tiempoCambio = 0;
unsigned long tiempoFiesta = 0;
int estadoSistema = 0;
bool alternanciaFiesta = false;
bool mensajeFiesta = false;


String nombreCustom1 = "";
bool ventiladorCustom1 = false;
bool salaCustom1 = false;
bool comedorCustom1 = false;
bool cocinaCustom1 = false;
bool banoCustom1 = false;
bool habitacionCustom1 = false;


String nombreCustom2 = "";
bool ventiladorCustom2 = false;
bool salaCustom2 = false;
bool comedorCustom2 = false;
bool cocinaCustom2 = false;
bool banoCustom2 = false;
bool habitacionCustom2 = false;

//Lector .org
enum EstadoParser { ESPERANDO_INI, LEYENDO_ARCHIVO, LEYENDO_CUSTOM };
EstadoParser estadoActual = ESPERANDO_INI;

int indiceCustom = 1; 
String tempNombre = "";
bool tempVentilador = false;
int lineasLeidasCustom = 0;

// CONFIGURACIÓN INICIAL
void setup()
{
  Serial.begin(9600);

  pinMode(sala, OUTPUT);
  pinMode(comedor, OUTPUT);
  pinMode(cocina, OUTPUT);
  pinMode(bano, OUTPUT);
  pinMode(habitacion, OUTPUT);
  pinMode(ventilador, OUTPUT);

  apagarTodo();

  configurarCustom1(
    "CENA",
    false,
    true,
    true,
    false,
    false,
    false
  );
  
  tiempoCambio = millis();
}

// BUCLE PRINCIPAL
void loop()
{
  leerArchivoOrg();

  unsigned long tiempoActual = millis();

  switch (estadoSistema)
  {
    case 0:
      modoFiesta();
      if (tiempoActual - tiempoCambio >= 3000)
      {
        tiempoCambio = tiempoActual;
        mensajeFiesta = false;
        estadoSistema = 1;
      }
      break;

    case 1:
      modoRelajado();
      if (tiempoActual - tiempoCambio >= 3000)
      {
        tiempoCambio = tiempoActual;
        estadoSistema = 2;
      }
      break;

    case 2:
      modoNoche();
      if (tiempoActual - tiempoCambio >= 3000)
      {
        tiempoCambio = tiempoActual;
        estadoSistema = 3;
      }
      break;

    case 3:
      encenderTodo();
      if (tiempoActual - tiempoCambio >= 3000)
      {
        tiempoCambio = tiempoActual;
        estadoSistema = 4;
      }
      break;

    case 4:
      apagarTodo();
      if (tiempoActual - tiempoCambio >= 3000)
      {
        tiempoCambio = tiempoActual;
        estadoSistema = 5;
      }
      break;

    case 5:
      modoCustom1();
      if (tiempoActual - tiempoCambio >= 3000)
      {
        tiempoCambio = tiempoActual;
        estadoSistema = 0; 
      }
      break;
  }
}

// FUNCIONES DEL LECTOR .ORG
void leerArchivoOrg() {
  while (Serial.available() > 0) {
    String linea = Serial.readStringUntil('\n');
    linea.replace("\r", ""); 
    linea.trim();
    
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
        }
        break;

      case LEYENDO_ARCHIVO:
        if (linea == "conf:fin") {
           estadoActual = ESPERANDO_INI;
           Serial.println("ÉXITO: Configuracion guardada");
        } 
        else if (linea.startsWith("modo_custom:")) {
           int primerComilla = linea.indexOf('"');
           int segundaComilla = linea.lastIndexOf('"');
           
           if(primerComilla != -1 && segundaComilla != -1 && segundaComilla > primerComilla) {
             tempNombre = linea.substring(primerComilla + 1, segundaComilla);
             if (tempNombre.length() > 10) {
                tempNombre = tempNombre.substring(0, 10); 
             }
             estadoActual = LEYENDO_CUSTOM;
             lineasLeidasCustom = 0;
           } else {
             Serial.println("Error: Archivo.org (Sintaxis de nombre custom)");
             estadoActual = ESPERANDO_INI; 
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
             if (indiceCustom == 1) {
                configurarCustom1(tempNombre, tempVentilador, tSala, tComedor, tCocina, tBano, tHab);
                indiceCustom++;
             } else if (indiceCustom == 2) {
                configurarCustom2(tempNombre, tempVentilador, tSala, tComedor, tCocina, tBano, tHab);
             }
             estadoActual = LEYENDO_ARCHIVO; 
           }
        }
        else {
           Serial.println("Error: modo custom incompleto o con sintaxis mala");
           estadoActual = ESPERANDO_INI; 
        }
        break;
    }
  }
}

// FUNCIONES DE CONTROL DE MODOS
void ActualizarPantalla(
  String modo,
  String estadoVentilador,
  String estadoLeds
)
{
  Serial.println("--------------------");
  Serial.println("Modo: " + modo);
  Serial.println("Ventilador: " + estadoVentilador);
  Serial.println("LEDS: " + estadoLeds);
}

void modoFiesta()
{
  digitalWrite(ventilador, HIGH);
  if (!mensajeFiesta)
  {
    ActualizarPantalla("FIESTA","ON","ALTERNANDO");
    mensajeFiesta = true;
  }

  unsigned long tiempoActual = millis();
  if (tiempoActual - tiempoFiesta >= 100)
  {
    tiempoFiesta = tiempoActual;
    alternanciaFiesta = !alternanciaFiesta;
    
    if (alternanciaFiesta)
    {
      digitalWrite(sala, HIGH);
      digitalWrite(cocina, HIGH);
      digitalWrite(habitacion, HIGH);
      digitalWrite(comedor, LOW);
      digitalWrite(bano, LOW);
    }
    else
    {
      digitalWrite(sala, LOW);
      digitalWrite(cocina, LOW);
      digitalWrite(habitacion, LOW);
      digitalWrite(comedor, HIGH);
      digitalWrite(bano, HIGH);
    }
  }
}

void modoRelajado()
{
  static bool mensaje = false;
  if (!mensaje)
  {
    apagarTodo();
    ActualizarPantalla(
      "RELAJADO",
      "OFF",
      "OFF"
    );
    mensaje = true;
  }
}

void modoNoche()
{
  static bool mensaje = false;
  if (!mensaje)
  {
    apagarTodo();
    ActualizarPantalla("NOCHE", "OFF", "OFF");
    mensaje = true;
  }
}

void encenderTodo()
{
  static bool mensaje = false;
  if (!mensaje)
  {
    digitalWrite(sala, HIGH);
    digitalWrite(comedor, HIGH);
    digitalWrite(cocina, HIGH);
    digitalWrite(bano, HIGH);
    digitalWrite(habitacion, HIGH);

    digitalWrite(ventilador, HIGH);
    ActualizarPantalla(
      "ALL",
      "ON",
      "ON"
    );
    mensaje = true;
  }
}

void apagarTodo()
{
  digitalWrite(sala, LOW);
  digitalWrite(comedor, LOW);
  digitalWrite(cocina, LOW);
  digitalWrite(bano, LOW);
  digitalWrite(habitacion, LOW);

  digitalWrite(ventilador, LOW);
}

// FUNCIONES CUSTOM Y DE VINCULACIÓN
void configurarCustom1(
  String nombre,
  bool estadoVentilador,
  bool estadoSala,
  bool estadoComedor,
  bool estadoCocina,
  bool estadoBano,
  bool estadoHabitacion
)
{
  nombreCustom1 = nombre;
  ventiladorCustom1 = estadoVentilador;

  salaCustom1 = estadoSala;
  comedorCustom1 = estadoComedor;
  cocinaCustom1 = estadoCocina;
  banoCustom1 = estadoBano;
  habitacionCustom1 = estadoHabitacion;
}

void modoCustom1()
{
  digitalWrite(sala, salaCustom1);
  digitalWrite(comedor, comedorCustom1);
  digitalWrite(cocina, cocinaCustom1);
  digitalWrite(bano, banoCustom1);
  digitalWrite(habitacion, habitacionCustom1);

  digitalWrite(
    ventilador,
    ventiladorCustom1
  );

  ActualizarPantalla(
    nombreCustom1,
    ventiladorCustom1 ? "ON" : "OFF",
    "CUSTOM"
  );
}

void configurarCustom2(
  String nombre,
  bool estadoVentilador,
  bool estadoSala,
  bool estadoComedor,
  bool estadoCocina,
  bool estadoBano,
  bool estadoHabitacion
)
{
  nombreCustom2 = nombre;
  ventiladorCustom2 = estadoVentilador;

  salaCustom2 = estadoSala;
  comedorCustom2 = estadoComedor;
  cocinaCustom2 = estadoCocina;
  banoCustom2 = estadoBano;
  habitacionCustom2 = estadoHabitacion;
}

void modoCustom2()
{
  digitalWrite(sala, salaCustom2);
  digitalWrite(comedor, comedorCustom2);
  digitalWrite(cocina, cocinaCustom2);
  digitalWrite(bano, banoCustom2);
  digitalWrite(habitacion, habitacionCustom2);

  digitalWrite(ventilador, ventiladorCustom2);
  
  ActualizarPantalla(nombreCustom2, ventiladorCustom2 ? "ON" : "OFF", "CUSTOM");
}