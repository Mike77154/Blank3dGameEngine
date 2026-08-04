# Changelog

## v18 stabilized patch 1

- `fpi_fixed_cos_deg()` normaliza el ángulo antes del desplazamiento de 90 grados, evitando saturación y pérdida de periodicidad en valores Q16.16 extremos.
- `fpi_cli --tokens` reconoce e imprime RHS sin comillas y entrecomillados, incluidos espacios, comas/dos puntos escapados y valores vacíos.
- Pruebas de regresión para coseno en límites signed de 32 bits.

## v18 stabilized

- Registry permanente con IDs, aliases y callbacks estables.
- Reload transaccional con doble banco.
- AST compacto con pool de RHS.
- Errores estructurados con span y sin truncamiento silencioso.
- Parser estricto frente a texto fuera de reglas.
- Propagación explícita de símbolos sin binding en intérprete y VM.
- Q16.16 para parsing, store, variables y matemática.
- Corrección de bordes signed en `INT32_MIN` para conversión y módulo.
- `polysym` para valores simbólicos definidos por el host.
- Palabra de estado configurable; `state` permanece como default.
- IR real, compilador, programa validado y VM con guardia de generación.
- Two-phase hasta `FPI_MAX_RULES` en intérprete y VM.
- Librerías estáticas separadas, CLI, tests y ejemplo host.
