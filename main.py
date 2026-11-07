#!/usr/bin/env python3
import time
import subprocess
import threading
import re
import BlynkLib
import sensores
import pantalla

# ==========================================
# 1. BLYNK CONFIGURATION
# ==========================================
BLYNK_TEMPLATE_ID = "TMPL2q2rXQbxS"
BLYNK_TEMPLATE_NAME = "sistema de seguridad"
BLYNK_AUTH = 'ZsiKXq0erycWM-b_jdWqLj4iXm3iN9Ni'

blynk = BlynkLib.Blynk(BLYNK_AUTH, server='blynk.cloud', port=80)

# ==========================================
# 2. CAMERA AND STATE CONFIGURATION
# ==========================================
URLS_STREAMING = {}
PIN_DEL_WIDGET_VIDEO = 4
PIN_V6_TEXTO = 6 # Virtual pin to send status text to Blynk

sistema_armado = False
alarma_disparada = False
texto_lcd_actual = "Sistema OK"

# ==========================================
# 3. CLOUDFLARE TUNNEL FUNCTIONS
# ==========================================
def esperar_internet():
    print("Iniciando servicios...")
    return True

# Thread to capture the Cloudflare URL
def _leer_salida_cf(proc):
    global URLS_STREAMING
    for linea in iter(proc.stdout.readline, ''):
        if "trycloudflare.com" in linea and not URLS_STREAMING:
            match = re.search(r'(https://[a-zA-Z0-9-]+\.trycloudflare\.com)', linea)
            if match:
                base_url = match.group(1)
                url_sin_candado = base_url.replace("https://", "http://")
                
                URLS_STREAMING = {
                    1: f"{url_sin_candado}/cam1/stream", 
                    2: f"{url_sin_candado}/cam2/stream"
                }
                print(f"\n>> Tunel Cloudflare LISTO. URL: {url_sin_candado}\n")

def iniciar_cloudflare():
    print("Levantando tunel Cloudflare...")
    ruta_cf = "/home/anael/sistema_seguridad/cloudflared"
    comando = [ruta_cf, "tunnel", "--url", "http://127.0.0.1:8080"]
    proc = subprocess.Popen(comando, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    
    hilo = threading.Thread(target=_leer_salida_cf, args=(proc,), daemon=True)
    hilo.start()
    time.sleep(4)

# ==========================================
# 4. BLYNK HANDLERS
# ==========================================
# Handler for Arm/Disarm switch
@blynk.VIRTUAL_WRITE(0)
def v0_write_handler(value):
    global sistema_armado, alarma_disparada
    print(f"--> [BLYNK] V0 (Armado/Desarmado): {value}")
    
    if int(value[0]) == 1:
        sistema_armado = True
        blynk.virtual_write(PIN_V6_TEXTO, "Armado")
        print(">> Sistema ARMADO.")
    else:
        sistema_armado = False
        alarma_disparada = False
        sensores.desactivar_alarma()
        blynk.virtual_write(PIN_V6_TEXTO, "Desarmado")
        print(">> Sistema DESARMADO.")

# Handler for Camera selection
@blynk.VIRTUAL_WRITE(3)
def v3_write_handler(value):
    try:
        seleccion = int(value[0])
        if seleccion in URLS_STREAMING:
            nueva_url = URLS_STREAMING[seleccion]
            blynk.set_property(PIN_DEL_WIDGET_VIDEO, "url", nueva_url)
            print(f">> Cambiando camara a: {nueva_url}")
    except Exception as e:
        print(f"!! Error procesando camara: {e}")

# Handler for LCD Text update
@blynk.VIRTUAL_WRITE(5)
def v5_write_handler(value):
    global texto_lcd_actual
    texto_lcd_actual = str(value[0])
    pantalla.actualizar_texto(texto_lcd_actual)
    print(f"--> [BLYNK] V5 (LCD): {texto_lcd_actual}")

# ==========================================
# 5. MAIN LOGIC
# ==========================================
def main():
    global sistema_armado, alarma_disparada
    esperar_internet()
    iniciar_cloudflare()
    pantalla.iniciar()
    
    # Initial status
    blynk.virtual_write(PIN_V6_TEXTO, "Desarmado")
    
    print(f"Iniciando {BLYNK_TEMPLATE_NAME}...")
    
    while True:
        try:
            blynk.run()
            
            # Logic to check sensors only when armed
            if sistema_armado and not alarma_disparada:
                if sensores.verificar_sensores():
                    alarma_disparada = True
                    sensores.activar_alarma()
                    pantalla.actualizar_texto("Intrusion")
                    blynk.virtual_write(PIN_V6_TEXTO, "Intrusion")
                    
                    print("!! ALARMA DISPARADA !!")
                    try:
                        blynk.log_event("alerta", "Intrusion detectada")
                    except:
                        pass
            
            time.sleep(1)
        except Exception as e:
            print(f"Error en main: {e}")
            time.sleep(2)

if __name__ == "__main__":
    main()