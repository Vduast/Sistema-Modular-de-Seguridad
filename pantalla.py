# Importamos las herramientas necesarias
import threading  # Permite ejecutar tareas en segundo plano (multihilo)
import time       # Permite hacer pausas

try:
    from RPLCD.i2c import CharLCD # Libreria especializada para controlar pantallas LCD por protocolo I2C
except ImportError:
    print("!! Error: No se encontro la libreria RPLCD dentro del entorno !!")

# ==========================================
# CONFIGURACION DEL LCD (4x16)
# ==========================================
try:
    # Configuramos el adaptador negro (PCF8574) soldado a tu pantalla.
    # address=0x27 es la direccion de red interna predeterminada.
    # cols=16 y rows=4 definen el tamano del display.
    lcd = CharLCD(i2c_expander='PCF8574', address=0x27, port=1, cols=16, rows=4, dotsize=8)
    lcd.clear() 
    print(">> Hardware LCD detectado e inicializado correctamente.")
except Exception as e:
    print(f"Advertencia: No se pudo conectar fisicamente al LCD ({e})")
    lcd = None 

# Variables globales que usara el hilo en segundo plano
texto_blynk = ""   # Aqui se guardara lo que escribas desde el celular
_corriendo = True  # Un "switch" para mantener el motor del texto encendido

def _scroll_logic():
    global texto_blynk
    pos = 0 # Posicion actual de la letra
    
    while _corriendo:
        if not lcd:
            time.sleep(2)
            continue
            
        actual = texto_blynk 
        
        # CASO 1: Si el texto esta vacio
        if actual == "":
            lcd.cursor_pos = (1, 0) # Nos movemos a la Fila 2
            lcd.write_string(" " * 16) # Borramos la linea con espacios
            time.sleep(1) 
            continue

        # CASO 2: Si el texto es corto (16 letras o menos)
        if len(actual) <= 16:
            str_show = actual.center(16) # Centra el texto
            pos = 0 
            
        # CASO 3: Si el texto es largo (mas de 16 letras), hace desplazamiento
        else:
            espacios = "                " 
            padded = espacios + actual + espacios 
            str_show = padded[pos : pos+16] 
            pos += 1 
            
            if pos > len(padded) - 16:
                pos = 0

        # Mandamos a imprimir el bloque preparado a la fila 2
        lcd.cursor_pos = (1, 0)
        lcd.write_string(str_show)
        
        time.sleep(0.35) 

def iniciar():
    # Creamos y arrancamos el hilo para que el texto no congele la alarma
    hilo = threading.Thread(target=_scroll_logic, daemon=True)
    hilo.start()
    print(">> Motor de desplazamiento de pantalla LCD [ACTIVO].")

def actualizar_texto(nuevo_texto):
    """Esta funcion recibe el texto de Blynk (Pin V5) lo imprime en terminal e inyecta al LCD"""
    global texto_blynk
    texto_blynk = nuevo_texto
    print(f"--> [MODULO PANTALLA] Datos nuevos asignados al LCD: '{nuevo_texto}' (Letras: {len(nuevo_texto)})")