export interface Project {
  title: string;
  desc: string;
  diff: 'facil' | 'medio' | 'avanzado';
  parts: string;
  link?: string;
}

export interface Component {
  name: string;
  qty: string;
  icon: string;
  img?: string;
  cat: string;
  demo?: string;
  projects: Project[];
}

export const CATS = [
  { id: "todo", label: "Todo" },
  { id: "sensor", label: "Sensores" },
  { id: "motor", label: "Motores" },
  { id: "display", label: "Pantallas" },
  { id: "comunicacion", label: "Comunicación" },
  { id: "control", label: "Control" },
  { id: "electronica", label: "Electrónica" },
  { id: "audio", label: "Audio" },
] as const;

export const DIFF_LABELS: Record<string, string> = {
  facil: "Principiante",
  medio: "Intermedio",
  avanzado: "Avanzado",
};

export const components: Component[] = [
  { name: "ESP-32 Board", qty: "1PC", icon: "📡", img: "/img/esp32.png", cat: "comunicacion", demo: "/demos/esp32-blink", projects: [
    { title: "Portal WiFi Cautivo", desc: "Crea tu propio punto de acceso WiFi. Cualquier dispositivo que se conecte ve una página personalizada.", diff: "facil", parts: "Solo ESP-32", link: "/proyectos-board/1-wifi_cautivo" },
    { title: "Servidor web con WiFi", desc: "Controla el LED integrado desde cualquier navegador en tu red local. Sin apps, sin cables.", diff: "facil", parts: "ESP-32, LED, Resistor", link: "/proyectos-board/2-servidor_web" },
    { title: "Reloj NTP por Internet", desc: "Sincroniza la hora exacta con servidores NTP. Muestra fecha y hora completas en el Monitor Serie.", diff: "facil", parts: "Solo ESP-32", link: "/proyectos-board/3-reloj_ntp" },
    { title: "Escáner de Redes WiFi", desc: "Lista todas las redes WiFi cercanas con señal, canal y tipo de cifrado. Se actualiza cada 10 segundos.", diff: "facil", parts: "Solo ESP-32", link: "/proyectos-board/4-escaneo_wifi" },
    { title: "Terminal Bluetooth Serial", desc: "Empareja el ESP-32 con tu teléfono y envía comandos por Bluetooth para controlar el LED.", diff: "medio", parts: "Solo ESP-32", link: "/proyectos-board/5-bluetooth" },
    { title: "Actualizaciones OTA", desc: "Actualiza el firmware del ESP-32 por WiFi sin cable USB. Ideal para proyectos ya montados.", diff: "medio", parts: "Solo ESP-32", link: "/proyectos-board/6-ota" },
  ]},
  { name: "0.96\" OLED Display", qty: "1PC", icon: "🖥️", img: "/img/oled-96inch.png", cat: "display", demo: "/demos/oled-hola-mundo", projects: [
    { title: "Reloj digital", desc: "Muestra hora, fecha y temperatura en la pantalla OLED.", diff: "facil", parts: "OLED, DHT11", link: "/proyectos-oled/1-reloj_digital" },
    { title: "Medidor de distancia", desc: "Muestra en tiempo real la distancia medida por el sensor ultrasónico.", diff: "facil", parts: "OLED, Ultrasonic Sensor", link: "/proyectos-oled/2-medidor_distancia" },
    { title: "Snake", desc: "El clásico juego Snake con joystick y pantalla OLED. El juego se acelera con cada comida.", diff: "avanzado", parts: "OLED, Joystick", link: "/proyectos-oled/3-snake" },
    { title: "Nivel Digital", desc: "Muestra ángulos Pitch y Roll en tiempo real con barras indicadoras. Detecta cuando está nivelado.", diff: "facil", parts: "OLED, GY-6500", link: "/proyectos-oled/4-nivel_digital" },
    { title: "Calculadora", desc: "Calculadora de 4 operaciones con teclado matricial 4×4. Muestra la expresión y el resultado en pantalla.", diff: "medio", parts: "OLED, Keypad 4×4", link: "/proyectos-oled/5-calculadora" },
  ]},
  { name: "GY-6500 Module", qty: "1PC", icon: "📐", img: "/img/gy-6500.png", cat: "sensor", demo: "/demos/gy6500-test", projects: [
    { title: "Nivel digital", desc: "Muestra el ángulo de inclinación en la pantalla OLED.", diff: "facil", parts: "GY-6500, OLED", link: "/proyectos-oled/4-nivel_digital" },
    { title: "Gimbal simple", desc: "Usa el giroscopio para controlar servos y mantener estable una plataforma.", diff: "avanzado", parts: "GY-6500, Servo SG90", link: "/proyectos-gy6500/1-gimbal_simple" },
    { title: "Controlador de gestos", desc: "Controla el color del LED RGB según la orientación del módulo.", diff: "medio", parts: "GY-6500, RGB LED", link: "/proyectos-gy6500/2-controlador_gestos" },
  ]},
  { name: "Power Supply Module", qty: "1PC", icon: "🔌", img: "/img/power-supply.png", cat: "electronica", demo: "/demos/power-supply-test", projects: [
    { title: "Fuente regulada", desc: "Provee 3.3V y 5V estables a todos tus circuitos en el protoboard.", diff: "facil", parts: "Power Supply, Protoboard" },
    { title: "Banco de pruebas", desc: "Alimenta múltiples módulos simultáneamente durante el desarrollo.", diff: "facil", parts: "Power Supply, todos los módulos" },
  ]},
  { name: "ULN2003 + Stepper Motor", qty: "1PC c/u", icon: "⚙️", img: "/img/ULN2003-stepper-motor.png", cat: "motor", demo: "/demos/stepper-test", projects: [
    { title: "Indicador de Aguja (Gauge)", desc: "Convierte el motor en un medidor analógico de aguja. El potenciómetro mueve la aguja sobre una escala de ~270°.", diff: "facil", parts: "Stepper, ULN2003, Potenciómetro", link: "/proyectos-stepper/1-indicador_aguja" },
    { title: "Cortina automática", desc: "Abre y cierra una cortina según la luz ambiente (LDR con histéresis) y con un botón de control manual.", diff: "medio", parts: "Stepper, ULN2003, Photoresistor, Button", link: "/proyectos-stepper/2-cortina_automatica" },
    { title: "Seguidor de luz (Sun Tracker)", desc: "El motor orienta una superficie hacia la mayor fuente de luz comparando dos LDR (lazo cerrado).", diff: "medio", parts: "Stepper, ULN2003, Photoresistor x2", link: "/proyectos-stepper/3-seguidor_luz" },
    { title: "Cerradura / Caja fuerte con código", desc: "El motor actúa como pestillo motorizado, desbloqueado con un código en el teclado matricial.", diff: "medio", parts: "Stepper, ULN2003, Keypad 4×4, OLED", link: "/proyectos-stepper/4-caja_fuerte" },
    { title: "Dispensador automático", desc: "Gira un tambor para dispensar porciones a horas programadas (NTP) o por botón.", diff: "avanzado", parts: "Stepper, ULN2003, OLED, Button", link: "/proyectos-stepper/5-dispensador" },
  ]},
  { name: "Servo Motor SG90", qty: "1PC", icon: "🦾", img: "/img/Servo-motor-SG90.png", cat: "motor", demo: "/demos/servo-test", projects: [
    { title: "Barrera automática", desc: "Sube y baja una barrera cuando se presiona un botón.", diff: "facil", parts: "Servo SG90, Button" },
    { title: "Brazo robótico", desc: "Controla el ángulo del servo con el potenciómetro en tiempo real.", diff: "facil", parts: "Servo SG90, Potenciómetro" },
    { title: "Seguidor de luz", desc: "El servo orienta un panel hacia la fuente de luz más intensa.", diff: "medio", parts: "Servo SG90, Photoresistor x2" },
  ]},
  { name: "5V Relay", qty: "1PC", icon: "⚡", img: "/img/5V-Relay.png", cat: "control", demo: "/demos/relay-test", projects: [
    { title: "Enchufe inteligente", desc: "Enciende o apaga un dispositivo de 110V/220V con una señal de 5V.", diff: "medio", parts: "5V Relay, ESP-32" },
    { title: "Temporizador de riego", desc: "Activa una bomba de agua a intervalos programados.", diff: "medio", parts: "5V Relay, ESP-32" },
    { title: "Alarma con sirena", desc: "Activa una sirena cuando el PIR detecta movimiento.", diff: "facil", parts: "5V Relay, HC-SR501, Active Buzzer" },
  ]},
  { name: "IR Receiver & Emitter", qty: "1PC c/u", icon: "🔴", img: "/img/IR-Receiver.png", cat: "comunicacion", demo: "/demos/ir-test", projects: [
    { title: "Control remoto universal", desc: "Decodifica señales de cualquier control IR y activa módulos.", diff: "facil", parts: "IR Receiver, Remote Control, LED" },
    { title: "Barrera de seguridad", desc: "Detecta cuando algo interrumpe el haz infrarrojo.", diff: "facil", parts: "IR Receiver, IR Emitter" },
    { title: "Control de TV desde ESP-32", desc: "Envía comandos IR a tu televisor programáticamente.", diff: "medio", parts: "IR Emitter, ESP-32" },
  ]},
  { name: "Joystick Module", qty: "1PC", icon: "🕹️", img: "/img/Joystick.png", cat: "control", demo: "/demos/joystick-test", projects: [
    { title: "Control de robot", desc: "Dirige un robot con el joystick: adelante, atrás, giros.", diff: "medio", parts: "Joystick, Motor DC x2, L293D" },
    { title: "Cursor en OLED", desc: "Mueve un cursor o selecciona opciones en el menú de la pantalla OLED.", diff: "facil", parts: "Joystick, OLED" },
    { title: "Mini consola de videojuego", desc: "Juego de laberinto o Snake con joystick y pantalla OLED.", diff: "avanzado", parts: "Joystick, OLED" },
  ]},
  { name: "DHT11 Temp & Humidity", qty: "1PC", icon: "🌡️", img: "/img/DHT11-tem-y-humedad.png", cat: "sensor", demo: "/demos/dht11-test", projects: [
    { title: "Estación meteorológica", desc: "Mide temperatura y humedad y las muestra en la pantalla OLED.", diff: "facil", parts: "DHT11, OLED" },
    { title: "Termostato inteligente", desc: "Activa un ventilador cuando la temperatura supera un umbral.", diff: "medio", parts: "DHT11, Fan Motor, Relay" },
    { title: "Logger de clima IoT", desc: "Sube los datos a internet cada 10 minutos y los grafica.", diff: "avanzado", parts: "DHT11, ESP-32" },
  ]},
  { name: "Ultrasonic Sensor", qty: "1PC", icon: "📏", img: "/img/ultrasonic.png", cat: "sensor", demo: "/demos/ultrasonic-test", projects: [
    { title: "Sensor de estacionamiento", desc: "Avisa con buzzer y LED cuando un coche se acerca demasiado.", diff: "facil", parts: "Ultrasonic, Active Buzzer, LED" },
    { title: "Medidor con display", desc: "Muestra la distancia en cm o pulgadas en la pantalla OLED.", diff: "facil", parts: "Ultrasonic, OLED" },
    { title: "Robot anticolisión", desc: "El robot se detiene o gira al detectar obstáculos.", diff: "medio", parts: "Ultrasonic, Servo, Motor" },
  ]},
  { name: "Fan + Motor 3-6V", qty: "1PC", icon: "🌀", img: "/img/fan-motor-6v.png", cat: "motor", demo: "/demos/fan-motor-test", projects: [
    { title: "Ventilador automático", desc: "Enciende el fan cuando el DHT11 detecta calor excesivo.", diff: "facil", parts: "Fan Motor, DHT11, Transistor PN2222" },
    { title: "Anemómetro casero", desc: "Mide velocidad del viento contando las revoluciones del fan.", diff: "medio", parts: "Fan Motor, IR sensor" },
  ]},
  { name: "Active & Passive Buzzer", qty: "1PC c/u", icon: "🔊", img: "/img/Active-buzzer.png", cat: "audio", demo: "/demos/buzzer-test", projects: [
    { title: "Piano de 4 teclas", desc: "Cada botón toca una nota diferente con el buzzer pasivo.", diff: "facil", parts: "Passive Buzzer, Button 5PCS" },
    { title: "Alarma de temperatura", desc: "Suena cuando el DHT11 detecta temperatura peligrosa.", diff: "facil", parts: "Active Buzzer, DHT11" },
    { title: "Reproductor de melodías", desc: "Toca canciones como Star Wars o Mario con el buzzer pasivo.", diff: "medio", parts: "Passive Buzzer" },
  ]},
  { name: "74HC595 + L293D ICs", qty: "1PC c/u", icon: "🔲", img: "/img/74HC595IC.png", cat: "electronica", demo: "/demos/74hc595-l293d-test", projects: [
    { title: "Control de motor DC", desc: "Usa el L293D para controlar velocidad y dirección de un motor.", diff: "medio", parts: "L293D, Fan Motor, Potenciómetro" },
    { title: "Expansión de salidas", desc: "Controla 8 LEDs con solo 3 pines usando el 74HC595.", diff: "medio", parts: "74HC595, LED 25PCS" },
  ]},
  { name: "RC522 RFID Module", qty: "1PC", icon: "💳", img: "/img/RC522-RFID.png", cat: "comunicacion", demo: "/demos/rc522-rfid-test", projects: [
    { title: "Control de acceso", desc: "Abre una cerradura solo con tu tarjeta RFID autorizada.", diff: "medio", parts: "RC522, Servo SG90, Relay" },
    { title: "Registro de asistencia", desc: "Registra quién pasó y a qué hora usando RFID.", diff: "avanzado", parts: "RC522, ESP-32, OLED" },
    { title: "Alcancía inteligente", desc: "Muestra cuánto has ahorrado al pasar una moneda con RFID.", diff: "facil", parts: "RC522, OLED" },
  ]},
  { name: "HC-SR501 PIR Sensor", qty: "1PC", icon: "👁️", img: "/img/HC-sr501-PIR.png", cat: "sensor", demo: "/demos/pir-test", projects: [
    { title: "Alarma con LED RGB y Buzzer", desc: "En reposo el LED RGB está verde; al detectar movimiento pasa a rojo y el buzzer hace un solo \"pip\" corto.", diff: "facil", parts: "HC-SR501, RGB LED, Active Buzzer", link: "/proyectos-pir/1-alarma_rgb_buzzer" },
    { title: "Luz automática", desc: "Enciende LEDs cuando detecta movimiento.", diff: "facil", parts: "HC-SR501, LED, Relay" },
    { title: "Alarma de seguridad", desc: "Activa buzzer y envía notificación al celular al detectar movimiento.", diff: "medio", parts: "HC-SR501, ESP-32, Active Buzzer" },
    { title: "Contador de personas", desc: "Cuenta cuántas veces alguien pasa frente al sensor.", diff: "medio", parts: "HC-SR501, OLED" },
  ]},
  { name: "Membrane Switch Keypad", qty: "1PC", icon: "⌨️", img: "/img/number-membrana.png", cat: "control", projects: [
    { title: "Caja fuerte con PIN", desc: "Introduce un PIN de 4 dígitos para abrir la cerradura.", diff: "medio", parts: "Keypad, Servo SG90, OLED" },
    { title: "Calculadora", desc: "Calculadora básica con teclado matricial y display OLED.", diff: "medio", parts: "Keypad, OLED", link: "/proyectos-oled/5-calculadora" },
    { title: "Sistema de menú", desc: "Navega entre opciones de configuración con el teclado.", diff: "facil", parts: "Keypad, OLED" },
  ]},
  { name: "Button + Potenciómetro", qty: "5 / 1PC", icon: "🔘", img: "/img/boton.png", cat: "control", projects: [
    { title: "Control de brillo LED", desc: "El potenciómetro ajusta el brillo de un LED con PWM.", diff: "facil", parts: "Potenciómetro, LED, Resistor" },
    { title: "Selección de modos", desc: "Los botones seleccionan diferentes efectos de luz o melodías.", diff: "facil", parts: "Buttons, LED, Passive Buzzer" },
  ]},
  { name: "7-Segment Display", qty: "1 + 4 dígitos", icon: "🔢", img: "/img/4digit-7segmentos.png", cat: "display", projects: [
    { title: "Demo Contador + Reloj OLED", desc: "Cuenta del 0 al 9 en el display de 1 dígito, parpadea el punto decimal y luego muestra un reloj con temperatura en pantalla OLED.", diff: "medio", parts: "7-Seg Display, OLED, DHT11", link: "/proyectos-7seg/1-demo_contador" },
    { title: "Contador 0-9999", desc: "Cada botón incrementa el contador en el display de 4 dígitos.", diff: "facil", parts: "4-Digit Display, Button, 74HC595" },
    { title: "Cronómetro", desc: "Inicia, pausa y reinicia un cronómetro con botones.", diff: "medio", parts: "4-Digit Display, Button" },
    { title: "Dado electrónico", desc: "Al presionar el botón muestra un número aleatorio del 1 al 6.", diff: "facil", parts: "1-Digit Display, Button" },
  ]},
  { name: "Tilt Ball Switch", qty: "1PC", icon: "⚖️", img: "/img/tilt-ball.png", cat: "sensor", projects: [
    { title: "Detector de caída", desc: "Activa una alarma cuando se inclina el dispositivo.", diff: "facil", parts: "Tilt Switch, Active Buzzer" },
    { title: "Nivel de burbuja digital", desc: "Indica si un objeto está nivelado con LEDs verde y rojo.", diff: "facil", parts: "Tilt Switch, LED, Resistor" },
  ]},
  { name: "Remote Control", qty: "1PC", icon: "📺", img: "/img/remote-control.png", cat: "comunicacion", projects: [
    { title: "Control de LEDs RGB", desc: "Cambia colores del LED RGB con los botones del control remoto.", diff: "facil", parts: "Remote, IR Receiver, RGB LED" },
    { title: "Robot teledirigido", desc: "Controla un robot de 4 ruedas con el control IR.", diff: "avanzado", parts: "Remote, IR Receiver, L293D, Motor" },
  ]},
  { name: "Resistores, LEDs, RGB", qty: "120 / 25 / 2PCS", icon: "💡", img: "/img/leds.png", cat: "electronica", projects: [
    { title: "Semáforo programable", desc: "LEDs rojo, amarillo y verde simulan un semáforo real.", diff: "facil", parts: "LED, Resistor, Protoboard" },
    { title: "Efecto de fuego RGB", desc: "El LED RGB parpadea en tonos naranja/rojo simulando fuego.", diff: "facil", parts: "RGB LED, Resistor" },
    { title: "Visualizador de audio", desc: "LEDs que reaccionan al sonido del micrófono.", diff: "medio", parts: "LED 25PCS, Transistor, Resistor" },
  ]},
  { name: "Photoresistor (LDR)", qty: "2PCS", icon: "☀️", img: "/img/Photoresistor.png", cat: "sensor", projects: [
    { title: "Luz nocturna automática", desc: "Se enciende cuando oscurece y se apaga con la luz del día.", diff: "facil", parts: "LDR, LED, Transistor PN2222, Resistor" },
    { title: "Seguidor de luz solar", desc: "Dos LDRs orientan un servo hacia la fuente de luz.", diff: "medio", parts: "LDR x2, Servo SG90" },
  ]},
  { name: "NPN Transistor PN2222", qty: "2PCS", icon: "🔁", img: "/img/NPN-Transistor-PN2222.png", cat: "electronica", projects: [
    { title: "Switch para motor", desc: "Usa el transistor para controlar el motor con poca corriente.", diff: "medio", parts: "PN2222, Fan Motor, Resistor" },
    { title: "Oscilador de audio", desc: "Circuito astable que genera tonos sin microcontrolador.", diff: "avanzado", parts: "PN2222 x2, Resistor, Passive Buzzer" },
  ]},
  { name: "Thermistor", qty: "1PC", icon: "🌡️", img: "/img/Thermistor.png", cat: "sensor", projects: [
    { title: "Termómetro analógico", desc: "Lee la temperatura mediante variación de resistencia.", diff: "facil", parts: "Thermistor, OLED, Resistor" },
  ]},
  { name: "9V Battery + Clip", qty: "1PC", icon: "🔋", img: "/img/9V-Battery-connector-clip.png", cat: "electronica", projects: [
    { title: "Alimentación portátil", desc: "Alimenta tus proyectos sin necesidad de cable USB.", diff: "facil", parts: "9V Battery, Power Module" },
  ]},
];
