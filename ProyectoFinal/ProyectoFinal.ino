#include <LiquidCrystal_I2C.h>
#include <Wire.h>

//Objeto Sensor
class SensorUltrasonico {
private:
  int trigPin;
  int echoPin;
  int ledPin;

  // Buffer para promediar las lecturas
  static const int NUM_LECTURAS = 5;
  float lecturas[NUM_LECTURAS];
  int indice = 0;
  int erroresConsecutivos = 0;
  static const int MAX_ERRORES_TOLERABLES = 20;

public:
  // Constructor de Objeto                                                                             
  SensorUltrasonico(int t, int e, int l)
    : trigPin(t), echoPin(e), ledPin(l){
  // Inicializar buffer de lecturas
      for (int i = 0; i < NUM_LECTURAS; i++) {
        lecturas[i] = 0;
      }
    }

  void iniciar() {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    // --- FASE DE CALENTAMIENTO--- (tambien se le llama Warm-Uo)
    // Intentamos obtener una lectura válida inicial para no arrancar en 0 falso
    float lecturaInicial = -1; //colocamos -1 por si en ninguno de los 10 intentos la duracion supera "0" entonces nos dá error 
    
    // Hacemos hasta 10 intentos rápidos para conseguir un dato
    for(int k=0; k < 10; k++) {
        digitalWrite(trigPin, LOW); delayMicroseconds(2);
        digitalWrite(trigPin, HIGH); delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        
        long dur = pulseIn(echoPin, HIGH, 15000);
        
        if (dur > 0) {
           // Si se realizó correctamete la lectura calculamos y guardamos utilizando variables auxiliares
           long dist = dur * 0.034 / 2;
           float dLleno = 10; float dVacio = 100;
           lecturaInicial = (dVacio - dist)/(dVacio - dLleno) * 100.0;
           if (lecturaInicial < 0) lecturaInicial = 0;
           break;
        }
        delay(20); // Esperar un poco antes de reintentar
    }

    // Llenamos TODO el buffer con esta lectura inicial
    for (int i = 0; i < NUM_LECTURAS; i++) {
      lecturas[i] = lecturaInicial;
    }
  }

  float medirDistancia() {
    // 1. Disparo
    digitalWrite(trigPin, LOW); delayMicroseconds(2);
    digitalWrite(trigPin, HIGH); delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // 2. Lectura
    long duracion = pulseIn(echoPin, HIGH, 15000); 

    float nuevoPorcentaje = 0;
    int indiceAnterior = (indice + NUM_LECTURAS - 1) % NUM_LECTURAS;

    // 3. Lógica para controlar la cantidad de fallos que admitimos
    if (duracion == 0) {
      erroresConsecutivos++; // Aumentamos la cuenta de errores
      
      if (erroresConsecutivos >= MAX_ERRORES_TOLERABLES) {
         // Error en el sensor 
         nuevoPorcentaje = -1; 
      } else {
         // Usamos el último valor bueno
         nuevoPorcentaje = lecturas[indiceAnterior];
      }
      
    } else {
      
      erroresConsecutivos = 0; // Reseteamos el contador de errores (todo está bien)
      
      long distancia = duracion * 0.034 / 2;
      float dLleno = 10; float dVacio = 150; // Ajusta tus distancias aquí
      nuevoPorcentaje = (dVacio - distancia) / (dVacio - dLleno) * 100.0;
      
      if (nuevoPorcentaje < 0) nuevoPorcentaje = 0; //En este caso el sensor estía leyendo más lejos de lo admitido
    }

    // 4. Actualizar Buffer ya inicializado anteriormente
    lecturas[indice] = nuevoPorcentaje;
    indice = (indice + 1) % NUM_LECTURAS; //Operación matematica para ir moviendo el indice según el resto de una división

    // 5. Calcular Promedio
    float suma = 0;
    int validos = 0;
    

    for (int i = 0; i < NUM_LECTURAS; i++) {
    
      suma += lecturas[i];
    }
    
    float promedio = suma / NUM_LECTURAS;
    
    // Si el valor ACTUAL es -1 (sensor no funciona), devolvemos -1 directo.
    if (nuevoPorcentaje == -1) return -1;
    
    return promedio;
  }


  void controlarLED(float porcentaje) {
    if (porcentaje < 100 ){
      digitalWrite(ledPin, HIGH); 
    } 
    else digitalWrite(ledPin, LOW);
  }
  
};

//Objetos Tanque
class SistemaTanques {
  private:
    static const int sensoresCantidad = 4;
    SensorUltrasonico* sensores[sensoresCantidad];
    LiquidCrystal_I2C lcd;

    float porcentajes[sensoresCantidad]; 
    int sensorActual = 0;
    bool detenido = false;
    unsigned long ultimoCambio = 0;
    const unsigned long intervalo = 3000; // ms entre sensores
    int btnAnt, btnSig, btnStop;
    int Buzzer;
    

