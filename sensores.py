# -*- coding: utf-8 -*-
print("--- [DEBUG] ARCHIVO SENSORES.PY CARGADO EXITOSAMENTE ---")
import RPi.GPIO as GPIO

# Configuracion de Pines
PIN_S1 = 17
PIN_S2 = 27
PIN_S3 = 10
PIN_SIRENA = 25

# Configurar GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)

# Sensores como entrada con resistencia Pull-Up interna
# Recordatorio: Con Pull-Up, el pin lee 1 (HIGH) cuando esta ABIERTO (ventana abierta)
# y lee 0 (LOW) cuando esta CERRADO (ventana cerrada/iman presente).
GPIO.setup(PIN_S1, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(PIN_S2, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(PIN_S3, GPIO.IN, pull_up_down=GPIO.PUD_UP)

# Sirena como salida
GPIO.setup(PIN_SIRENA, GPIO.OUT)
GPIO.output(PIN_SIRENA, GPIO.LOW) # Sirena apagada al inicio

def verificar_sensores():
    # Leemos el estado
    s1 = GPIO.input(PIN_S1)
    s2 = GPIO.input(PIN_S2)
    s3 = GPIO.input(PIN_S3)
    
    # DEBUG: Para ver en journalctl si estan abriendo o cerrando
    # 1 = ABIERTO (Alarma), 0 = CERRADO (Seguro)
    print(f"--- [DEBUG SENSOR] S1:{'ABIERTO' if s1 else 'CERRADO'} | S2:{'ABIERTO' if s2 else 'CERRADO'} | S3:{'ABIERTO' if s3 else 'CERRADO'} ---")
    
    # Si cualquiera de los sensores es 1 (ABIERTO), disparamos
    if s1 == 1 or s2 == 1 or s3 == 1:
        return True
        
    return False

def activar_alarma():
    GPIO.output(PIN_SIRENA, GPIO.HIGH)

def desactivar_alarma():
    GPIO.output(PIN_SIRENA, GPIO.LOW)