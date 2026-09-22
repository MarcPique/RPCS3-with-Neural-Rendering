# Validación — RPCS3 Neural / ReShade 0.2.0 experimental

Fecha: 22 de septiembre de 2026. Paquete local Windows x64.

Base RPCS3: `8db660b185496f115701ef4c77c1ca2bef60e422` (0.0.42).
Ejecutable SHA256: `6345d11d637760cad0d9efabbc61e939b08e0d96a30fea9869daa57b19f6ad4f`.

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

Extrae el ZIP público, ejecuta `Setup-Neural.ps1` para obtener los componentes, abre `rpcs3.exe`, selecciona Vulkan y la GPU NVIDIA, activa **Config → Neural / ReShade → Cargar ReShade Vulkan en RPCS3**, guarda y reinicia. El paquete empieza con la carga desactivada. Consulta `LEEME.md` en el ZIP o `NEURAL_RENDERING.md` en el repositorio para los ajustes, controles avanzados y licencias.

Es una compilación independiente y experimental. El release público excluye los componentes neurales con redistribución restringida o no verificada; el descargador los obtiene de sus autores para uso local. El código de la integración, pruebas y scripts está en este fork.
