# BVH DSL syntax

## Blocks

Blocks open with `-=)` and may close with `(=-`.
The parser is tolerant: a sibling declaration can implicitly end the current node.

```text
hud NAME -=)
  property value
  node NAME -=)
    property value
(=-
```

## Declarations

```text
hud NAME
node NAME
group NAME
animation NAME
```

## Properties

A property is a key plus the remainder of the line.

```text
alpha 0.95
size 640,80
color #00FF66
pos bottom_left + 40,30
bind weapon.loaded "/" weapon.reserve
```

## Anchors

```text
top_left
top_center
top_right
center
bottom_left
bottom_center
bottom_right
```

## Fixed-point numbers

Fractional properties are stored as Q16.16 integers.
For example, `1.0` becomes `65536`, and `0.5` becomes `32768`.
