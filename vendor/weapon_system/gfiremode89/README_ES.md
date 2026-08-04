# gfiremode89 — automatización de disparo

Se encarga exclusivamente de decidir **cuándo el gatillo solicita un disparo**:

- `semi`: una solicitud por pulsación;
- `auto`: solicitudes continuas mientras el botón siga abajo;
- `hold_once`: una sola solicitud hasta soltar;
- `burst`: ráfaga de cantidad configurable;
- cadencia mediante milisegundos enteros.

La decisión usa dos fases: `gfm89_request()` solicita y el host llama `gfm89_commit_fire()` sólo cuando munición y recarga permiten disparar. Si no puede, usa `gfm89_reject_fire()`.

No conoce cargadores, reserva, proyectiles, actores, físicas ni render.
