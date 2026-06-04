// Arduino UNO R3 - Receptor do ESP32-CAM (Comando PS2 Web)
// Autor: Gil Tavares
// Data: 2026-05-26
//
// Descrição:
// Recebe comandos estruturados via SoftwareSerial (RX=2, TX=3) enviados pela
// ESP32-CAM. Processa movimentos com regulação de velocidade por PWM e controlo
// de servos nas portas A0 e A1. Implementa proteção ativa contra obstáculos via
// sensor ultrassónico HC-SR04 com evasão automática.

#include <Servo.h>
#include <SoftwareSerial.h>

// Comunicação com ESP32-CAM
SoftwareSerial espSerial(2, 3); // RX=2, TX=3

// --- Pinos Motor Driver L298N - Lado Direito
#define Motor_Dir_Frente_A1 6
#define Motor_Dir_Frente_A2 7
#define Motor_Dir_Tras_B1 5
#define Motor_Dir_Tras_B2 4

// --- Pinos Motor Driver L298N - Lado Esquerdo
#define Motor_Esq_Frente_A1 9
#define Motor_Esq_Frente_A2 8
#define Motor_Esq_Tras_B1 11
#define Motor_Esq_Tras_B2 10

// --- Sensor Ultrassônico HC-SR04
#define TriggerPin A5
#define EchoPin A4

// --- Pinos Físicos dos Servos
#define PINO_SERVO_BASE A0
#define PINO_SERVO_GARRA A1

// --- LED de Estado
#define led_status 13

// --- Velocidades de Operação (PWM)
#define vel_Normal 180
#define vel_giro 150
#define vel_turbo 255
#define vel_lento 110

// --- Objetos de Controlo dos Servos
Servo servoBase;
Servo servoGarra;

// --- Variáveis Globais
int velocidade = vel_Normal;
int val_atual_do_servo_base = 90;  // Inicia a 90 graus (centro)
int val_atual_do_servo_garra = 90; // Inicia a 90 graus (meio curso)

bool obstaculoAtivo = false;
bool indoParaFrente = false; // Indica se o carro está em marcha em frente
bool carroMovendo = false;   // Indica se o carro está em movimento

// Temporizador para Failsafe (Watchdog)
unsigned long lastCommandTime = 0;
const unsigned long WATCHDOG_TIMEOUT =
    1000; // Parar se perder ligação por mais de 1000ms

void setup() {
  // Configuração da porta de depuração USB
  Serial.begin(115200);
  Serial.println(F("=== Arduino Uno R3 Pronto ==="));

  // Inicialização da SoftwareSerial com o ESP32-CAM
  espSerial.begin(9600);
  espSerial.setTimeout(50);

  // Configuração dos pinos do Driver L298N
  pinMode(Motor_Dir_Frente_A1, OUTPUT);
  pinMode(Motor_Dir_Frente_A2, OUTPUT);
  pinMode(Motor_Dir_Tras_B1, OUTPUT);
  pinMode(Motor_Dir_Tras_B2, OUTPUT);
  pinMode(Motor_Esq_Frente_A1, OUTPUT);
  pinMode(Motor_Esq_Frente_A2, OUTPUT);
  pinMode(Motor_Esq_Tras_B1, OUTPUT);
  pinMode(Motor_Esq_Tras_B2, OUTPUT);

  // Garante arranque em paragem
  parar();

  // Configuração do sensor HC-SR04
  pinMode(TriggerPin, OUTPUT);
  pinMode(EchoPin, INPUT);
  digitalWrite(TriggerPin, LOW);

  // Associação física dos Servos (corrige erro de compilação)
  servoBase.attach(PINO_SERVO_BASE);
  servoGarra.attach(PINO_SERVO_GARRA);

  // Configura posições iniciais estáveis
  servoBase.write(val_atual_do_servo_base);
  servoGarra.write(val_atual_do_servo_garra);

  // LED de Estado
  pinMode(led_status, OUTPUT);
  digitalWrite(led_status, LOW);

  lastCommandTime = millis();
}

