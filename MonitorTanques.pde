import processing.serial.*;

// ------------------ CONFIGURACION ------------------
// NOTA: Mirar la consola al iniciar para ver los puertos disponibles
final String COM_PORT = "COM7"; 
final int BAUD = 9600;
// Usamos sketchPath para que se guarde en la carpeta del programa
String LOG_PATH; 

// ------------------ ESTADO ------------------
float nivel1 = 0, nivel2 = 0, nivel3 = 0, nivel4 = 0;
float[] niveles = {0, 0, 0, 0}; 

int tanqueSeleccionado = 0;   // 0 = ninguno, 1..4 = tanque activo
int tanqueActualArduino = 0;  // Confirmación desde Arduino
boolean modoManual = false;   // false = Auto, true = Manual

Serial Mipuerto;
boolean puertoConectado = false;

// Flags de estado (true = está rebalsando actualmente)
boolean reb1 = false;
boolean reb2 = false;
boolean reb3 = false;
boolean reb4 = false;
//Flags de error (true = hay error de sensor)
boolean err1 = false;
boolean err2 = false;
boolean err3 = false;
boolean err4 = false;

// ------------------------ TABLA ------------------------
Table tabla; 
int ultimoGuardado = 0;   

void setup() {
  size(800, 500);
  
  // Configurar ruta del log dinámica
  LOG_PATH = sketchPath("historial_tanques.csv");
  println("El historial se guardará en: " + LOG_PATH);

  // Serial Debug: Muestra puertos disponibles
  printArray(Serial.list());
  
  try {
    Mipuerto = new Serial(this, COM_PORT, BAUD);
    Mipuerto.clear();
    puertoConectado = true; // Conectado exitosamente
    println("✅ Puerto " + COM_PORT + " conectado");
  } catch (Exception e) {
    println("¡ERROR! No se pudo abrir el puerto " + COM_PORT);
    println("Verifica que el Arduino esté conectado y el puerto sea correcto.");
  }

  File f = new File(LOG_PATH);
  if (f.exists()) {
    try {
      tabla = loadTable(LOG_PATH, "header"); // "header" respeta los títulos de columnas
      println("✅ Historial previo cargado exitosamente.");
    } catch (Exception e) {
      println("⚠️ Error leyendo archivo, creando nuevo.");
      crearTablaNueva();
    }
  } else {
    crearTablaNueva();
  }

  textFont(createFont("Arial", 14));
}

void draw() {
  background(240);

  // Sincronizo array para visualización y limita los valores entre 0 y 100
  niveles[0] = constrain(nivel1, 0, 100);
  niveles[1] = constrain(nivel2, 0, 100);
  niveles[2] = constrain(nivel3, 0, 100);
  niveles[3] = constrain(nivel4, 0, 100);

  dibujarTanque(120, niveles[0], nivel1, 1);
  dibujarTanque(270, niveles[1], nivel2, 2);
  dibujarTanque(420, niveles[2], nivel3, 3);
  dibujarTanque(570, niveles[3], nivel4, 4);
 
  dibujarInterfazBotones();

  verificarEstadoTanque(1, nivel1);
  verificarEstadoTanque(2, nivel2);
  verificarEstadoTanque(3, nivel3);
  verificarEstadoTanque(4, nivel4);

  // --- CARTEL DE ALERTA ---
  boolean hayProblema = (nivel1 > 103 || nivel2 > 103 || nivel3 > 103 || nivel4 > 103 );
  boolean hayError = (nivel1 < 0 || nivel2 < 0 || nivel3 < 0 || nivel4 < 0);
  if (hayProblema || hayError) {
    fill(255, 0, 0, 100);
    rect(0, 0, width, height);
    
    fill(255);
    stroke(0);
    strokeWeight(3);
    rect(width/2 - 200, height/2 - 150, 400, 120, 20);
    strokeWeight(1);
    
    fill(200, 0, 0);
    textAlign(CENTER, CENTER);
    textSize(36);
    text("¡ALERTA!", width/2, height/2 - 120);
    textSize(18);
    fill(0);
    if(hayProblema){
      text("Rebalse detectado. Verifique sistema.", width/2, height/2 - 80);
    }
    if(hayError){
      text("Error en los sensores detectado. Verifique sistema.", width/2, height/2 - 80);
    }

    // Guardar CSV cada 5 segundos si hubo cambios recientes
    if (millis() - ultimoGuardado > 5000) {
      saveTable(tabla, LOG_PATH);
      ultimoGuardado = millis();
      // println("Tabla guardada (Auto-save)"); 
    }
  }
}
// ------------------ FUNCIONES AUXILIARES ------------------

