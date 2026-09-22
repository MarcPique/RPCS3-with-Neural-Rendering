# RPCS3 Neural / ReShade 0.2.1 — descarga integrada y paquetes completos

Esta versión añade la instalación de componentes desde los ajustes de RPCS3 y corrige la descarga del EXE sin sus dependencias.

## Descarga recomendada

- **Extraer-RPCS3-Neural-0.2.1-win64.exe**: autoextraíble. Ejecútalo, elige una carpeta y abre `rpcs3.exe` dentro de la carpeta extraída. Incluye OpenCV, Qt, FFmpeg y los scripts de descarga.
- **RPCS3-Neural-0.2.1-public-win64.zip**: alternativa con los mismos archivos. Usa **Extraer todo** y abre el emulador desde la carpeta extraída.

No muevas `rpcs3.exe` fuera de esa carpeta ni lo ejecutes directamente dentro del ZIP. Ya no se publica el emulador suelto como descarga independiente: le faltarían DLL como `opencv_world4140.dll`.

## Activación sin terminal

1. Abre **Config → Neural / ReShade**.
2. Pulsa **Descargar / reparar componentes**, o marca la casilla de activación: si faltan componentes, se abre automáticamente la descarga con progreso y cancelación.
3. Selecciona **Vulkan** y una GPU NVIDIA compatible. Elige un preset **Suave / Equilibrado / Detalle / Cinematográfico** si lo deseas; puedes ajustar intensidad, tono, estructura y piel con sus deslizadores numéricos.
4. Pulsa **Save / Guardar** y reinicia RPCS3.

Los ajustes pendientes se conservan durante la instalación. Una descarga fallida o cancelada no guarda la activación. Puedes reintentar. Para reparar componentes que ya estén cargados, primero desactiva la integración, guarda y reinicia.

Requiere Windows x64, conexión a Internet para obtener los componentes neurales y Microsoft Visual C++ x64 compatible con MSVC 14.51 o posterior instalado en Windows. No copies DLL de Visual C++ junto al emulador.

## Comprobado

- Compilación Release x64: 0 errores y 0 advertencias.
- Pruebas Qt de interfaz y descarga: errores, cancelación, archivos incompletos, conservación de ajustes y guardado posterior.
- Instalación desde el RPCS3 real con la aplicación abierta, verificación de 32 hashes y guardado de la activación.
- Importaciones de los 42 EXE/DLL del paquete resueltas en el paquete o Windows/System32 de la máquina de prueba.

**Experimental: sigue sin validarse una sesión de juego de PS3**, incluida calidad visual, estabilidad y rendimiento de la cadena neural completa. No es una integración oficial de NVIDIA ni un release oficial de RPCS3. No incluye firmware ni juegos.

Los componentes neurales con redistribución restringida o no verificada se descargan desde sus autores. No están dentro de las descargas públicas; no vuelvas a publicar el kit completo después de instalarlos.

[Guía de ajustes](https://github.com/MarcPique/rpcs3-neural/blob/v0.2.1-neural/NEURAL_RENDERING.md) · [Validación](https://github.com/MarcPique/rpcs3-neural/blob/v0.2.1-neural/docs/neural-rendering/VALIDACION.md) · [Código y compilación](https://github.com/MarcPique/rpcs3-neural/blob/v0.2.1-neural/tools/neural-rendering/README.md)
