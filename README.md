# SIMAPD IOT

## 🚀 Sobre o Projeto

O SIMAPD IOT implementa um **sistema IoT integrado** para monitoramento em tempo real de condições ambientais que podem indicar situações de emergência ou desastres urbanos. A solução combina múltiplos sensores, comunicação MQTT, gateway inteligente e dashboard web para fornecer alertas precoces e monitoramento contínuo.

## 👥 Equipe de Desenvolvimento

| Nome | RM | E-mail | GitHub | LinkedIn |
|------|-------|---------|---------|----------|
| Arthur Vieira Mariano | RM554742 | arthvm@proton.me | [@arthvm](https://github.com/arthvm) | [arthvm](https://linkedin.com/in/arthvm/) |
| Guilherme Henrique Maggiorini | RM554745 | guimaggiorini@gmail.com | [@guimaggiorini](https://github.com/guimaggiorini) | [guimaggiorini](https://linkedin.com/in/guimaggiorini/) |
| Ian Rossato Braga | RM554989 | ian007953@gmail.com | [@iannrb](https://github.com/iannrb) | [ianrossato](https://linkedin.com/in/ianrossato/) |

### 🎯 Aplicação Prática

O sistema foi desenvolvido para **cenários de emergência urbana**, sendo especialmente útil em:

- **Monitoramento Sísmico**: Detecção precoce de tremores e movimentações do solo
- **Alertas de Enchentes**: Monitoramento de chuva e umidade do solo para prevenção de alagamentos
- **Gestão de Riscos**: Sistema de classificação automática de níveis de risco (LOW/MEDIUM/HIGH/CRITICAL)
- **Resposta Rápida**: Notificações em tempo real para equipes de emergência

## 🏗️ Arquitetura do Sistema

```
┌─────────────────┐    MQTT     ┌──────────────┐    HTTP    ┌─────────────────┐
│   ESP32 + 3     │ ──────────► │   Node-RED   │ ─────────► │   Dashboard     │
│   Sensores      │             │   Gateway    │            │   Web (/ui)     │
└─────────────────┘             └──────────────┘            └─────────────────┘
        │                              │
        │ HTTP/JSON                    │ MQTT Broker
        ▼                              ▼
┌─────────────────┐             ┌──────────────┐
│ API Validação   │             │  Histórico   │
│   Sensores      │             │    Dados     │
└─────────────────┘             └──────────────┘
```

## 🔧 Componentes Implementados

### 📱 **Hardware (ESP32)**
- **Plataforma**: ESP32 DevKit C v4
- **Sensor de Movimento**: MPU6050 (acelerômetro + giroscópio)
- **Sensor de Chuva**: Chip customizado para detecção de precipitação
- **Sensor de Umidade**: Chip customizado para monitoramento do solo

### 🌐 **Comunicação**
- **Protocolo MQTT**: Comunicação principal entre dispositivos
- **HTTP/JSON**: Validação de sensores e APIs REST
- **Tópicos MQTT**:
  - `iot/measurement/movement` - Dados de movimento sísmico
  - `iot/measurement/rain` - Níveis de precipitação
  - `iot/measurement/moisture` - Umidade do solo

### 🖥️ **Gateway (Node-RED)**
- **Broker MQTT**: Gerenciamento de mensagens
- **Processamento**: Análise e classificação de riscos
- **API REST**: Endpoints para validação e consultas
- **Dashboard Web**: Interface de monitoramento em tempo real

## 🚀 Instruções de Configuração e Execução

### **Pré-requisitos**
- Node.js (v14 ou superior)
- Node-RED instalado
- Acesso ao simulador Wokwi ou hardware ESP32 real

### **1. Configuração do Gateway (Node-RED)**

```bash
# Instalar Node-RED (se não instalado)
npm install -g node-red

# Instalar dependências do dashboard
npm install node-red-dashboard node-red-contrib-mqtt-broker

# Iniciar Node-RED
node-red
```

#### **Importar Fluxos do Node-RED**
1. Acesse `http://localhost:1880`
2. Menu ☰ → Import → Clipboard
3. Cole o conteúdo do arquivo `gateway/flows.json`
4. Clique em "Import"

#### **Configuração do Broker MQTT (Caso indisponível, use localhost)**
- **Host**: `172.190.255.213`
- **Porta**: `1883`
- **Cliente ID**: Gerado automaticamente
- **Tópicos configurados automaticamente**

### **2. Configuração dos Sensores (ESP32)**

#### **Opção A: Simulação no Wokwi**
1. Acesse [wokwi.com](https://wokwi.com)
2. Crie um novo projeto ESP32
3. Copie o conteúdo dos arquivos da pasta `sensors/`:
   - `sketch.ino` → Código principal
   - `diagram.json` → Configuração dos componentes
   - `libraries.txt` → Bibliotecas necessárias
   - `*.chip.json` e `*.chip.c` → Sensores customizados (Sensore reais não estao presentes no Wokwi)

#### **Opção B: Hardware Real**
1. **Conexões**:
   ```
   ESP32          MPU6050
   ───────────────────────
   3.3V     →     VCC
   GND      →     GND
   21       →     SDA
   22       →     SCL
   
   ESP32          Sensores Analógicos
   ─────────────────────────────────
   35       →     Sensor de Chuva
   32       →     Sensor de Umidade
   ```

2. **Upload do Código**:
   ```bash
   # Instalar Arduino IDE ou PlatformIO
   # Configurar bibliotecas:
   # - Adafruit MPU6050
   # - ArduinoJson
   # - PubSubClient
   ```

### **3. Configuração de Rede**

#### **Parâmetros de Conexão no ESP32**:
```cpp
// WiFi Configuration
const char* ssid = "SEU_WIFI";
const char* password = "SUA_SENHA";

// MQTT Configuration  
const char* mqtt_server = "IP_DO_BROKER";
const int mqtt_port = 1883;

// IDs únicos do sistema
const String AREA_ID = "b138ns1z29jrop4q35o51cao";
const String SENSOR_ID = "j6nmu6qwjgo6h7k29ababv02";
```

### **4. Acesso ao Dashboard**

1. **URL do Dashboard**: `http://localhost:1880/ui`
2. **Funcionalidades disponíveis**:
   - Monitoramento em tempo real dos 3 sensores
   - Gráficos históricos de dados
   - Sistema de alertas visuais
   - Classificação de níveis de risco
   - Status de conectividade dos dispositivos

## 📊 Detalhamento dos Fluxos Node-RED

### **Fluxo 1: Recepção de Dados MQTT**
- **Entrada**: Tópicos MQTT dos sensores
- **Processamento**: Parsing JSON e validação de dados
- **Saída**: Dados estruturados para dashboard

### **Fluxo 2: Sistema de Alertas**
```javascript
// Processamento de Alertas
const riskLevel = msg.payload.riskLevel;
const sensorType = msg.payload.type;

if (riskLevel === 'HIGH' || riskLevel === 'CRITICAL') {
    // Gerar alerta imediato
    // Armazenar no histórico
    // Notificar dashboard
}
```

### **Fluxo 3: API de Validação**
- **Endpoint**: `GET /sensors/validate`
- **Parâmetros**: `areaId`, `sensorId`
- **Função**: Autenticar sensores antes de aceitar dados

### **Fluxo 4: Dashboard Web**
- **Widgets de tempo real**: Exibição instantânea dos dados
- **Gráficos históricos**: Tendências e padrões
- **Sistema de cores**: Verde (LOW) → Amarelo (MEDIUM) → Laranja (HIGH) → Vermelho (CRITICAL)

## 📖 Código-Fonte Detalhado

### **Estrutura do Projeto**
```
iot/
├── sensors/                 # Código ESP32 e configurações Wokwi
│   ├── sketch.ino          # Código principal do microcontrolador
│   ├── diagram.json        # Esquema de conexões Wokwi
│   ├── libraries.txt       # Dependências Arduino
│   ├── rain-sensor.chip.*  # Sensor de chuva customizado
│   └── soil-sensor.chip.*  # Sensor de umidade customizado
├── gateway/                # Configuração Node-RED
│   └── flows.json         # Fluxos completos do gateway
└── README.md              # Documentação (este arquivo)
```

### **Principais Funcionalidades do Código ESP32**

#### **1. Sistema de Classificação de Riscos**
```cpp
enum RiskLevel {
  RISK_LOW,      // Condições normais
  RISK_MEDIUM,   // Atenção necessária  
  RISK_HIGH,     // Situação de alerta
  RISK_CRITICAL  // Emergência imediata
};
```

#### **2. Validação de Sensores**
```cpp
bool validateSensor() {
  // Autenticação via API REST
  // Verificação de AREA_ID e SENSOR_ID
  // Prevenção de dados não autorizados
}
```

#### **3. Detecção Inteligente**
```cpp
void detectMovement() {
  // Análise de aceleração e giroscópio
  // Filtros de ruído e falsos positivos
  // Classificação automática de intensidade
}
```

## 🔒 Segurança e Validação

- **Autenticação**: Todos os sensores devem ser validados antes do envio de dados
- **Tópicos únicos**: Sistema de identificação exclusiva por área/sensor
- **Validação de dados**: Verificação de integridade das mensagens JSON
- **Timeout de conexão**: Reconexão automática em caso de falha

## 📈 Monitoramento e Alertas

### **Níveis de Risco por Sensor**

| Sensor | LOW | MEDIUM | HIGH | CRITICAL |
|--------|-----|--------|------|----------|
| **Movimento** | < 0.01g | 0.01-0.03g | 0.03-0.1g | > 0.1g |
| **Chuva** | 150-400 | 400-700 | 700-900 | > 900 |
| **Umidade** | 150-400 | 400-700 | 700-900 | > 900 |

### **Sistema de Alertas**
- **Alertas visuais**: Mudança de cores no dashboard
- **Histórico**: Armazenamento dos últimos 50 alertas
- **Timestamp**: Registro preciso de data/hora
- **Persistência**: Dados mantidos em contexto global

## 🛠️ Resolução de Problemas

### **Problemas Comuns**

1. **ESP32 não conecta ao WiFi**
   - Verificar SSID e senha
   - Confirmar alcance do sinal

2. **Sensor não validado**
   - Verificar AREA_ID e SENSOR_ID
   - Confirmar conectividade com API

3. **Dashboard não carrega**
   - Verificar se Node-RED está executando
   - Acessar `http://localhost:1880/ui`

4. **Dados não aparecem no MQTT**
   - Verificar configuração do broker
   - Confirmar tópicos MQTT

## 🌟 Funcionalidades Avançadas

- **Reconexão automática**: WiFi e MQTT com retry inteligente
- **Buffer de dados**: Prevenção de perda de informações
- **API RESTful**: Integração com sistemas externos
- **Processamento local**: Análise de dados no ESP32

## 👥 Aplicações Futuras

- **Integração com sistemas municipais de emergência**
- **Expansão para redes de sensores distribuídos com LoRaWAN**
- **Machine Learning para previsão de eventos**
- **Aplicativo móvel para notificações push**

Este projeto foi desenvolvido para fins acadêmicos como parte do Global Solution da FIAP - DISRUPTIVE ARCHITECTURES: IOT, IOB & GENERATIVE IA# SIMAPD IOT