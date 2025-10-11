

| Fase                                                   | Semana                 | Actividad Principal                                                                                 | Entregable / Hito Concreto                                   | Estado        |
| :----------------------------------------------------- | :--------------------- | :-------------------------------------------------------------------------------------------------- | :----------------------------------------------------------- | :------------ |
| **🧠 Fase 1:<br>Investigación y Diseño**               | **25–31 Mar**          | Revisión bibliográfica inicial y definición del plan.                                               | Cronograma inicial y estado del arte preliminar.             | ✅ Hecho       |
|                                                        | **1–7 Abr**            | Análisis técnico en profundidad del pattern matcher nativo de WL.                                   | Notebook de análisis con ejemplos de cuellos de botella.     | ✅ Hecho       |
|                                                        | **8–14 Abr**           | Definición formal de objetivos y resultados esperados.                                              | Documento de Objetivos Generales y Específicos.              | ✅ Hecho       |
|                                                        | **15–21 Abr**          | Redacción de la problemática y justificación técnica.                                               | **Reporte Técnico #1:** Problemática y Justificación.        | ✅ Hecho       |
|                                                        | **22–28 Abr**          | Investigación de arquitecturas de VM (WASM, regex compilada).                                       | Documento de diseño arquitectónico preliminar.               | ✅ Hecho       |
|                                                        | **29 Abr–5 May**       | Diseño del Instruction Set (ISA) y la Representación Intermedia (IR).                               | Borrador del conjunto de instrucciones y esquema de IR.      | ✅ Hecho       |
|                                                        | **6–12 May**           | Redacción del proyecto de investigación completo.                                                   | **Reporte Técnico #2:** Anteproyecto de Tesis.               | ✅ Hecho       |
|                                                        | **13–19 May**          | Preparación de la presentación parcial.                                                             | Slides y material de apoyo.                                  | ✅ Hecho       |
|                                                        | **20–24 May**          | **HITO DE REVISIÓN**                                                                                | **Presentación Parcial y Retroalimentación.**                | ✅ Hecho       |
| **⚙️ Fase 2:<br>Implementación del Núcleo (Stack VM)** | **27 May–2 Jun**       | Configuración del entorno de desarrollo (paclet, GitHub, CI).                                       | Repositorio de código público y estructura inicial.          | ✅ Hecho       |
|                                                        | **3–9 Jun**            | Implementación del front-end (parser de patrones a IR).                                             | Función `PatternToIR` para patrones básicos.                 | ✅ Hecho       |
|                                                        | **10–16 Jun**          | Implementación del compilador de IR a bytecode.                                                     | Función `IRToBytecode` con representación textual.           | ✅ Hecho       |
|                                                        | **17–23 Jun**          | Diseño e implementación del loop principal de la VM.                                                | **Reporte Técnico #3:** Diseño de la VM.                     | ✅ Hecho       |
|                                                        | **24–30 Jun**          | Implementación de instrucciones básicas (stack-based).                                              | Ejecutor capaz de correr bytecode simple.                    | ✅ Hecho       |
|                                                        | **1–7 Jul**            | Integración pipeline completo: Patrón → IR → Bytecode → Ejecución.                                  | **Prototipo Alfa (Stack VM).**                               | ✅ Hecho       |
|                                                        | **8–14 Jul**           | Ampliación ISA para patrones compuestos.                                                            | Soporte para `f[x_, y_]`, `{a_, b_}` en stack VM.            | ✅ Hecho       |
|                                                        | **15–19 Jul**          | **HITO DE REVISIÓN**                                                                                | **Prototipo Funcional (Stack VM).**                          | ✅ Hecho       |
|                                                        | **Jul–Ago (paralelo)** | Iteraciones sobre ejecución en stack.                                                               | Motor de ejecución funcional.                                | ✅ Hecho       |
| **⚙️ Fase 2A:<br>Implementación Low-Level (C++)**      | **Agosto (continuo)**  | Inicio de la implementación en C++ del motor (estructura base, runtime, opcodes).                   | Repositorio C++ con prototipo funcional.                     | ✅ En progreso |
|                                                        | **Agosto (continuo)**  | Desarrollo de herramientas de **debugging**: logging, trazas de ejecución, inspección de registros. | Módulo de logging activo dentro de la implementación en C++. | ✅ En progreso |
| **⚙️ Fase 2B:<br>Transición a VM de Registros**        | **26 Ago – 1 Sep**     | Diseño formal del modelo de registros y comparación con stack.                                      | Documento técnico: modelo de registros y mapeo de opcodes.   | 🔄            |
|                                                        | **2–8 Sep**            | Migración del motor de ejecución a registros.                                                       | Motor iterativo con registros básicos.                       | 🔄            |
|                                                        | **9–15 Sep**           | Reimplementación de opcodes existentes en modelo de registros.                                      | Ejecutor capaz de correr casos simples en registros.         | 🔄            |
|                                                        | **16–22 Sep**          | Extensión ISA para patrones compuestos/anidados (`f[x_, y_]`, `{a_, b_}`).                          | Conjunto de opcodes ampliado y probado.                      | 🔄            |
|                                                        | **23–29 Sep**          | **HITO INTERMEDIO**: Comparación stack vs registros.                                                | Reporte de equivalencia semántica preliminar.                | 🔄            |
| **🧪 Fase 3:<br>Optimización y Validación**            | **30 Sep – 6 Oct**     | Implementación de optimizaciones básicas en IR/bytecode.                                            | Función `OptimizeIR` con ejemplos.                           | 🔄            |
|                                                        | **7–13 Oct**           | Sistema de logging y tracing (versión final, C++).                                                  | Logs detallados de ejecución.                                | 🔄            |
|                                                        | **14–20 Oct**          | Suite de pruebas (MatchQ vs VM).                                                                    | Pruebas de equivalencia semántica.                           | 🔄            |
|                                                        | **21–27 Oct**          | Benchmarking preliminar de rendimiento.                                                             | **Reporte Técnico #4:** Resultados iniciales.                | 🔄            |
|                                                        | **28 Oct – 3 Nov**     | Optimización de memoria y gestión de registros.                                                     | Mejoras medibles en benchmarks.                              | 🔄            |
|                                                        | **4–10 Nov**           | **HITO DE CALIDAD**                                                                                 | **Versión Beta del matcher con registros.**                  | 🔄            |
| **📝 Fase 4:<br>Documentación y Cierre**               | **11–17 Nov**          | Redacción de capítulos de implementación.                                                           | Borrador de capítulos 3–4.                                   | 🔄            |
|                                                        | **18–24 Nov**          | Análisis de resultados y redacción de conclusiones.                                                 | Borrador capítulos finales.                                  | 🔄            |
|                                                        | **25 Nov – 1 Dic**     | Revisión de estilo, citaciones, formato.                                                            | **Reporte Técnico #5:** Primer borrador completo.            | 🔄            |
|                                                        | **2–8 Dic**            | Correcciones con asesor.                                                                            | Documento corregido final.                                   | 🔄            |
|                                                        | **9–15 Dic**           | Preparación de slides de defensa.                                                                   | Slides ejecutivas.                                           | 🔄            |
|                                                        | **16–22 Dic**          | **HITO FINAL**                                                                                      | **Entrega de tesis y documentación.**                        | 🔄            |
|                                                        | **Enero 2026**         | **SUSTENTACIÓN**                                                                                    | Defensa pública.                                             | 🔄            |


