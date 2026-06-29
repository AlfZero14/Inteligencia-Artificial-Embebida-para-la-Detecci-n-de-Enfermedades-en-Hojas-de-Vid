# Inteligencia-Artificial-Embebida-para-la-Detecci-n-de-Enfermedades-en-Hojas-de-Vid

El sistema desarrollado tiene como finalidad identificar automáticamente enfermedades presentes en hojas de vid mediante técnicas de visión artificial y Machine Learning embebido. Todo el procesamiento relacionado con el reconocimiento de imágenes se realiza de manera local en la **ESP32-S3-EYE**, evitando la necesidad de enviar imágenes a servidores externos y permitiendo obtener una respuesta rápida, autónoma y portátil.

La cámara integrada de la ESP32-S3-EYE captura una imagen de la hoja y la envía al procesador para su preprocesamiento. La imagen es adaptada al formato requerido por el modelo de inteligencia artificial y posteriormente es procesada mediante un modelo de clasificación desarrollado en la plataforma **Edge Impulse** utilizando la técnica de **Transfer Learning**.

El modelo analiza las características visuales de la hoja y determina la categoría a la que pertenece, pudiendo identificar hojas saludables o afectadas por Mildiu, Oídio o Mancha bacteriana.

Debido a que la **ESP32-S3-EYE** no dispone de pines GPIO expuestos para la conexión directa de dispositivos externos, se incorpora un segundo microcontrolador, el **ESP32-S3-DevKit-1**, el cual actúa como unidad auxiliar para la visualización de la información. Una vez obtenido el resultado de la clasificación, la **ESP32-S3-EYE** lo transmite al **ESP32-S3-DevKit-1** mediante un enlace de comunicación entre ambos microcontroladores. Posteriormente, el **ESP32-S3-DevKit-1** recibe esta información y la presenta en una pantalla OLED de 0.96", permitiendo al usuario visualizar el diagnóstico de forma clara e inmediata.

Todo el sistema es alimentado por un banco de baterías recargables de 7.4 V, lo que permite su funcionamiento de manera autónoma y portátil.