void loop() {
  // 1. Escuta de novos comandos serial
  if (espSerial.available() > 0) {
    String comando = espSerial.readStringUntil('\n');
    comando.trim();

    if (comando.length() > 0) {
      Serial.print("Comando recebido: ");
      Serial.println(comando);
      lastCommandTime = millis(); // Reinicia watchdog
      processarComando(comando);
    }
  }

  // 2. Failsafe (Caso o sinal Wi-Fi caia ou a página seja fechada durante o
  // movimento)
  if (millis() - lastCommandTime > WATCHDOG_TIMEOUT) {
    if (carroMovendo) {
      parar();
      indoParaFrente = false;
    }
  }

  // 3. Monitorização de Obstáculos (apenas quando o robô marcha para a frente)
  if (indoParaFrente) {
    AvistarObstaculo();
    if (obstaculoAtivo) {
      Serial.println(F("Alerta: Obstáculo à frente! A desviar..."));
      espSerial.println(F("OBSTACULO"));

      // Para as rodas
      parar();

      // Recua por 300 ms de forma autónoma
      tras(velocidade);
      delay(300);

      // Imobiliza o veículo
      parar();
      indoParaFrente = false;
      lastCommandTime = millis(); // Evita ativar o watchdog devido ao delay
    }
  }
}

// Interpolação e tratamento dos comandos do ESP32-CAM
void processarComando(String comando) {

  comando.toUpperCase();

  // --- Controlo de Velocidade ---
  if (comando == "TURBO") {

    velocidade = vel_turbo;
    espSerial.println(F("VEL: TURBO"));

  } else if (comando == "LENTO") {

    velocidade = vel_lento;
    espSerial.println(F("VEL: LENTO"));

  } else if (comando == "NORMAL") {

    velocidade = vel_Normal;
    espSerial.println(F("VEL: NORMAL"));

  }

  // --- Controlo de Movimento ---
  else if (comando == "FRENTE") {

    // Verifica primeiro se tem espaço livre
    AvistarObstaculo();

    if (!obstaculoAtivo) {

      frente(velocidade);
      indoParaFrente = true;
      espSerial.println(F("FRENTE"));

    } else {

      Serial.println(F("Bloqueado: Obstáculo imediato!"));
      espSerial.println(F("BLOQUEADO"));
      parar();
      tras(velocidade);
      delay(300);
      parar();
      indoParaFrente = false;
    }

  } else if (comando == "TRAS") {

    indoParaFrente = false;
    tras(velocidade);
    espSerial.println(F("TRAS"));

  } else if (comando == "ESQ") {

    indoParaFrente = false;
    esquerda(vel_giro);
    espSerial.println(F("ESQ"));

  } else if (comando == "DIR") {

    indoParaFrente = false;
    direita(vel_giro);
    espSerial.println(F("DIR"));

  } else if (comando == "PARE") {

    indoParaFrente = false;
    parar();
    espSerial.println(F("PARE"));

  }

  // --- Controlo dos Servos ---
  else if (comando == "BASE_DIR") {

    val_atual_do_servo_base = min(90, val_atual_do_servo_base + 5);
    servoBase.write(val_atual_do_servo_base);
    espSerial.println("BASE: " + String(val_atual_do_servo_base));

  } else if (comando == "BASE_ESQ") {

    val_atual_do_servo_base = max(0, val_atual_do_servo_base - 5);
    servoBase.write(val_atual_do_servo_base);
    espSerial.println("BASE: " + String(val_atual_do_servo_base));

  } else if (comando == "GARRA_ABRE") {

    val_atual_do_servo_garra = min(45, val_atual_do_servo_garra + 5);
    servoGarra.write(val_atual_do_servo_garra);
    espSerial.println("GARRA: " + String(val_atual_do_servo_garra));

  } else if (comando == "GARRA_FECHA") {

    val_atual_do_servo_garra = max(0, val_atual_do_servo_garra - 5);
    servoGarra.write(val_atual_do_servo_garra);
    espSerial.println("GARRA: " + String(val_atual_do_servo_garra));

  }

  // --- Controlo do LED de Iluminação ---
  else if (comando == "LED_ON") {

    digitalWrite(led_status, HIGH);
    espSerial.println(F("LED: ON"));

  } else if (comando == "LED_OFF") {

    digitalWrite(led_status, LOW);
    espSerial.println(F("LED: OFF"));

  }
}

