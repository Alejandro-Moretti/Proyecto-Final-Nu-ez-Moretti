#include <LiquidCrystal_I2C.h>

#include <Wire.h>



//Objeto Sensor
class SensorUltrasonico {
  int trigPin;
  int echoPin;
  int ledPin;

public:
  SensorUltrasonico(int t, int e, int l) : trigPin(t), echoPin(e), ledPin(l) {}

  void iniciar() {
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
  }

  long medirDistancia() {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duracion = pulseIn(echoPin, HIGH, 30000); // 30ms = 5m máx.
    if (duracion == 0) return -1; // Sin lectura
    long distancia = duracion * 0.034 / 2;

    float distanciaLleno = 5;
    float distanciaVacio = 100;
    float porcentaje = (distanciaVacio - distancia)/(distanciaVacio - distanciaLleno) * 100.0;

    if (porcentaje < 0) porcentaje = 0;
    if (porcentaje > 100 && porcentaje <102) porcentaje = 100;
    return porcentaje;
  }

  void controlarLED(float porcentaje) {
    if (porcentaje < 100 ) digitalWrite(ledPin, HIGH); //haya una separacion mayor a 25 cm entre el sensor y el agua
    else digitalWrite(ledPin, LOW);
  }
};

//Objetos Tanque
class SistemaTanques {
  static const int sensoresCantidad = 2;
  SensorUltrasonico* sensores[sensoresCantidad];
  LiquidCrystal_I2C lcd;

  float porcentajes[sensoresCantidad]; 
  int sensorActual = 0;
  bool detenido = false;
  unsigned long ultimoCambio = 0;
  const unsigned long intervalo = 3000; // ms entre sensores
  int btnAnt, btnSig, btnStop;

public:
  SistemaTanques(SensorUltrasonico* s1, SensorUltrasonico* s2,
                 int bAnt, int bSig, int bStop)
      : lcd(0x27, 16, 2), 
        btnAnt(bAnt), btnSig(bSig), btnStop(bStop) {
    sensores[0] = s1;
    sensores[1] = s2;
   // sensores[2] = s3;
   // sensores[3] = s4;
  }

  void iniciar() {
    lcd.init();       // Inicializa la comunicación I2C
	lcd.backlight();
    lcd.clear();
    lcd.print("Iniciando...");
    delay(2000);
    lcd.clear();

    for (int i = 0; i < sensoresCantidad; i++) sensores[i]->iniciar();

    pinMode(btnAnt, INPUT_PULLUP);
    pinMode(btnSig, INPUT_PULLUP);
    pinMode(btnStop, INPUT_PULLUP);

    Serial.begin(9600); // 💬 Comunicación con Processing
  }

  void actualizar() {

    for (int i = 0; i < sensoresCantidad; i++) {
          float p = sensores[i]->medirDistancia();
          porcentajes[i] = p;
          sensores[i]->controlarLED(p);


        //  Enviar al puerto Serial para Processing
      Serial.print("Tanque: ");
      Serial.print(i + 1);
      Serial.print("  Llenado: ");
      if (p <= 0) Serial.println("---");
      else {
        Serial.print(p, 1);
        Serial.println("%\n");
      }
          
      delay(100);    
    }

    // Botón detener
    static bool lastDet = HIGH;
    bool curDet = digitalRead(btnStop);
    if (curDet == LOW && lastDet == HIGH) {
      detenido = !detenido;
      delay(100);
    }
    lastDet = curDet;

    // Navegación
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
    } else if (millis() - ultimoCambio >= intervalo) {
      sensorActual = (sensorActual + 1) % sensoresCantidad;
      ultimoCambio = millis();
    }

    // mostrar

    
    lcd.setCursor(0, 0);
    lcd.print("Tanque ");
    lcd.print(sensorActual + 1);


    lcd.setCursor(0, 1);
    lcd.print("Llenado: ");
    float p = porcentajes[sensorActual];
    if (p <= 0 || p>= 102){
      lcd.setCursor(0, 1);
      lcd.print("                "); // 16 espacios (borra toda la línea)
      lcd.setCursor(0, 1);
      lcd.print("---Error---");
    } 
    else {
      lcd.print(p, 1);
      lcd.print("%");
    }
    

  }
};

// Instancias
SensorUltrasonico s1(5, 2, A0);
SensorUltrasonico s2(4, 3, A1);
//SensorUltrasonico s3(4, 5, A2);
//SensorUltrasonico s4(4, 6, A3);

SistemaTanques sistema(&s1, &s2, 10, 9, 13);

void setup() {
  sistema.iniciar();
}

void loop() {
  sistema.actualizar();
}
