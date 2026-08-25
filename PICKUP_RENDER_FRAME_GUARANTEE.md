# Pickup render frame guarantee

The previous render-fix treated `gfo_live` as proof that the pickup GFO had
actually submitted geometry. On the Win32 runtime this could leave an active
pickup invisible: the GFO existed, while the direct fallback skipped it.

Each pickup now stores `render_submit_frame`. The GFO `draw_mesh()` callback
sets it after issuing the mesh. `draw_pickups()` checks the current frame and
submits the mesh directly when the GFO path did not submit on that frame.
Thus an active pickup cannot disappear merely because a lifecycle render event
was skipped or rejected.

The Uzi and magazine prototype recipes were also enlarged and recolored for
visual QA at the default z=24 test distance.
