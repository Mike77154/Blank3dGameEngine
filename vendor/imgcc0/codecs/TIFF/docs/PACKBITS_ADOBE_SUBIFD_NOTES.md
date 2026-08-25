# PackBits tiled writer + Adobe-style SubIFD chains

Esta rama agrega dos cosas importantes encima de `SubIFDs` clásicos/BigTIFF y pyramids tiled:

1. **Writer tiled con `Compression = 32773` (PackBits)**
2. **Variante opcional Adobe-style para `SubIFDs`**

## Tiles + PackBits

El writer ahora permite `Compression = 32773` también cuando una imagen se escribe en **tiles**.

Cobertura:

- `gray8`
- `RGB24`
- `bilevel 1 bpp`

Reglas del path PackBits en writer:

- cada fila visible del tile se comprime por separado
- no se comprime a través del límite de fila
- los edge tiles solo leen los píxeles visibles y no inventan muestras extra
- en bilevel, la entrada sigue siendo **MSB-first** y `fill_order = 2` solo afecta la serialización del TIFF

## SubIFDs en variante Adobe-style

Se añadió:

```c
enum tifx_subifd_style {
    TIFX_SUBIFD_STYLE_TREE = 0,
    TIFX_SUBIFD_STYLE_ADOBE_CHAIN = 1
};
```

Uso:

```c
tifx_write_params_init(&params);
params.subifd_style = TIFX_SUBIFD_STYLE_ADOBE_CHAIN;
```

Comportamiento:

- `TREE`: el padre escribe `SubIFDs` como array de offsets
- `ADOBE_CHAIN`: el padre escribe `SubIFDs` con **un solo offset** al primer hijo y los hermanos se enlazan por `NextIFD`

El parser mantiene navegación por **path explícito**, pero cuando encuentra `SubIFDs` con un solo offset también sigue la cadena `NextIFD` para exponer todos los hermanos del estilo Adobe.

## Ejemplo

`examples/write_subifd_tree.c` ahora genera:

- `subifd_tree_classic.tif`
- `subifd_chain_classic.tif`
- `subifd_tree_big.tif`
- `subifd_chain_big.tif`

Todos en tiles + PackBits.
