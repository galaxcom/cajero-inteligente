# cajero-inteligente

## Introducción
En los supermercados actuales, el sistema de pesaje utilizado para determinar el precio de los productos se basa en el uso de una báscula tradicional, seguido de la captura manual de un código por parte del cajero para identificar el tipo de mercancía que se está pesando. Esta metodología presenta diversas limitaciones y desafíos que afectan la eficiencia y la experiencia del usuario

## Justificación
La implementación de un sistema de pesaje más avanzado y automatizado en los supermercados no sólo optimizará el proceso de cobro y mejorará la precisión de las transacciones, sino que también permitirá una integración más efectiva con otras tecnologías y mejorará la experiencia del cliente.

## Requisitos

1. Componentes a utilizar
     - ESP32 CAM
     - Raspberry pi 4b
     - Lector de código de barras GM65
     - LCD 12C
     - FTDI
     - Loadcell y amplificador HX711
2. Instalado en la Raspberry pi
     - Node-red
        - node-red-contrib-telegrambot
        - node-red-node-mysql   
     - Grafana
     - LAMP
     - Mosquitto server
3. Instalado en la pc
     - Arduino IDE
## Diseño de circuito

<img width="542" alt="image" src="https://github.com/galaxcom/cajero-inteligente/assets/33261737/f57c19ec-128e-43bb-a644-7ccac69bdecf">

## Software y flujo de información

<img width="643" alt="image" src="https://github.com/galaxcom/cajero-inteligente/assets/33261737/0f8fb645-e6f2-4b90-8456-25d21d271440">

## Node-red flow

1. Envío de notificaciones a Telegram
<img width="930" alt="image" src="https://github.com/galaxcom/cajero-inteligente/assets/33261737/b861f06f-803b-4674-8114-5cab261cd289">

2. Inicialización del modelo
<img width="607" alt="image" src="https://github.com/galaxcom/cajero-inteligente/assets/33261737/c11af922-8029-4629-ac21-a1f616799b60">

3. Registro de venta por peso
- Obtiene el peso
- Obtiene la imagen y la predicción
- Guarda la venta en la base de datos
- Mandar producto por MQTT
<img width="938" alt="image" src="https://github.com/galaxcom/cajero-inteligente/assets/33261737/cdf479e3-facb-4ca4-b7ca-08d4d716f6e0">

4. Administración
- Editar productos
<img width="874" alt="image" src="https://github.com/galaxcom/cajero-inteligente/assets/33261737/421b73c1-3420-49a7-8e29-5356eac26f89">

- Editar proveedor
<img width="889" alt="image" src="https://github.com/galaxcom/cajero-inteligente/assets/33261737/ce9fc0c9-242f-48b3-84e6-71ee6c08323e">

- Editar almacén
<img width="893" alt="image" src="https://github.com/galaxcom/cajero-inteligente/assets/33261737/d68ef997-eaa6-4fc2-aeea-a82c8ee898cc">

## Grafana
- Productos
<img width="468" alt="image" src="https://github.com/galaxcom/cajero-inteligente/assets/33261737/3961c63c-6c1b-4004-9017-6d138de99c02">

- Proveedores
<img width="467" alt="image" src="https://github.com/galaxcom/cajero-inteligente/assets/33261737/1cecf05a-b36e-48bd-83bc-476d27577eea">

- almacenes
<img width="468" alt="image" src="https://github.com/galaxcom/cajero-inteligente/assets/33261737/89987914-580d-4fc9-922d-f722496f8df4">


## Resultado
<img width="960" alt="image" src="https://github.com/galaxcom/cajero-inteligente/assets/33261737/4a8dfdbf-bb2f-43b9-a567-6d4494df6720">
<img width="959" alt="image" src="https://github.com/galaxcom/cajero-inteligente/assets/33261737/c1b6db11-2d6a-4aad-972f-85e4081448d3">
<img width="960" alt="image" src="https://github.com/galaxcom/cajero-inteligente/assets/33261737/699d1bb9-3977-40a9-bf7a-1db53d6df8b8">

- dashboard
<img width="960" alt="image" src="https://github.com/galaxcom/cajero-inteligente/assets/33261737/7181776f-8774-45ae-9643-cf7d2655aaa2">

- link video
https://youtu.be/EE_gVXz0QTI

## Elaborado por
- David Galaviz Cruz
- Raúl Vargas Nava
- Dafne Citlaly Jiménez Román