void verificarEstadoTanque(int id, float nivel) {
  // Obtenemos el estado actual de rebalse de este tanque usando un array o switch sucio
  // Para simplificar sin cambiar tu estructura de variables globales booleanas:
  boolean Rebalse = false;
  boolean Error = false;

  if (id == 1) Rebalse = reb1;
  if (id == 2) Rebalse = reb2;
  if (id == 3) Rebalse = reb3;
  if (id == 4) Rebalse = reb4;

  if(id == 1) Error = err1;
  if(id == 2) Error = err2;
  if(id == 3) Error = err3;
  if(id == 4) Error = err4;

  // CASO 1: Entra en rebalse (Sube de 100)
  if (nivel > 100 && !Rebalse) {
    registrarEvento(id, nivel, "REBALSE");
    println("⚠️ T" + id + " REBALSANDO");
    enviarAlertaArduino(id);
    setRebalse(id, true); // Actualizamos flag
  }
  
  // CASO 2: Se normaliza (Baja a 100 o menos)
  else if (nivel <= 100 && Rebalse) {
    registrarEvento(id, nivel, "NORMALIZADO");
    println("✅ T" + id + " Normalizado");
    setRebalse(id, false); // Actualizamos flag
  }

  // CASO 3: Error de sensor (nivel negativo)
  if(nivel < 0 && !Error){
      registrarEvento(id, nivel, "ERROR DE SENSOR");
      println("✅ T" + id + " ERROR DE SENSOR");
      setError(id, true); // Actualizamos flag de error
    }
    else if(nivel >= 0 && Error){
      registrarEvento(id, nivel, "SENSOR OK");
      println("✅ T" + id + " SENSOR OK");
      setError(id, false); // Actualizamos flag de error
    }
}
// Helper para actualizar las variables booleanas globales
void setRebalse(int id, boolean estado) {
  if (id == 1) reb1 = estado;
  if (id == 2) reb2 = estado;
  if (id == 3) reb3 = estado;
  if (id == 4) reb4 = estado;
}
//Helper para actualizar las variables booleanas de error globales
void setError(int id, boolean estado) {
  if (id == 1) err1 = estado;
  if (id == 2) err2 = estado;
  if (id == 3) err3 = estado;
  if (id == 4) err4 = estado;
}


void enviarAlertaArduino(int tanque) {
  if (puertoConectado) {
    Mipuerto.write('R'); 
    Mipuerto.write(char('0' + tanque)); 
  }
}

void registrarEvento(int tanque, float porcentaje, String evento) {
  TableRow fila = tabla.addRow();
  String fecha = nf(day(),2) + "/" + nf(month(),2) + "/" + year();
  String hora  = nf(hour(),2) + ":" + nf(minute(),2) + ":" + nf(second(), 2);

  fila.setString("fecha", fecha);
  fila.setString("hora", hora);
  fila.setInt("tanque", tanque);
  fila.setFloat("porcentaje", porcentaje);
  fila.setString("evento", evento);
  
}

// ------------------ INTERACCIÓN ------------------
void mousePressed() {
  // BOTÓN MODO (manual <-> auto)
  if (mouseX > 650 && mouseX < 770 && mouseY > 420 && mouseY < 470) {
    modoManual = !modoManual;   
    if (puertoConectado) {
      if (modoManual) {
        println("-> MODO MANUAL");
        Mipuerto.write('M');
      } else {
        println("-> MODO AUTOMÁTICO");
        Mipuerto.write('A');
      }
    }
    tanqueSeleccionado = 0; 
    return; // Salimos para no clickear dos cosas a la vez
  }

  // Selección tanques SOLO en modo manual
  if (modoManual) {
    int nuevoSeleccionado = 0;
    
    // Chequeo de zonas de click
    if (mouseX > 20 && mouseX < 120 && mouseY > 420 && mouseY < 470) nuevoSeleccionado = 1;
    else if (mouseX > 170 && mouseX < 270 && mouseY > 420 && mouseY < 470) nuevoSeleccionado = 2;
    else if (mouseX > 320 && mouseX < 420 && mouseY > 420 && mouseY < 470) nuevoSeleccionado = 3;
    else if (mouseX > 470 && mouseX < 570 && mouseY > 420 && mouseY < 470) nuevoSeleccionado = 4;

    if (nuevoSeleccionado != 0) {
      // Si clickeo el mismo que ya está, lo apago
      if (nuevoSeleccionado == tanqueSeleccionado) {
        tanqueSeleccionado = 0;
        if (puertoConectado) Mipuerto.write('0');
        println("-> Tanque Desactivado");
      } else {
        // Si es uno nuevo, lo activo
        tanqueSeleccionado = nuevoSeleccionado;
        if (puertoConectado) Mipuerto.write(char('0' + tanqueSeleccionado));
        println("-> Activando Tanque " + tanqueSeleccionado);
      }
    }
  }
}

