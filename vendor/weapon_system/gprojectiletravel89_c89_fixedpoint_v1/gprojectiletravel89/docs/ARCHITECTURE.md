# Flujo

```text
GDP89_Projectile / projectile spawner
                  │
                  ▼
             GPT89_Desc
                  │
                  ▼
             GPT89_State
                  │
       ┌──────────┼────────────┐
       ▼          ▼            ▼
    life_ms   max_distance   destination
       └──────────┴────────────┘
                  │ primera condición
                  ▼
            GPT89_EndReason
```

La biblioteca no resuelve impactos ni rebotes. Es un limitador de viaje reutilizable.
En modo externo puede trabajar encima de cualquier proveedor de física o transform.
