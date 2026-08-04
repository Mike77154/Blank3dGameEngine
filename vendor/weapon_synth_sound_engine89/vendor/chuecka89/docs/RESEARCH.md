# Research map

The implementation translates documented mechanical event order into compact procedural audio gestures. These sources describe operation and are not redistributed in this package.

## Pump shotgun

Mossberg pump-action owner manual:

- magazine loading pushes the shell until its rim passes the cartridge stop;
- rearward forearm travel ejects a shell/case;
- forward forearm travel chambers the next shell.

Source:

https://resources.mossberg.com/hubfs/manuals/12173-Owners-Manual-Pumps-English.pdf

Design mapping:

```text
shell sliding against port/tube -> dark scrape envelope
rim crossing cartridge stop     -> short stop/latch transient
forearm rearward                -> brighter CHECK
forearm forward + bolt lock     -> darker, heavier CHECK
```

## Semiautomatic pistol

GLOCK instructions describe pulling the slide fully rearward and releasing it so it returns to the forward position.

Source:

https://eu.glock.com/-/media/Global/EU/GLOCK-GmbH-2019/ContactandSupport/Download-Area/Instructions-for-use_Relaunch_G44_BT01-Set_En_042021.pdf

Design mapping:

```text
rearward hand friction -> bright short scrape
spring-driven return   -> shorter scrape + harder stop
```

## Repeating feed/chamber/lock cycle

The U.S. Army EIB/ESB handbook describes a charging cycle as extracting/ejecting the previous cartridge and feeding/chambering/locking a new round.

Source:

https://home.army.mil/wainwright/2616/1834/9100/1-25_EIB_ESB_Handbook_final.pdf

Design mapping:

```text
extract/eject       -> brighter edge
feed/chamber/lock   -> darker stop
repeated cycle      -> sheke-sheke sequence
```

## Revolver

Smith & Wesson describes revolver chambers as being successively aligned with the barrel; in double-action operation the trigger advances the cylinder while cocking and releasing the hammer.

Source:

https://ir.smith-wesson.com/static-files/cceb2980-5804-45ba-8f6e-c01db8c2eb17

Design mapping:

```text
cylinder indexing -> repeated short ticks
final closure     -> darker latch transient
```

## Procedural-audio rationale

Garcia Sihuay and Reiss describe procedural audio as algorithmic, real-time sound generation and note benefits including variation, lower stored-audio requirements, and interactivity.

Source:

https://arxiv.org/pdf/2501.17198

`chuecka89` therefore exposes deterministic variation and editable control parameters instead of embedding recordings.