--


Fase 1: Investigación y Diseño
- 25–31 Mar: Revisión bibliográfica inicial y definición del plan. Entregable: cronograma inicial y estado del arte preliminar. Estado: Hecho.
- 1–7 Abr: Análisis técnico en profundidad del pattern matcher nativo de WL. Entregable: notebook de análisis con ejemplos de cuellos de botella. Estado: Hecho.
- 8–14 Abr: Definición formal de objetivos y resultados esperados. Entregable: documento de objetivos generales y específicos. Estado: Hecho.
- 15–21 Abr: Redacción de la problemática y justificación técnica. Entregable: Reporte Técnico #1: Problemática y Justificación. Estado: Hecho.
- 22–28 Abr: Investigación de arquitecturas de VM (WASM, regex compilada). Entregable: documento de diseño arquitectónico preliminar. Estado: Hecho.
- 29 Abr–5 May: Diseño del Instruction Set (ISA) y la Representación Intermedia (IR). Entregable: borrador del conjunto de instrucciones y esquema de IR. Estado: Hecho.
- 6–12 May: Redacción del proyecto de investigación completo. Entregable: Reporte Técnico #2: Anteproyecto de Tesis. Estado: Hecho.
- 13–19 May: Preparación de la presentación parcial. Entregable: slides y material de apoyo. Estado: Hecho.
- 20–24 May: Hito de revisión. Entregable: presentación parcial y retroalimentación. Estado: Hecho.

