# gmagazine89 — cargador

Se encarga exclusivamente del **cargador actualmente instalado**:

- capacidad;
- cartuchos restantes;
- gasto por disparo;
- consulta de lleno, vacío y faltantes;
- carga parcial o llenado completo.

No maneja inventario de reserva, tiempo de recarga, gatillo, proyectiles ni actores. Una capacidad de `0` significa que el arma usa un proveedor externo y devuelve `GMAG89_BYPASS`.
