# Validación — RPCS3 Neural / ReShade 0.2.2 experimental

Fecha: 23 de septiembre de 2026. Windows x64, RTX 4090.
Base RPCS3: `8db660b185496f115701ef4c77c1ca2bef60e422` (0.0.42).
El hash del ejecutable y el commit se registran en `BUILD-INFO.json` dentro del paquete; `SHA256SUMS.txt` identifica las descargas.

## Cambios y pruebas 0.2.2

- El complemento propio `rpcs3-settings-only.addon64` veta la apertura del menú mediante teclado, mando y API. Permite cerrar el menú. No modifica la evaluación de efectos.
- Prueba de carga del DLL real con ABI ReShade simulado: registro con versión correcta, veto de todas las fuentes de entrada, cierre permitido, registro limitado a un mensaje y descarga limpia. No equivale a pulsar un mando físico dentro de un juego.
- Prueba Vulkan aislada con ReShade 6.8.0 real: el registro confirma que se carga el complemento propio y se activa el bloqueo; creación de dispositivo NVIDIA correcta (`VK_SUCCESS`). No crea swapchain ni evalúa fotogramas.
- Pruebas de configuración: elimina los atajos del menú y de efectos, fija la carpeta de complementos local, impide deshabilitar el complemento de bloqueo y conserva el resto de claves/listas escapadas. Verifica que la integración apagada bloquea las capas Vulkan locales y globales solo en este proceso.
- Pruebas Qt de interfaz y descargador aprobadas. Todos los parámetros numéricos del panel Neural / Feeder tienen slider y valor de solo lectura; los modos usan selectores. Los INI/CFG son vistas de solo lectura. Se verifican valores ausentes, fuera de rango, precisión existente, números enteros y negativos, cuatro presets, cancelar, aplicar varias veces, cambios externos y bloqueo durante emulación.
- El paquete incluye las DLL de OpenCV, Qt y FFmpeg, además del complemento propio de bloqueo. El ZIP y el EXE autoextraíble contienen el mismo conjunto de archivos. Los modelos, complementos neurales y shaders de terceros se descargan desde sus autores con el botón integrado.

## Sesión de Saw II antes de esta corrección

Se inició Saw II (BLES01050) con la versión 0.2.1 y se alcanzó una escena jugable con Vulkan y salida 3840×2160 en una RTX 4090. El registro confirmó compilación de Lumenite/Feeder y creación/evaluaciones satisfactorias de la función neural 18. La ventana rondó los 30 FPS durante esa sesión.

Esto demuestra que hubo evaluación neural en ese entorno, no que todos los presets mejoren la imagen. No se completó una comparación visual controlada activado/desactivado: el usuario decidió hacer personalmente esa comparación y las pruebas de modos. No se certifican calidad de profundidad/movimiento, HUD, estabilidad prolongada ni compatibilidad con otros juegos/GPU. Algunas muestras de profundidad del registro fueron planas y requieren revisión en más escenas.

## Comprobaciones anteriores conservadas

En 0.2.1 se completó una instalación real desde RPCS3 abierto y se verificaron 32 hashes descargados, conservación de ajustes y guardado posterior. Las pruebas simuladas cubren fallo, cancelación, descarga incompleta y reintento. También se comprobó la extracción del EXE completo y la resolución de dependencias del paquete.

Una prueba independiente del runtime D3D12 completó 300/300 evaluaciones a 640×360; no se utiliza como sustituto de una sesión RPCS3. No se ejecutó el conjunto de pruebas upstream de toda la solución; el objetivo compilado es `rpcs3`.

## Uso

Extrae el paquete completo y abre `rpcs3.exe`. Selecciona Vulkan y tu GPU NVIDIA. En **Ajustes → Neural / ReShade**, descarga los componentes, elige un preset si lo deseas y guarda. Para activar o desactivar completamente la integración, guarda y reinicia RPCS3. El menú ReShade no está disponible dentro del juego, incluso con la integración activada.

La compilación es independiente y experimental. Requiere el runtime Visual C++ x64 compatible con MSVC 14.51 o posterior. No incluye firmware ni juegos. No se publican registros ni ajustes personales de la máquina de prueba.