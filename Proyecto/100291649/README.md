# Proyecto Práctica Global Aptel 2017
## Datos autora
**Nombre:** Rebeca Jiménez Guillén
**NIA:** 100291649
**Correo electrónico:** 100291649@alumnos.uc3m.es
**Grupo:** 72
**Grado:** Grado en Ingeniería Telemática
## Descripción
  El directorio del proyecto contiene: atdate.c, Makefile y README.md

## Funcionalidad atdate
### Especificaciones necesarias
  El servidor TCP escucha en el puerto 6649 (6000 + 4 últimos dígitos del NIA).
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
