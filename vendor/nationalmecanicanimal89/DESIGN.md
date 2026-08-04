# Diseño interno de NationalMecanicanimal89

## Separación de responsabilidades

### Pieza lógica

Una `nm89_part` describe parentesco, pose y enlaces a recursos compartidos. No conoce vértices ni APIs gráficas.

### Recursos compartidos

- `nm89_constraint`: límites por canal.
- `nm89_pivot`: punto de giro.
- `nm89_alignment`: ajuste permanente de una geometría.

Esta separación evita repetir nueve límites y transformaciones de bind en cada pieza.

### Pose solicitada y resuelta

`requested` conserva la intención combinada de manual, clips y transform provider.

`resolved` conserva el resultado después de constraints. Esto permite inspeccionar intentos bloqueados sin corromper la mecánica.

### Binding

Un binding enlaza una pieza lógica con un recurso/selector. Una pieza puede poseer varios bindings y una geometría no define la jerarquía.

## Registro de providers

La librería guarda copias pequeñas de hasta `NM89_MAX_PROVIDERS` estructuras. El `user` sigue perteneciendo al caller y debe vivir mientras el rig lo use.

Los callbacks de salida y eventos son broadcast: todos los providers registrados que implementen la capacidad reciben el paquete.

Las capacidades de entrada se enlazan explícitamente por pieza mediante provider ID y source ID.

## Garantía mecánica

El orden normal es:

```text
manual -> clip -> transform provider -> constraints
```

Un transform provider no recibe acceso directo a `resolved`. El constraint dinámico en modo INTERSECT se ejecuta después del constraint estático.

## Convención matemática

El fallback usa matrices 4x4 Q16.16 y transformaciones TRS con pivote. Las rotaciones están expresadas en grados Q16.16.

El math provider puede reemplazar identity, multiply, compose y point transform para adaptar orden, handedness o una implementación propia del engine.

## Política de memoria

No existe ownership dinámico. El rig contiene todos los bancos. Los IDs son índices estables hasta reinicializar el rig.

La aplicación controla memoria con las macros `NM89_MAX_*`.
