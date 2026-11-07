// ==========================================
// 1. LIBRERÍAS
// ==========================================
#include "esp_camera.h"      // Librería oficial de Espressif para controlar el lente de la cámara
#include <WiFi.h>            // Librería para conectar el chip a internet
#include "esp_http_server.h" // Librería para crear un mini servidor web dentro de la cámara

// ==========================================
// 2. CONFIGURACIÓN DE RED
// ==========================================
const char* ssid = "anael";  // El nombre exacto de la red Wi-Fi a la que te vas a conectar
const char* password = "";   // Al estar vacío, le indica al chip que es una red abierta (sin seguridad)

// ==========================================
// 3. MAPA DE PINES (MODELO AI-THINKER)
// ==========================================
// Aquí le decimos al procesador exactamente con qué cables internos está soldado el lente de la cámara.
// Cada modelo (Wrover, AI-Thinker, etc.) tiene un mapa distinto. Este es el de tu placa.
#define PWDN_GPIO_NUM     32 // Pin de encendido/apagado de la cámara
#define RESET_GPIO_NUM    -1 // -1 significa que no tiene pin de reinicio por software dedicado
#define XCLK_GPIO_NUM      0 // Reloj principal que sincroniza la cámara
#define SIOD_GPIO_NUM     26 // Pin de datos I2C (como el de tu pantalla LCD, pero interno)
#define SIOC_GPIO_NUM     27 // Reloj I2C
// Los pines "Y" son el bus de datos por donde viajan los píxeles de la imagen al procesador
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25 // Sincronización vertical (avisa cuando empieza un cuadro nuevo)
#define HREF_GPIO_NUM     23 // Sincronización horizontal (avisa cuando empieza una línea nueva)
#define PCLK_GPIO_NUM     22 // Reloj de píxeles (avisa cuando un solo punto de color está listo)

// Creamos un "manejador" (handle) vacío para nuestro servidor de video
httpd_handle_t stream_httpd = NULL;

// ==========================================
// 4. LÓGICA DE TRANSMISIÓN DE VIDEO (STREAMING)
// ==========================================
// PART_BOUNDARY es una "etiqueta" invisible que separa una foto de la siguiente
#define PART_BOUNDARY "123456789000000000000987654321"

// Estas líneas son el lenguaje estándar de internet (HTTP). Le dicen al navegador web o a Blynk: 
// "No te voy a mandar una página web, te voy a mandar muchas imágenes JPEG y debes reemplazar 
// la vieja por la nueva inmediatamente" (multipart/x-mixed-replace).
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// Esta es la función que "dispara" las fotos a toda velocidad
esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL; // Un "Frame Buffer", es decir, el espacio en memoria para guardar 1 foto
  esp_err_t res = ESP_OK;  // Variable para saber si todo va bien
  size_t _jpg_buf_len = 0; // Tamaño de la foto
  uint8_t * _jpg_buf = NULL; // Los datos crudos de la foto
  char * part_buf[64];

  // Avisamos al que se conecte el tipo de archivo que le vamos a mandar
  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) return res;

  // Ciclo infinito: Mientras el celular esté conectado viendo el video, haz esto:
  while (true) {
    fb = esp_camera_fb_get(); // ¡CLICK! Tomamos una foto de la cámara
    if (!fb) {
      Serial.println("Fallo al capturar frame");
      res = ESP_FAIL;
    } else {
      _jpg_buf_len = fb->len; // Guardamos cuánto pesa la foto
      _jpg_buf = fb->buf;     // Guardamos la foto en sí
    }
    
    // Si la foto se tomó bien, armamos el "paquete" de envío
    if (res == ESP_OK) {
      // 1. Mandamos la cabecera (aviso de "ahí te va una imagen y pesa tanto")
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK) {
      // 2. Mandamos la foto real (los píxeles comprimidos en JPEG)
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK) {
      // 3. Mandamos la línea separadora "BOUNDARY" para avisar que la foto terminó
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    
    // Vaciamos la memoria. Si no hacemos esto, la cámara se satura en 1 segundo y colapsa.
    if (fb) {
      esp_camera_fb_return(fb); 
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    
    // Si algo falló (ej. se desconectó el Wi-Fi), rompemos el ciclo infinito
    if (res != ESP_OK) break;
  }
  return res;
}

// ==========================================
// 5. ARRANCAR EL SERVIDOR WEB
// ==========================================
void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG(); // Cargamos configuración por defecto
  config.server_port = 81; // AJUSTE CRÍTICO: Blynk requiere puertos específicos, usamos el 81

  // Creamos la "ruta" o dirección web. Para ver el video hay que poner "/stream" al final de la IP
  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,      // Método GET (solo entregar información)
    .handler   = stream_handler, // Le decimos que ejecute la función de fotos rápidas que hicimos arriba
    .user_ctx  = NULL
  };

  // Arrancamos el servidor y si todo sale bien (ESP_OK), registramos la ruta "/stream"
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

// ==========================================
// 6. INICIALIZACIÓN (Función que se ejecuta solo UNA VEZ al encender)
// ==========================================
void setup() {
  Serial.begin(115200); // Iniciamos comunicación con la PC para poder ver la IP en el Monitor Serie
  delay(2000); // Agrega esta línea: Espera 2 segundos para estabilizar la energía
  Serial.setDebugOutput(false); // Apagamos mensajes innecesarios para que la consola se vea limpia

  // Construimos un objeto "config" con todo el mapa de pines que definimos hasta arriba
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;      // Velocidad a la que trabaja el cerebro del lente (20 MHz)
  config.pixel_format = PIXFORMAT_JPEG; // Queremos que el lente ya nos entregue la foto comprimida

  // psramFound() revisa si el chip negro extra de memoria (PSRAM) está soldado y activado.
  // Si tiene memoria extra, podemos darnos el lujo de guardar 2 fotos al mismo tiempo en buena calidad (VGA).
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA; 
    config.jpeg_quality = 20; // Subimos el número para bajar la calidad y consumir menos RAM
    config.fb_count = 1;      // Usamos solo 1 espacio de memoria en vez de 2               // Doble buffer (memoria para 2 imágenes)
  } else {
    // Si no tiene memoria extra, bajamos la resolución (SVGA) y la calidad (12) para que el chip no colapse
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Ahora sí, inyectamos toda esta configuración al chip físico
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) { // Si falla (ej. lente mal conectado), avisa por la computadora
    Serial.printf("Fallo al iniciar camara: 0x%x", err);
    return; // Detiene la ejecución aquí mismo
  }

  // Comenzamos la conexión Wi-Fi
  WiFi.begin(ssid, password);
  
  // Mientras la conexión no diga "Conectado", imprime un punto cada medio segundo
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // ¡Se conectó! Avisamos a la PC
  Serial.println("");
  Serial.println("WiFi conectado!");

  // Encendemos el servidor de video en el puerto 81
  startCameraServer();

  // Imprimimos la dirección IP local que le dio el router, para que la puedas copiar a tu Python
  Serial.print("Servidor de video iniciado. IP de esta camara: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println(":81/stream");
}

// ==========================================
// 7. CICLO INFINITO
// ==========================================
void loop() {
  // En las placas de Arduino normales, aquí pones todo el código que se repite.
  // Pero el ESP32 es tan avanzado que el servidor web de la cámara funciona de 
  // manera "asíncrona" (en otro procesador interno de forma invisible).
  // Por lo tanto, el ciclo loop se queda "durmiendo" en bloques de 10 segundos 
  // para ahorrar energía. Toda la magia ya está ocurriendo en segundo plano.
  delay(10000); 
}