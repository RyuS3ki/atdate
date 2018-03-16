# Proyecto atdate (Servidor y cliente NTP)
## Datos autora
**Nombre:** Rebeca Jiménez Guillén
**Correo electrónico:** rebecajg@protonmail.com
**Grado:** Grado en Ingeniería Telemática

## Descripción
  El directorio del proyecto contiene: atdate.c, Makefile y README.md

## Funcionalidad atdate
### Especificaciones necesarias
  El servidor TCP escucha en el puerto 6649.
  
### Especificaciones extra
  Como función extra, el programa contiene también un servidor UDP funcional que
  puede probarse con el cliente UDP implementado. El mismo sirve también en el
  puerto 6649.
  A diferencia del servidor TCP, este envía una sola vez la respuesta al cliente
  y vuelve a escuchar nuevas peticiones hasta que se cierra con Ctrl-C.

# Compilación
  **make**
    -Se ejecuta un "rm" que limpia el directorio de posibles ejecutables previos
    -Se ejecuta la compilación del código fuente

# Ejecución
  ./atdate [-h serverhost] [-p port] [-m cu|ct|s|su] [-d]

## Warnings
  Debe tenerse en cuenta el estado de los puertos a utilizar si se hace entre varias máquinas.