    //funcion que recibe el comando (cmd) de la pc
    void procesarComando(char cmd) {
      if (cmd == 'M') {          // M = modo manual
        detenido = true;
        Serial.println("MODO:MANUAL");
      }
      else if (cmd == 'A') {     // A = modo automático
        detenido = false;
        Serial.println("MODO:AUTO"); 
      }
      else if (cmd == '1') {     // 1 = Sensor 1
        if (detenido) {
          sensorActual = 0;
        }
      }
      else if (cmd == '2') {     // 2 = Sensor 2
        if (detenido) {
          sensorActual = 1;
        }
      }
      else if (cmd == '3') {     // 3 = Sensor 3
        if (detenido) {
          sensorActual = 2;
        }
      }
      else if (cmd == '4') {     // 4=Sensor4
        if (detenido) {
          sensorActual = 3;
        }
      }
    }

    //funcion para navegacion manual o automatica
    void navegar(){
       // Botón detener
      static bool ultimoStop = HIGH; //boton suelto es HIGH y boton apretado es LOW
      bool actualStop = digitalRead(btnStop);
      if (actualStop == LOW && ultimoStop == HIGH) { // Se presionó el boton Stop y la ultima vez estaba suelto
        detenido = !detenido; //detenido pasa a LOW 

      if (detenido) Serial.println("MODO:MANUAL");
        else  Serial.println("MODO:AUTO");

        delay(100);
      }
      ultimoStop = actualStop;

      // Navegación manual
      static int lastSensor = sensorActual;

      if (detenido) {
        if (digitalRead(btnSig) == LOW) {
          sensorActual = (sensorActual + 1) % sensoresCantidad;
          delay(100);
        }
        if (digitalRead(btnAnt) == LOW) {
          sensorActual--;
          if (sensorActual < 0) sensorActual = sensoresCantidad - 1;
          delay(100);
        }
      }
      
      //navegacion automatica
      else if (millis() - ultimoCambio >= intervalo) {
        sensorActual = (sensorActual + 1) % sensoresCantidad;
        ultimoCambio = millis();
      }

      // ----- ENVIAR CAMBIO DE SENSOR A PROCESSING -----
      if (sensorActual != lastSensor) {
        Serial.print("TANQUE:");
        Serial.println(sensorActual + 1);
        lastSensor = sensorActual;
      }

    }

public:
  SistemaTanques(SensorUltrasonico* s1, SensorUltrasonico* s2, SensorUltrasonico* s3, SensorUltrasonico* s4, int bAnt, int bSig, int bStop, int B):
   lcd(0x27, 16, 2), btnAnt(bAnt), btnSig(bSig), btnStop(bStop), Buzzer(B) {
    sensores[0] = s1;
    sensores[1] = s2;
    sensores[2] = s3;
    sensores[3] = s4;
  }

  void iniciar() {
    lcd.init();       // Inicializa la comunicación I2C
	lcd.backlight();
    lcd.clear();
    lcd.print("Iniciando...");
    delay(2000);
    lcd.clear();

    for (int i = 0; i < sensoresCantidad; i++){
		sensores[i]->iniciar();
	}

    pinMode(btnAnt, INPUT_PULLUP);
    pinMode(btnSig, INPUT_PULLUP);
    pinMode(btnStop, INPUT_PULLUP);
    pinMode(Buzzer, OUTPUT);
    

    Serial.begin(9600);
  }

  void actualizar() {

    noTone(Buzzer); //el titileo en el buzzer surge de que el arduino no procesa el "tono del buzzer y el trig de los sensores" a la vez
                    //por lo tanto hay que apagar el buzzer cada vez que se midan los sensores 
    
    // Damos un pequeñísimo respiro al procesador (opcional pero recomendado)
    delay(20);

    bool hayEmergencia = false;

    for (int i = 0; i < sensoresCantidad; i++) {
          float p = sensores[i]->medirDistancia();
          delay(60);
          porcentajes[i] = p;
          sensores[i]->controlarLED(p);

        //  Enviar al puerto Serial para Processing
        Serial.print(p, 1);
        if (i < sensoresCantidad - 1){
			Serial.print(",");
		}
        if (p >= 103){
          hayEmergencia = true;
        } 
      delay(10);    
    }
    Serial.print("\n"); //Envío el salto de linea al puerto serie

    while (Serial.available() > 0) {
      char c = Serial.read();
      procesarComando(c);
    }
    navegar(); //funcion para navegacion manual o automatica

  //buzzer en caso de rebalse
    if (hayEmergencia) {
      tone(Buzzer, 3000);
    } else {
      noTone(Buzzer);
    }

    // mostrar
    lcd.setCursor(0, 0);
    lcd.print("Tanque ");
    lcd.print(sensorActual + 1);

    lcd.setCursor(0, 1);
    lcd.print("Llenado:        ");
    lcd.setCursor(9, 1);
    float p = porcentajes[sensorActual];
    if (p < 0 || p>= 103){
      lcd.setCursor(0, 1);
      lcd.print("                "); // 16 espacios (borra toda la línea)
      lcd.setCursor(0, 1);
      lcd.print("---Error---");
      
    } 
    else {
      if(p>100){
		  p=100;
	  }

      lcd.print(p, 1);
      lcd.print("%");
      
    }
  }
};

// Instancias
SensorUltrasonico s1(5, 2, A0);
SensorUltrasonico s2(4, 3, A1);
SensorUltrasonico s3(8, 6, A2);
SensorUltrasonico s4(9, 7, A3);

SistemaTanques sistema(&s1, &s2, &s3, &s4, 10, 11, 13, 12);

void setup() {
  sistema.iniciar();
}
void loop() {
  sistema.actualizar();
}
