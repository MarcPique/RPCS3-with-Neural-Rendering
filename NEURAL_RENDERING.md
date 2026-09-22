# RPCS3 Neural / ReShade — Windows x64 experimental

Esta variante añade una pestaña **Neural / ReShade** a los ajustes globales de RPCS3. Integra la carga de ReShade mediante una capa Vulkan local al proceso y permite editar la configuración que utilizan ReShade y sus complementos. Los cambios requieren reiniciar RPCS3.

La ruta de renderizado es **RPCS3 Vulkan → ReShade → LumeniteFX → DLSS5-Feeder → complemento neural → imagen presentada**. DLSS5oneclick sirve como referencia para la selección de componentes y la configuración. Su instalación DirectX habitual (`dxgi.dll`) no es suficiente para RPCS3.

## Uso

Requiere Windows x64, el redistribuible Microsoft Visual C++ x64 compatible con MSVC 14.51 o posterior y una GPU/controlador compatible con los componentes neurales. La máquina de prueba tiene una RTX 4090 y controlador 616.56. RPCS3 necesita el runtime de Visual C++ instalado en Windows; no copies `msvcp140.dll` ni `vcruntime140.dll` junto al ejecutable.

1. Ejecuta el autoextraíble `Extraer-RPCS3-Neural-0.2.1-win64.exe` o extrae **todo** `RPCS3-Neural-0.2.1-public-win64.zip` en una carpeta con permisos de escritura. Ambos incluyen `rpcs3.exe`, OpenCV, Qt y las demás DLL. Abre el emulador desde la carpeta extraída, sin separar el EXE de sus DLL.
2. Abre RPCS3 y selecciona **Vulkan** y la GPU NVIDIA en los ajustes de GPU.
3. En **Neural / ReShade**, pulsa **Descargar / reparar componentes**. También se descargan automáticamente al marcar la casilla de carga Vulkan si faltan archivos. La ventana muestra el progreso y permite cancelar. Elige un preset si lo deseas, activa la integración y guarda.
4. Cierra y vuelve a abrir RPCS3. Inicia el juego.
5. **Inicio/Home** abre ReShade. Comprueba que los complementos se hayan cargado y que `Lumenite_Kernel` preceda a `DLSS5_Feed` en los efectos activos. El complemento neural tiene sus propios controles; la carga de ReShade por sí sola no significa que la evaluación neural esté activa.
6. Detén el juego antes de editar los ajustes persistidos desde RPCS3. ReShade puede escribir esos archivos mientras está en uso.

La pestaña incluye controles de ReShade, activación e intensidad neural de RenoDX, presets, estilo, máscaras, profundidad, vectores de movimiento y controles del Feeder. Incluye edición completa de `ReShade.ini`, `ReShadePreset.ini` y `dlss5-feed.cfg`, y consulta de registros. Los editores completos permiten configurar las opciones persistidas de los complementos aunque aparezcan nuevas claves en sus versiones posteriores. Las opciones transitorias del overlay que no se guardan en INI siguen perteneciendo al complemento.

### Descarga integrada (0.2.1)

No necesitas abrir PowerShell: el botón utiliza los scripts incluidos junto al EXE para descargar desde los autores. Los componentes descargados permanecen instalados; los ajustes y la activación solo se guardan con **Aplicar / Guardar**. Si falla o cancelas la descarga iniciada desde la casilla, esta vuelve a quedar desmarcada. Puedes reintentar usando la caché local. El registro está en `neural-install.log`.

Para reparar componentes ya cargados, desactiva ReShade, guarda y reinicia antes de usar el botón. La instalación se bloquea durante la emulación y si otra instancia de esa misma copia de RPCS3 está abierta. `Setup-Neural.ps1` sigue disponible como alternativa manual con RPCS3 cerrado.

Si Windows informa de que falta `opencv_world4140.dll` o una DLL de Qt, se ha abierto una copia incompleta: vuelve a extraer el paquete completo. No descargues esas DLL por separado. El release ya no ofrece el emulador suelto como descarga independiente.

### Configuración rápida y deslizadores (0.2.0)

En **Neural / Feeder**, elige un perfil y pulsa **Usar preset**. Se preparan la activación Vulkan local, los hooks de RenoDX, el modo completo del Feeder y las técnicas Lumenite → Feeder. Se conservan los efectos adicionales, rutas personalizadas y claves desconocidas. Pulsa **Aplicar** o **Guardar** y reinicia RPCS3 con Vulkan y la GPU NVIDIA seleccionados. Cerrar sin guardar descarta también el preset.

| Preset | Intensidad | Tono | Estructura | Piel | Estilo |
| --- | ---: | ---: | ---: | ---: | --- |
| Suave | 0,60 | 0,90 | 0,75 | -1,00 | Natural |
| Equilibrado | 1,00 | 1,00 | 1,00 | -1,00 | Predeterminado |
| Detalle | 1,00 | 1,05 | 1,35 | 0,25 | Natural |
| Cinematográfico | 1,00 | 1,20 | 1,10 | -1,00 | Cinematográfico |

Son puntos de partida definidos para este paquete, no presets oficiales ni ajustes calibrados con juegos. El selector muestra el preset que se aplicará al pulsar el botón; puedes modificar después cada valor.

