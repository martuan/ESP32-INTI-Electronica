# ESP32-INTI-Electronica

## Descripción del proyecto

Se pretende armar un sistema IoT para diversas aplicaciones en INTI con capacidad de enviar datos a la nube y acceder a ellos con cualquier dispositivo conectado a la red.

## Características
El sistema está compuesto por un microcontrolador con capacidad de conectarse por Wifi y por Bluetooth. Puede ser controlado por Bluetooth a través de una aplicación diseñada para Android o por medio de un teclado o botonera. Puede enviar datos vía Wifi a un servidor en la nube por medio de algún protocolo como, por ejemplo, MQTT. Puede transmitir y recibir datos de forma segura (ciberseguridad). Tiene posibilidad de integrar distintos sensores y periféricos y visualizar información a través de un display. Está pensado para ser un sistema escalable, donde se puedan conectar más módulos si se requiere.
El equipo debe poder actualizarse de forma práctica y de fácil acceso; en lo posible, vía programación OTA (Over The Air).
## Ciberseguridad
Cada equipo (nodo) enviará datos a la nube y recibirá datos de la misma, que puede ser un servidor (broker). Por lo tanto, la conexión debe ser segura para proteger los datos ya sean críticos o no. Deberá, en lo posible, realizar transacciones de datos encriptadas con un algoritmo adecuado. 
El sistema debe estar preparado para soportar ciberataques como Denegación de servicio, Man in the middle, captura de información por medio de un Sniffer, etc.
Debe poseer métodos de protección de credenciales y evitar falsas autenticaciones. La conexión Bluetooth debe lograrse a través de un apareamiento autenticado.
## Protocolos utilizados
Para el envío y recepción de datos por internet se utilizará el protocolo MQTT basado en la pila TCP/IP como base para la comunicación. Permite comunicaciones bidireccionales, de bajo ancho de banda, con posibilidad de encriptar sus mensajes, calidad de servicio configurable, etc.



Para más información:
https://www.luisllamas.es/que-es-mqtt-su-importancia-como-protocolo-iot/
Para la comunicación con otros módulos, puede aplicarse el protocolo ESP-Now que permite comunicarse con otros microcontroladores ESP32 de manera One-Way (de uno a otro) o Two-Way ( de uno a otro bidireccional). Además puede establecerse una comunicación Maestro - Esclavos (varios) o Maestros - Esclavo (uno solo).


Para más información:
https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/

Para comunicación con sensores y periféricos, se utilizarán los protocolos necesarios, ya sea I2C, SPI, RS232, etc.

## Entorno de desarrollo
### Lenguaje:
El firmware embebido en la placa ESP32 será realizado en lenguaje C/C++.
### IDE:
Se sugiere utilizar el IDE Visual Studio Code por su detección de errores mientras se escribe el código, función de autocompletado, guías de identado, accesos directo al código de librerías incluídas, etc.
Es fundamental instalar las extensiones “Arduino for Visual Studio Code” y “C/C++ Intellisense”
-https://www.luisllamas.es/arduino-visual-studio-code/
Para instalar la placa ESP32 en Arduino IDE
-https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/


## Diseño
### Circuito esquemático:
El diseño del circuito esquemático puede realizarse con algún CAD como Altium, KiCad, etc. También puede utilizarse la herramienta de software Fritzing para montar rápidamente un esquema de conexionado.
### PCB:
El diseño del PCB puede realizarse con algún CAD como Altium, KiCad, etc.
### Gabinete
Podrá ser realizado con software Fusion 360 y luego impreso en 3D con filamentos PLA o ABS. Puede llevar una inscripción personalizada en cualquiera de sus caras. Para generar el código de máquina (g-Code) que deben interpretar las impresoras 3D, puede utilizarse el software Slicer o el Cura u otro.

## Trabajo en repositorio
Se trabajará en un repositorio de github, en donde se alojarán principalmente los archivos de firmware del equipo y se gestionará su versionado. Además contendrá archivos readme.md donde se describen las features de cada versión. Se recomienda, entonces, instalar GIT, el sistema de control de versiones moderno más utilizado del mundo.
Más información acá:
https://www.freecodecamp.org/espanol/news/guia-para-principiantes-untitled/

Repositorio del proyecto:
https://github.com/martuan/ESP32-INTI-Electronica

## Proyección a futuro
Propiciar mecanismos para escalar el sistema de manera tal de que se puedan vincular módulos de manera sencilla.
A nivel broker (servidor), tener servicios dedicados para conformar informes y permitir exportalos.
