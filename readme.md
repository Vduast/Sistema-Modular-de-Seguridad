# 🛡️ Niebla Seguridad — Sistema Modular de Seguridad e IoT en Linux Embebido

![Python](https://img.shields.io/badge/Python-3.9%2B-blue?logo=python&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-ESP32--CAM-00599C?logo=c%2B%2B&logoColor=white)
![Raspberry Pi](https://img.shields.io/badge/Hardware-Raspberry%20Pi%20%7C%20ESP32-C51A4A?logo=raspberry-pi&logoColor=white)
![Linux](https://img.shields.io/badge/OS-Embedded%20Linux%20(systemd)-FCC624?logo=linux&logoColor=black)
![Blynk](https://img.shields.io/badge/IoT-Blynk%20Cloud-23C48E)

**Niebla Seguridad** es una arquitectura modular de seguridad física y monitoreo IoT de bajo consumo, diseñada para operar de forma autónoma (*headless*) sobre **Linux Embebido (Raspberry Pi)** en conjunto con nodos de visión basados en microcontroladores **ESP32-CAM** y telemetría en tiempo real mediante **Blynk IoT**.

---

## 📋 Características Principales

* **Arquitectura Modular en Python:** Separación limpia de responsabilidades entre el orquestador central (`main.py`), adquisición de señales físicas (`sensores.py`) e interfaz local (`pantalla.py`).
* **Monitoreo de Sensores por Hardware (GPIO):** Lectura en tiempo real de sensores magnéticos de apertura (Reed Switches) y periféricos de intrusión con filtrado de rebotes (*debouncing*).
* **Nodos de Video con ESP32-CAM (`C++ / Arduino`):** Firmware dedicado (`ESP32camSEGURIDAD.ino`) para transmisión de video MJPEG en tiempo real y captura de eventos mediante Wi-Fi.
* **Telemetría Local e Interfaz LCD:** Visualización del estado del sistema, dirección IP, alertas activas y diagnóstico en pantalla física controlada vía protocolo serial $I^2C$ / GPIO (`pantalla.py`).
* **Integración Cloud & Alertas Móviles (Blynk):** Sincronización bidireccional con la plataforma Blynk para armado/desarmado remoto, notificaciones *push* instantáneas ante intrusiones y monitoreo de estado.
* **Ejecución Demonizada (`systemd`) y Acceso Seguro:** Configurado para iniciar automáticamente como servicio de segundo plano en Linux con reconexión automática y soporte para exposición segura vía **Cloudflare Tunnels**.

---

## 📂 Estructura del Proyecto

```text
Niebla seguridad/
├── main.py                      # Orquestador principal: máquina de estados, bucle de eventos y conexión Blynk
├── sensores.py                  # Módulo de abstracción GPIO para lectura de sensores magnéticos y alarmas
├── pantalla.py                  # Controlador de interfaz visual/LCD para telemetría y estado local
├── blynk.txt                    # Configuración de credenciales, tokens de autenticación y Virtual Pins de Blynk
├── requirements.txt             # Dependencias de Python (RPi.GPIO / gpiozero, blynk-library, smbus, etc.)
└── ESP32camSEGURIDAD/
    └── ESP32camSEGURIDAD.ino    # Firmware en C++ para el microcontrolador ESP32-CAM (servidor de streaming)
```

---

## 🛠️ Arquitectura de Hardware y Componentes

| Componente | Rol en el Sistema | Protocolo / Interfaz |
| :--- | :--- | :--- |
| **Raspberry Pi (OS Lite)** | Nodo central de procesamiento y servidor de control Linux | GPIO / Wi-Fi / SSH |
| **ESP32-CAM (AI-Thinker)** | Cámaras IP de vigilancia y streaming de video en tiempo real | Wi-Fi (HTTP MJPEG Stream) |
| **Sensores Magnéticos (Reed)** | Detección de apertura en puertas y ventanas | GPIO Digital (Pull-Up interno) |
| **Pantalla LCD / Display** | Monitoreo de estado local en sitio (Armado, Desarmado, Alerta) | Bus $I^2C$ (`SDA` / `SCL`) |

---

## 🚀 Instalación y Despliegue

### 1. Nodo Central (Raspberry Pi - Linux)

Clonar el repositorio e instalar las dependencias dentro de un entorno virtual:

```bash
git clone https://github.com/Vduast/Niebla-Seguridad.git
cd "Niebla seguridad"
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

Configurar el token de autenticación de **Blynk** verificando el archivo `blynk.txt` (o exportando la variable de entorno correspondiente) y ejecutar el orquestador:

```bash
python3 main.py
```

### 2. Ejecución como Servicio del Sistema (`systemd`)

Para garantizar operación continua 24/7 tras cortes de energía:

```ini
[Unit]
Description=Servicio Modular de Seguridad IoT - Niebla Seguridad
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
WorkingDirectory=/home/pi/Niebla seguridad
ExecStart=/home/pi/Niebla seguridad/venv/bin/python main.py
Restart=always
RestartSec=5
User=pi

[Install]
WantedBy=multi-user.target
```

### 3. Nodo de Cámara (ESP32-CAM)

1. Abrir `ESP32camSEGURIDAD/ESP32camSEGURIDAD.ino` en **Arduino IDE** o **PlatformIO**.
2. Seleccionar la placa `AI Thinker ESP32-CAM`.
3. Configurar las credenciales de red (`SSID` y `PASSWORD`) y cargar el firmware mediante un adaptador FTDI USB-Serial (conectando `IO0` a `GND` durante el flasheo).

---

## 🌐 English Summary (For Technical Recruiters)

**Niebla Seguridad** is a modular Embedded Linux & IoT physical security system written in **Python** and **C++**.
* **Modular Design:** Decoupled hardware abstraction layers for sensor polling (`sensores.py`), local display telemetry (`pantalla.py`), and core event orchestration (`main.py`).
* **Embedded C++ Firmware:** Custom `ESP32-CAM` firmware (`ESP32camSEGURIDAD.ino`) providing low-latency video streaming.
* **Cloud & Mobile Telemetry:** Integrated with **Blynk IoT** for remote arming/disarming and instant intrusion notifications, running as a resilient Linux `systemd` background daemon.

---

## 👨‍💻 Autor

**Ricardo Anael Castillo Valenzuela**
*Ingeniería en Software (UAdeO) & Técnico en Electrónica*
* 📧 Contacto: Anaelcastillovzla@gmail.com
* 🔗 GitHub: [@Vduast](https://github.com/Vduast)