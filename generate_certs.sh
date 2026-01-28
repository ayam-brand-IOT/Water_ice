#!/bin/bash

# Script para generar certificados SSL autofirmados para ESP32
# Ejecutar: bash generate_certs.sh

echo "Generando certificados SSL para ESP32..."

# Crear directorio para certificados
mkdir -p certs

# Generar clave privada y certificado autofirmado
openssl req -x509 -nodes -days 3650 -newkey rsa:2048 \
  -keyout certs/server_key.pem \
  -out certs/server_cert.pem \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=ESP32"

echo ""
echo "✓ Certificados generados en ./certs/"
echo ""
echo "Ahora convirtiendo a formato C para incluir en el código..."

# Convertir certificado a formato C
echo "const char server_cert[] PROGMEM = R\"CERT(" > include/ssl_cert.h
cat certs/server_cert.pem >> include/ssl_cert.h
echo ")CERT\";" >> include/ssl_cert.h
echo "" >> include/ssl_cert.h

# Convertir clave privada a formato C
echo "const char server_key[] PROGMEM = R\"KEY(" >> include/ssl_cert.h
cat certs/server_key.pem >> include/ssl_cert.h
echo ")KEY\";" >> include/ssl_cert.h

echo "✓ Archivo include/ssl_cert.h creado"
echo ""
echo "IMPORTANTE:"
echo "1. En tu navegador/iPhone verás advertencia de certificado no confiable"
echo "2. Acepta la advertencia para continuar"
echo "3. En iPhone: Settings > General > About > Certificate Trust Settings"
echo "4. Accede via: https://192.168.1.29 (tu IP del ESP32)"
echo ""