// ------------------ RECEPCIÓN SERIE ------------------
void serialEvent(Serial p) {
  try {
    String datos = p.readStringUntil('\n');
    if (datos == null) return;
    datos = trim(datos);
    if (datos.length() == 0) return;

    // 1. Mensajes de Niveles (CSV: "23.4,50.1,10.0,5.5")
    if (datos.indexOf(',') > 0 && !datos.startsWith("MODO") && !datos.startsWith("TANQUE")) {
       String[] valores = split(datos, ',');
       if (valores.length >= 2) {
         nivel1 = float(valores[0]);
         nivel2 = float(valores[1]);
         if (valores.length >= 3) nivel3 = float(valores[2]);
         if (valores.length >= 4) nivel4 = float(valores[3]);
       }
       return;
    }

    // 2. Mensajes de Control
    if (datos.startsWith("MODO:")) {
      String v = datos.substring(5).trim().toUpperCase();
      modoManual = v.equals("MANUAL");
    }
    else if (datos.startsWith("TANQUE:")) {
      tanqueActualArduino = int(trim(datos.substring(7)));
    }
    
    // 3. Debug en consola
    // println("Arduino: " + datos);
    
  } catch (Exception e) {
    println("Error serie: " + e.toString());
  }
}

// ------------------ DIBUJO INTERFAZ ------------------
void dibujarInterfazBotones() {
  textAlign(CENTER, CENTER);
  
  // MODO
  if (!modoManual) fill(0, 180, 0); else fill(255, 0, 0);
  rect(650, 420, 120, 50, 10);
  fill(255); textSize(16);
  text(modoManual ? "MANUAL" : "AUTOMÁTICO", 710, 445);

  // TANQUES
  dibujarBotonTanque(1, 20, "T1");
  dibujarBotonTanque(2, 170, "T2");
  dibujarBotonTanque(3, 320, "T3");
  dibujarBotonTanque(4, 470, "T4");
}

void dibujarBotonTanque(int id, int x, String etiqueta) {
  if (tanqueSeleccionado == id) fill(0, 200, 0); // Activo seleccionado por usuario
  else if (!modoManual) fill(200); // Deshabilitado en auto
  else fill(150); // Habilitado pero inactivo
  
  rect(x, 420, 100, 50);
  fill(0); textSize(18);
  text(etiqueta, x + 50, 445);
}

void dibujarTanque(float x, float nivelVis, float nivelReal, int idx) {
  stroke(0); fill(220);
  rect(x, 50, 100, 300); // Contenedor

  fill(0, 100, 255);
  float alturaAgua = map(nivelVis, 0, 100, 0, 300);
  rect(x, 350 - alturaAgua, 100, alturaAgua); // Agua

  fill(0); textSize(14);
  text(int(nivelVis) + "%", x + 50, 380);
  
  // Alerta visual individual
  if (nivelReal > 100 || nivelReal < 0) {
    fill(255, 0, 0);
    ellipse(x + 50, 40, 30, 30);
    fill(255); text("!", x + 50, 40);
  }
  
  // Indicador de selección de Arduino (feedback real)
  if (tanqueActualArduino == idx) {
    fill(255, 200, 0); // Naranja
    rect(x, 350, 100, 10); // Barra base indicadora
  }
}

void crearTablaNueva() {
  tabla = new Table();
  tabla.addColumn("fecha");
  tabla.addColumn("hora");
  tabla.addColumn("tanque");
  tabla.addColumn("porcentaje");
  tabla.addColumn("evento");
}
