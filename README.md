Medidor de Agua en Tanques

● Este proyecto presenta un prototipo de sistema de control de nivel de agua en tanques, utilizando sensores ultrasónicos para determinar el nivel presente en cada tanque y representarlo tanto en Arduino como en una interfaz gráfica en Processing.
● Aquí se describe cómo se desarrolló el proyecto, qué hardware y software se utilizaron, cómo interactúan entre sí y cuál es el alcance final del prototipo.

🚀 Descripción General

● El sistema permite medir el nivel de agua en hasta cuatro tanques usando sensores ultrasónicos.
● Arduino toma las lecturas, controla periféricos y envía datos a Processing.
● Processing muestra los niveles en tiempo real mediante una interfaz gráfica sencilla e intuitiva.
● El proyecto fue desarrollado en el marco de la materia Informática 2.

🔧 Hardware Utilizado

● Arduino (UNO/Nano/etc.)
● Sensores ultrasónicos HC-SR04
● Pantalla LCD
● LEDs para indicar estado de bombas
● Bocina/buzzer para alertas
● Botones para seleccionar el tanque
● Cables, protoboard, alimentación

💻 Software Utilizado
Arduino

● Lectura de sensores ultrasónicos
● Conversión de señales a centímetros
● Control de LEDs, buzzer y LCD
● Comunicación serial con Processing
● Uso de librerías como LiquidCrystal y Serial

Processing

● Interfaz gráfica para visualizar niveles
● Manejo de texto, gráficos y tablas
● Uso de la librería processing.serial
● Comunicación fluida con Arduino mediante serialWrite y funciones relacionadas
● Registro opcional de datos

🔌 Comunicación Arduino ↔ Processing

● La comunicación se realiza mediante Serial a 9600 baudios.
● Arduino envía periódicamente los niveles medidos.
● Processing interpreta los datos para actualizar la interfaz.

📌 Límites y Consideraciones

● El prototipo actual soporta cuatro tanques, aunque es ampliable.
● La precisión depende del rango seguro del sensor ultrasónico.
● No se contemplan otras sustancias ni condiciones adversas.
● Como todo prototipo, presenta margen para mejoras futuras.