Fase 2: Implementación del Núcleo (Stack VM)
- 27 May–2 Jun: Configuración del entorno de desarrollo (paclet, GitHub, CI). Entregable: repositorio de código público y estructura inicial. Estado: Hecho.
- 3–9 Jun: Implementación del front-end (parser de patrones a IR). Entregable: función PatternToIR para patrones básicos. Estado: Hecho.
- 10–16 Jun: Implementación del compilador de IR a bytecode. Entregable: función IRToBytecode con representación textual. Estado: Hecho.
- 17–23 Jun: Diseño e implementación del loop principal de la VM. Entregable: Reporte Técnico #3: Diseño de la VM. Estado: Hecho.
- 24–30 Jun: Implementación de instrucciones básicas (stack-based). Entregable: ejecutor capaz de correr bytecode simple. Estado: Hecho.
- 1–7 Jul: Integración pipeline completo (Patrón → IR → Bytecode → Ejecución). Entregable: Prototipo Alfa (Stack VM). Estado: Hecho.
- 8–14 Jul: Ampliación ISA para patrones compuestos. Entregable: soporte para f[x_, y_], {a_, b_} en stack VM. Estado: Hecho.
- 15–19 Jul: Hito de revisión. Entregable: Prototipo Funcional (Stack VM). Estado: Hecho.
- Jul–Ago (paralelo): Iteraciones sobre ejecución en stack. Entregable: motor de ejecución funcional. Estado: Hecho.

Fase 2A: Implementación Low-Level (C++)
- Agosto (continuo): Inicio de la implementación en C++ del motor (estructura base, runtime, opcodes). Entregable: repositorio C++ con prototipo funcional. Estado: En progreso.
- Agosto (continuo): Desarrollo de herramientas de debugging: logging, trazas de ejecución, inspección de registros. Entregable: módulo de logging activo dentro de la implementación en C++. Estado: En progreso.

Fase 2B: Transición a VM de Registros
- 26 Ago – 1 Sep: Diseño formal del modelo de registros y comparación con stack. Entregable: documento técnico con modelo de registros y mapeo de opcodes. Estado: Pendiente.
- 2–8 Sep: Migración del motor de ejecución a registros. Entregable: motor iterativo con registros básicos. Estado: Pendiente.
- 9–15 Sep: Reimplementación de opcodes existentes en modelo de registros. Entregable: ejecutor capaz de correr casos simples en registros. Estado: Pendiente.
- 16–22 Sep: Extensión ISA para patrones compuestos/anidados (f[x_, y_], {a_, b_}). Entregable: conjunto de opcodes ampliado y probado. Estado: Pendiente.
- 23–29 Sep: Hito intermedio. Entregable: reporte de equivalencia semántica preliminar (stack vs registros). Estado: Pendiente.

Fase 3: Optimización y Validación
- 30 Sep – 6 Oct: Implementación de optimizaciones básicas en IR/bytecode. Entregable: función OptimizeIR con ejemplos. Estado: Pendiente.
- 7–13 Oct: Sistema de logging y tracing (versión final en C++). Entregable: logs detallados de ejecución. Estado: Pendiente.
- 14–20 Oct: Suite de pruebas (MatchQ vs VM). Entregable: pruebas de equivalencia semántica. Estado: Pendiente.
- 21–27 Oct: Benchmarking preliminar de rendimiento. Entregable: Reporte Técnico #4: Resultados iniciales. Estado: Pendiente.
- 28 Oct – 3 Nov: Optimización de memoria y gestión de registros. Entregable: mejoras medibles en benchmarks. Estado: Pendiente.
- 4–10 Nov: Hito de calidad. Entregable: Versión Beta del matcher con registros. Estado: Pendiente.
Fase 4: Documentación y Cierre
- 11–17 Nov: Redacción de capítulos de implementación. Entregable: borrador de capítulos 3–4. Estado: Pendiente.
- 18–24 Nov: Análisis de resultados y redacción de conclusiones. Entregable: borrador capítulos finales. Estado: Pendiente.
- 25 Nov – 1 Dic: Revisión de estilo, citaciones y formato. Entregable: Reporte Técnico #5: primer borrador completo. Estado: Pendiente.
- 2–8 Dic: Correcciones con asesor. Entregable: documento corregido final. Estado: Pendiente.
9–15 Dic: Preparación de slides de defensa. Entregable: slides ejecutivas. Estado: Pendiente.
- 16–22 Dic: Hito final. Entregable: entrega de tesis y documentación. Estado: Pendiente.
- Enero 2026: Sustentación. Entregable: defensa pública. Estado: Pendiente.