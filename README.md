mod-fastmounts
==============

Sets ground mounts to fixed speed targets by mount tier and ships a matching client display patch.

Default behavior
----------------
- Enabled by default.
- Fast (100%) ground mounts are set to run rate `2.5` (150%).
- Slow (60%) ground mounts are set to run rate `2.0` (100%).
- Does not affect flying mounts or taxi flight.

Configuration
-------------
File: `conf/mod_fastmounts.conf.dist`

- `FastMounts.Enable = 1`
- `FastMounts.FastLandMountTargetRunRate = 2.5`
- `FastMounts.SlowLandMountTargetRunRate = 2.0`

Client Display Patch
--------------------

The patch files are not in this repository (each holds a modified Blizzard `Spell.dbc`,
about 49 MB). They live on the server in `/data/modules/mod-fastmounts/data/client/`.

To align tooltip display with the server behavior (100% slow, 150% fast):

- Use the included loose DBC at `data/client/patch-contents/dbc/Spell.dbc`.
- Or use the drop-in package `data/client/client-dropin.tar.gz` and extract into your WoW client root.

The drop-in package places:

- `Data/DBFilesClient/Spell.dbc`
- `Data/enUS/DBFilesClient/Spell.dbc`

Notes
-----
- This module uses script hooks and does not require DB edits.
- Alternative built-in setting: `Rate.MoveSpeed.Player` in `worldserver.conf` affects all player movement, not just mounts.
