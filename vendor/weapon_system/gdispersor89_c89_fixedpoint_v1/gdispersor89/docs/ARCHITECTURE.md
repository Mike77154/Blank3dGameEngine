# Flujo

```text
Weapon manager
      │ un disparo aceptado
      ▼
GDP89_Request
      │
      ├─ SINGLE -> 1 dirección
      └─ SPREAD -> N direcciones
                    │
                    ▼
              GDP89_Projectile
                    │
                    ├─ projectile spawner
                    ├─ mesh/trail system
                    └─ gprojectiletravel89
```

`spread_fx` es la apertura lineal del cono alrededor del vector `forward`.
La biblioteca normaliza cada dirección con una aproximación fija, sin trigonometría.
