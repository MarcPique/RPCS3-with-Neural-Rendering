# Validación — RPCS3 Neural / ReShade 0.2.1 experimental

Fecha: 22 de septiembre de 2026. Paquete local Windows x64.

Base RPCS3: `8db660b185496f115701ef4c77c1ca2bef60e422` (0.0.42).
Ejecutable SHA256: `f576ceb0ab4bda5a395d2e6f2d31e0908fecba89709e52c57bb5e4cfe8a4583e`.

## Corrección 0.2.1

- Compilación final Release x64: 0 errores y 0 advertencias.
- Las dos suites Qt (`neural-rendering-ui` y `neural-rendering-download`) pasaron. Las nuevas pruebas ejecutan un instalador local simulado, sin red, para verificar scripts ausentes, salida de error, descarga incompleta, cancelación, activación automática desde la casilla, conservación de cambios pendientes, guardado posterior y bloqueo durante emulación.
- En el ejecutable RPCS3 real se completó la instalación con el emulador abierto, usando las descargas almacenadas en caché y las consultas a los autores. Se verificaron 32 hashes instalados. Antes de pulsar Save no se había creado ReShade.ini y la activación seguía desmarcada en disco; Save creó los ajustes y guardó la activación correctamente.
- Se inspeccionaron las importaciones de los 42 EXE/DLL del paquete. Se resuelven en el paquete o Windows/System32, con el runtime Visual C++ de esta máquina. OpenCV, Qt y FFmpeg están incluidos. Esto no sustituye una prueba en otro Windows sin Visual C++ instalado.
- Se ofrece un EXE autoextraíble con todas las dependencias, además del ZIP completo. Se retira el EXE del emulador suelto del release anterior para evitar ejecutarlo sin sus DLL.

Las comprobaciones siguientes se realizaron durante el desarrollo 0.2.0. Se mantienen los mismos componentes neurales; las pruebas Qt se han vuelto a ejecutar con 0.2.1.

## Resultados

- **Compilación Release x64:** proyecto RPCS3 y dependencias completados con MSVC 14.51, Qt 6.11.2 y LLVM 22.1.8. Última compilación: 0 errores y 0 advertencias. La solución completa intentó ejecutar un proyecto de pruebas upstream sin GoogleTest; ese conjunto de pruebas no se ha ejecutado. El script entregado compila el objetivo `rpcs3`.
- **Configuración:** 46 comprobaciones aprobadas. Lectura/escritura INI, BOM/CRLF, conservación de claves, duplicados, manifiestos, rutas y variables Vulkan.
- **Deslizadores y presets 0.2.0:** pruebas aprobadas de sincronización slider/entrada numérica/INI, valores ausentes, fuera de rango o no numéricos; cuatro presets; conservación de listas escapadas, rutas y efectos personalizados; cancelación sin escrituras. El preset Detalle también se aplicó y guardó desde el ejecutable real; se verificaron intensidad 1,00, tono 1,05, estructura 1,35 y piel 0,25 en el INI. Los presets son puntos de partida, no calibraciones de juegos.
- **Interfaz Qt:** pruebas aprobadas de cancelar, sincronizar controles y editores, aplicar varias veces, detectar cambios externos y errores de lectura, bloquear edición durante emulación y validar componentes incompletos.
- **Interfaz del emulador:** arranque real de RPCS3, apertura de Config → Neural / ReShade, inspección de controles ReShade y Neural / Feeder y guardado de activación en `neural-rendering.json`.
- **Activación en el ejecutable final:** con `enabled=false` no se cargaron módulos ReShade/Feeder ni se creó ReShade.log; con `enabled=true`, ReShade.log confirmó la DLL de esta copia portable y el registro de los complementos DLSS 5 Feed y DLSS 5 Neural Rendering. Se corrigió la inicialización para ejecutarse antes de la primera enumeración Vulkan.
- **Vulkan real:** capas locales descubiertas, RTX 4090 detectada, extensiones de memoria/semaforización compartida disponibles y `vkCreateDevice=VK_SUCCESS`. Esta prueba no crea un swapchain ni evalúa fotogramas de un juego.
- **Evaluación neural real independiente:** `dlss5-feed-host64.exe --test`, en una carpeta de prueba separada, completó **300/300 evaluaciones** a 640×360. Su registro ReShade confirmó creación y evaluación de la función neural 18. Es una prueba sintética D3D12 del runtime; no equivale a una sesión RPCS3.
- **Instalador público:** instalación y reinstalación con Windows PowerShell 5.1 aprobadas. Se verificaron los hashes de 32 archivos instalados y la conservación de los cuatro archivos de ajustes, incluyendo cambios personalizados. Se probó una descarga HTTP nueva además de reutilizar la caché para los archivos grandes. El script no ejecutó componentes descargados y eliminó su carpeta temporal.

Máquina: NVIDIA GeForce RTX 4090, controlador 616.56; también hay una GPU AMD integrada. La carga local convive con ReShade instalado globalmente porque esta copia desactiva la capa global únicamente dentro de su proceso.

## Pendiente

**No se ha ejecutado un juego de PS3.** No se han certificado la cadena completa de presentación Vulkan → ReShade → Feeder → neural en un juego, la compilación de sus efectos durante esa sesión, la calidad de profundidad/movimiento, la corrección del HUD, la estabilidad prolongada ni el rendimiento en juegos. No se incluyen firmware ni juegos. No se promete compatibilidad de otros controladores o GPU.

## Uso

Extrae el paquete completo, abre `rpcs3.exe`, selecciona Vulkan y la GPU NVIDIA, y usa **Config → Neural / ReShade → Descargar / reparar componentes**. Marcar la casilla de activación también inicia la descarga si faltan archivos. Guarda y reinicia. El paquete empieza con la carga desactivada. Consulta `LEEME.md` en el ZIP o `NEURAL_RENDERING.md` en el repositorio para los ajustes, controles avanzados y licencias.

Es una compilación independiente y experimental. El release público excluye los componentes neurales con redistribución restringida o no verificada; el descargador los obtiene de sus autores para uso local. El código de la integración, pruebas y scripts está en este fork.
