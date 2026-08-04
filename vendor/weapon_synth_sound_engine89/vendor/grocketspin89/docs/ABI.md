# ABI y memoria

`grs89_state` contiene todo el estado mutable, incluido un buffer de reverb de 4096 muestras de 16 bits. En la compilación de referencia ocupa **8332 bytes** y `grs89_params` ocupa **44 bytes**. No se reserva memoria internamente.

Flujo típico:

```text
caller storage -> grs89_init -> grs89_trigger -> grs89_process_sample
                                      |                    |
                                      +-> grs89_stop <-----+
```

La salida es PCM mono firmado de 16 bits. El motor 3D puede duplicarla a estéreo, panoramizarla, aplicar atenuación por distancia y Doppler.
