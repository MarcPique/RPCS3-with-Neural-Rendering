# RPCS3 Neural / ReShade 0.2.2 — menú solo en ajustes y sliders

- Menú de ReShade bloqueado dentro del juego: teclado, mando y solicitudes de complementos. Los efectos se configuran desde Ajustes de RPCS3.
- Todos los parámetros numéricos del panel Neural / Feeder tienen sliders con el valor visible. Los modos usan selectores y se mantienen los cuatro presets rápidos.
- Se conservan los ajustes existentes. Las vistas INI/CFG son de solo lectura.
- Descarga/reparación integrada de los componentes desde sus autores.

Descarga **Extraer-RPCS3-Neural-0.2.2-win64.exe** o **RPCS3-Neural-0.2.2-public-win64.zip**. Ambos contienen el emulador y sus dependencias, incluido OpenCV. Abre `rpcs3.exe` desde la carpeta extraída.

Para actualizar una instalación existente, cierra RPCS3 y copia los archivos del paquete, conservando tu `neural-rendering.json`, INI/CFG y carpeta `config`. No copies la activación desmarcada del paquete sobre una configuración que quieras mantener activada.

**Activar/desactivar requiere guardar y reiniciar RPCS3.** El menú queda bloqueado también cuando la integración está activada. El paquete incluye nuestro complemento `rpcs3-settings-only.addon64`; no lo omitas al actualizar.

Validación: pruebas de configuración, interfaz, descargador y DLL de bloqueo aprobadas; carga del complemento verificada con ReShade/Vulkan real. Saw II alcanzó gameplay con evaluación neural real a 4K en la versión anterior; la comparación visual de presets queda a cargo del usuario. No se afirma que todos los modos hayan sido validados visualmente.

Compilación experimental no oficial. Requiere Windows x64 y runtime Visual C++ x64 compatible con MSVC 14.51 o posterior. No contiene juegos, firmware, modelos ni shaders de terceros; estos últimos componentes se obtienen desde sus autores mediante el descargador.

[Guía](https://github.com/MarcPique/rpcs3-neural/blob/neural-rendering/NEURAL_RENDERING.md) · [Validación detallada](https://github.com/MarcPique/rpcs3-neural/blob/neural-rendering/docs/neural-rendering/VALIDACION.md)