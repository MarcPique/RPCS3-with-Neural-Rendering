# RPCS3 Neural / ReShade 0.2.0 — Windows x64 experimental

Compilación independiente basada en RPCS3 0.0.42 (`8db660b`). Añade activación local de ReShade/Feeder y configuración neural siguiendo los componentes de DLSS5oneclick. No es un release oficial de RPCS3 o NVIDIA.

## Descarga y configuración rápida

1. Descarga y extrae **RPCS3-Neural-0.2.0-public-win64.zip**. Incluye el EXE y las dependencias de Qt y multimedia. El `rpcs3.exe` suelto es para sustituirlo en una copia con esas dependencias.
2. Abre PowerShell en la carpeta extraída y ejecuta `./Setup-Neural.ps1`. Si la política local impide ejecutar scripts, puedes usar `powershell.exe -NoProfile -ExecutionPolicy Bypass -File ./Setup-Neural.ps1`; esto solo cambia la política de ese proceso. No requiere administrador en una carpeta con permiso de escritura.
3. Abre RPCS3, selecciona **Vulkan** y tu GPU NVIDIA compatible.
4. En **Config → Neural / ReShade → Neural / Feeder**, elige **Suave**, **Equilibrado**, **Detalle** o **Cinematográfico**, pulsa **Usar preset**, guarda y reinicia RPCS3.

Requiere Windows x64 y el runtime Microsoft Visual C++ x64 compatible con MSVC 14.51 o posterior, instalado en Windows. No copies DLL de ese runtime junto al EXE.

## Incluido

- Deslizadores sincronizados con entrada numérica para **intensidad**, **tono**, **estructura** y **piel**, con paso 0,01.
- Cuatro presets para preparar la cadena Lumenite → Feeder y la activación neural. Puedes modificar todos sus valores antes de guardar.
- Ajustes de ReShade, RenoDX y Feeder, editores completos de INI/CFG, diagnóstico y conservación de ajustes personalizados.
- Carga Vulkan local al proceso; activación desmarcada inicialmente. El fork desactiva la actualización automática del emulador oficial.
- Código fuente, pruebas, scripts de compilación y SHA256 de los archivos publicados.

El ZIP público no incluye LumeniteFX, modelos ni complementos neurales. `Setup-Neural.ps1` los descarga desde sus autores en versiones fijadas, conservando tus ajustes. La licencia de LumeniteFX prohíbe su rehosting y no se ha establecido permiso de redistribución de todos los binarios neurales. No vuelvas a publicar el kit completo tras ejecutar el instalador.

## Validación y límites

Compilación Release x64, 46 comprobaciones de configuración, pruebas de interfaz/deslizadores/presets, carga de complementos, creación de dispositivo Vulkan y diagnóstico neural independiente de **300/300 evaluaciones** aprobados en una RTX 4090. Instalador probado en PowerShell 5.1, incluida reinstalación conservando ajustes.

**No se ha probado una sesión de juego de PS3.** La presentación neural completa, calidad visual, estabilidad y rendimiento en juegos siguen sin validar. Los presets son puntos de partida. No se incluyen firmware ni juegos.

[Guía de ajustes](https://github.com/MarcPique/rpcs3-neural/blob/v0.2.0-neural/NEURAL_RENDERING.md) · [Informe de validación](https://github.com/MarcPique/rpcs3-neural/blob/v0.2.0-neural/docs/neural-rendering/VALIDACION.md) · [Compilación y dependencias](https://github.com/MarcPique/rpcs3-neural/blob/v0.2.0-neural/tools/neural-rendering/README.md)
