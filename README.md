# Visão 360 - Carro Robótico Telecomandado

**Autor:** Gil Lopes Tavares  
**Curso:** Técnico Profissional de Informática - Escola Profissional António do Lago Cerqueira (EPALC)  
**Ano:** 2025/2026  

Sobre o Projeto
O **Visão 360** (também designado "Carro Telecomandado via Web") é um Veículo Terrestre Não Tripulado (UGV) de baixo custo desenvolvido no âmbito da Prova de Aptidão Profissional (PAP). O seu foco é a exploração, monitorização remota e manipulação de objetos em espaços confinados ou de difícil acesso.

O sistema baseia-se numa arquitetura de processamento duplo:
- **Arduino Uno R3:** Responsável pela locomoção, controlo dos motores e gestão da segurança com sensores.
- **ESP32-CAM:** Responsável por criar uma rede Wi-Fi própria (Access Point) e alojar um servidor web com uma interface de controlo virtual baseada num comando PS2.

Funcionalidades Principais
- **Locomoção Omnidirecional (Holonómica):** Utilização de rodas Mecanum que permitem ao robô deslocar-se em qualquer direção (frente, trás, lateralmente, diagonal) e rodar sobre o próprio eixo.
- **Manipulação Robótica:** Integração de um braço robótico equipado com servomotores para interação com o ambiente (apanhar e transportar objetos).
- **Interface de Controlo Web:** Uma página Web (com HTML, CSS e JavaScript integrados no ESP32) acessível via navegador para controlar o robô sem necessidade de instalar aplicações adicionais.
- **Segurança Ativa (Anticolisão):** Monitorização em tempo real com um sensor ultrassónico (HC-SR04). Se um obstáculo for detetado a menos de 20 cm, o robô para os motores e recua automaticamente, ignorando comandos de avanço do utilizador.
- **Failsafe (Watchdog):** O Arduino imobiliza o veículo se perder a comunicação serial com o ESP32-CAM por mais de 1 segundo.

Hardware Utilizado
- **Microcontroladores:** Arduino UNO R3 e Módulo ESP32-CAM-MB
- **Movimentação:** 4x Motores DC com Caixa de Redução + 4x Rodas Mecanum
- **Drivers de Motor:** Módulo Ponte H (para controlo independente de velocidade e direção)
- **Sensores:** Sensor Ultrassónico HC-SR04
- **Atuadores:** Servomotores Tower Pro MG995 (Base e Garra do braço)
- **Alimentação:** Baterias independentes para alimentação de potência e lógica.

Software e Tecnologias
- **Linguagem:** C/C++ (para Arduino e ESP32), HTML/CSS/JS (Front-End)
- **Ambientes de Desenvolvimento:** Arduino IDE e Visual Studio Code
- **Bibliotecas Principais:** - `<SoftwareSerial.h>` (Comunicação UART entre placas)
  - `<Servo.h>` (Controlo do braço robótico)
  - `<WiFi.h>` e `<WebServer.h>` (Criação do Ponto de Acesso e Servidor HTTP)

Comunicação
O sistema utiliza comunicação assíncrona (UART) a 9600 bps entre o ESP32-CAM (TX: Pino 14, RX: Pino 16) e o Arduino Uno (RX: Pino 2, TX: Pino 3). O utilizador envia comandos através do painel na Web, que o ESP32 transmite via porta serial virtual para o Arduino executar as ações nos motores e servos.