// --- Funções de Direção do Chassi (Motores L298N)
void parar() {

  digitalWrite(Motor_Dir_Frente_A1, LOW);
  digitalWrite(Motor_Dir_Frente_A2, LOW);
  digitalWrite(Motor_Dir_Tras_B1, LOW);
  digitalWrite(Motor_Dir_Tras_B2, LOW);

  digitalWrite(Motor_Esq_Frente_A1, LOW);
  digitalWrite(Motor_Esq_Frente_A2, LOW);
  digitalWrite(Motor_Esq_Tras_B1, LOW);
  digitalWrite(Motor_Esq_Tras_B2, LOW);

  carroMovendo = false;
}

void frente(int vel) {

  digitalWrite(Motor_Dir_Frente_A1, LOW);
  digitalWrite(Motor_Dir_Frente_A2, vel);

  digitalWrite(Motor_Dir_Tras_B1, LOW);
  digitalWrite(Motor_Dir_Tras_B2, vel);

  digitalWrite(Motor_Esq_Frente_A1, vel);
  digitalWrite(Motor_Esq_Frente_A2, LOW);

  digitalWrite(Motor_Esq_Tras_B1, vel);
  digitalWrite(Motor_Esq_Tras_B2, LOW);

  carroMovendo = true;
}

void tras(int vel) {

  digitalWrite(Motor_Dir_Frente_A1, vel);
  digitalWrite(Motor_Dir_Frente_A2, LOW);

  digitalWrite(Motor_Dir_Tras_B1, vel);
  digitalWrite(Motor_Dir_Tras_B2, LOW);

  digitalWrite(Motor_Esq_Frente_A1, LOW);
  digitalWrite(Motor_Esq_Frente_A2, vel);

  digitalWrite(Motor_Esq_Tras_B1, LOW);
  digitalWrite(Motor_Esq_Tras_B2, vel);

  carroMovendo = true;
}

void esquerda(int vel) {

  digitalWrite(Motor_Dir_Frente_A1, LOW);
  digitalWrite(Motor_Dir_Frente_A2, vel);

  digitalWrite(Motor_Dir_Tras_B1, LOW);
  digitalWrite(Motor_Dir_Tras_B2, vel);

  digitalWrite(Motor_Esq_Frente_A1, LOW);
  digitalWrite(Motor_Esq_Frente_A2, vel);

  digitalWrite(Motor_Esq_Tras_B1, LOW);
  digitalWrite(Motor_Esq_Tras_B2, vel);

  carroMovendo = true;
}

void direita(int vel) {

  digitalWrite(Motor_Dir_Frente_A1, vel);
  digitalWrite(Motor_Dir_Frente_A2, LOW);

  digitalWrite(Motor_Dir_Tras_B1, vel);
  digitalWrite(Motor_Dir_Tras_B2, LOW);

  digitalWrite(Motor_Esq_Frente_A1, vel);
  digitalWrite(Motor_Esq_Frente_A2, LOW);

  digitalWrite(Motor_Esq_Tras_B1, vel);
  digitalWrite(Motor_Esq_Tras_B2, LOW);

  carroMovendo = true;
}

// --- Função de Monitorização do Sensor Ultrassónico
void AvistarObstaculo() {

  digitalWrite(TriggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(TriggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(TriggerPin, LOW);

  // pulseIn com timeout de 25ms para evitar congelamentos e reboots do Arduino
  long duration = pulseIn(EchoPin, HIGH, 25000);

  if (duration == 0) {
    obstaculoAtivo = false;
    return;
  }

  long distance = duration * 0.034 / 2;

  // Filtra ruídos ou leituras inválidas de reflexão direta
  if (distance > 1 && distance < 20) {

    obstaculoAtivo = true;

  } else {

    obstaculoAtivo = false;

  }
}