Los deslizadores tienen paso 0,01, límites visibles y entrada numérica directa: intensidad, tono y estructura 0–2; piel -1–1. Estos rangos siguen el [panel de configuración de Feeder](https://github.com/jlrouzies-fr/DLSS5-Feeder/blob/main/src/dlss5-feed32.cpp), que usa rangos observados del complemento. No son una API garantizada de NVIDIA. Un valor fuera del rango en el editor INI se muestra y conserva hasta editar el control. Una clave ausente muestra su valor de referencia sin escribirla automáticamente.

## Alcance y comprobaciones

- Configuración global para esta copia portable. Los ajustes de cada juego siguen usando el sistema original de RPCS3.
- La activación verifica que existan los archivos requeridos y que el manifiesto apunte a la DLL del paquete. No certifica compatibilidad de GPU, controlador o juego.
- No se modifica el registro de Vulkan ni se instala una capa para otros programas. Las variables de entorno se cambian solo dentro de RPCS3.
- Desactivar y reiniciar bloquea también una capa ReShade Vulkan implícita del sistema en este proceso. No administra proxies ajenos como `opengl32.dll` o `dxgi.dll` añadidos manualmente.
- Guardar conserva claves desconocidas. Los INI modificados tienen una copia anterior `.rpcs3-backup`; cada guardado sustituye esa copia. Se detectan cambios externos para evitar sobrescribirlos.
- No se incluye una implementación DLSS nativa en el renderizador RSX. Feeder usa profundidad y movimiento reconstruidos a través de ReShade; los resultados dependen del juego.
- La existencia de archivos o un overlay visible no demuestra que se procesen fotogramas con la red neural. Comprueba los registros y compara la salida en un juego real.

## Archivos y carga

Los complementos y runtimes están junto a `rpcs3.exe`. El manifiesto de ReShade está en `neural-rendering/ReShade64.json` y apunta a `..\ReShade64.dll`. La preferencia de activación se almacena por separado en `neural-rendering.json`.

Al iniciar, RPCS3 configura `VK_ADD_LAYER_PATH` (o conserva y amplía `VK_LAYER_PATH` si ya existe), `VK_INSTANCE_LAYERS` y `RESHADE_BASE_PATH_OVERRIDE`. La capa local se llama `VK_LAYER_RPCS3_reshade` para evitar conflictos con una instalación global; la capa global de ReShade queda desactivada solo en este proceso. Se carga también `VK_LAYER_feed_vk` para preparar la interoperabilidad Vulkan/D3D12. No uses un `INSTALL/BasePath` que redirija ReShade fuera del paquete: los ajustes de esta interfaz corresponden al INI local.

## Versiones del kit local

- ReShade 6.8.0 con soporte de add-ons.
- DLSS5-Feeder 1.16.0-beta.4.
- RenoDX `renodx-dlss5-4.55`, motor clásico. El banner interno puede indicar 4.1.5.
- Modelo neural `dlssnr-310.8.SF-v2` y DLSS SR 310.9.1.
- LumeniteFX y cabeceras ReShade: commits y SHA256 en `PROVENANCE.json`.

El release público incluye el emulador, sus dependencias redistribuibles y el descargador, sin modelos, shaders ni complementos neurales. `Setup-Neural.ps1` obtiene las versiones anteriores de sus autores y conserva los INI/CFG existentes. La caché queda en `.neural-downloads` y los archivos de procedencia y hashes acompañan a los componentes descargados.

El kit completo resultante se obtiene para uso local. No lo vuelvas a publicar: LumeniteFX prohíbe el rehosting público y no se ha establecido el permiso de redistribución de todos los binarios neurales.

## Fuentes

- [RPCS3](https://github.com/RPCS3/rpcs3), base `8db660b185496f115701ef4c77c1ca2bef60e422`.
- [DLSS5oneclick](https://github.com/faisalkindi/DLSS5oneclick), referencia `986f323c516db27605eac6ba19fd34e2e93a0663`, en especial `src/installer.rs` y `src/reshade_ini.rs`.
- [DLSS5-Feeder](https://github.com/jlrouzies-fr/DLSS5-Feeder), puente de Vulkan hacia NGX/D3D12.
- [ReShade](https://github.com/crosire/reshade), capa Vulkan y sistema de efectos/add-ons.
- [LumeniteFX](https://github.com/umar-afzaal/LumeniteFX), estimación de movimiento.
- [rhi-repo](https://github.com/RankFTW/rhi-repo), distribución de los componentes neurales referenciados por DLSS5oneclick.

Esta compilación es una modificación independiente. No es un release oficial de RPCS3, NVIDIA, ReShade ni DLSS5oneclick. Cada componente conserva su propia licencia y procedencia.

La activación viene desmarcada: selecciona la GPU y actívala desde los ajustes antes de reiniciar. Se incluye `VALIDACION.md` con las pruebas realizadas y sus límites. No se incluyen firmware ni juegos.

## Compilación y pruebas

Consulta `BUILDING.md` para las dependencias de RPCS3 y `tools/neural-rendering/README.md` para los scripts y versiones utilizados. Clona el repositorio con sus submódulos; los ZIP de fuentes automáticos de GitHub no los contienen. Los archivos nuevos están registrados tanto en CMake como en la solución de Visual Studio. No se necesitan cambios de Qt ni herramientas MOC adicionales para la nueva pestaña.

Las pruebas aisladas se encuentran en `tests/neural_rendering_config` y `tests/neural_rendering_ui`. Se pueden compilar con CMake y Qt 6 sin compilar todo el emulador. Verifican lectura/escritura de INI, conservación de parámetros, bloqueo de runtimes incompletos y comportamiento de los controles. Estas pruebas no sustituyen la prueba de una sesión de juego con GPU.
