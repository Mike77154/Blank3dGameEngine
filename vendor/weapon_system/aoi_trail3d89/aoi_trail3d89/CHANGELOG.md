# Changelog

## 2.1.0 - Transform provider mode

- Preserved every existing manual `t3d89_emit_*` path.
- Added optional external transform providers.
- Added fixed-point affine move/rotate/scale transforms.
- Added local-to-world conversion for points, velocity, orientation vectors,
  and A/B sockets.
- Added optional explicit-width scaling.
- Added automatic provider sampling from `t3d89_tick()`.
- Added manual provider stepping for custom engine update order.
- Added provider skip/error diagnostics and counters.
- Provider bindings survive `t3d89_reset()` and are removed by destroy/unbind.
- Added provider tests and a standalone provider demo.
