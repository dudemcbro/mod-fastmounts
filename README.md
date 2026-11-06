# mod-fastmounts

An [AzerothCore](https://www.azerothcore.org/) (WotLK 3.3.5a) module that speeds up
ground mounts: slow (60%) mounts run at 100% and fast (100%) mounts at 150%.

## What it does

- Sets a fixed run rate for each ground mount tier while the player is mounted:
  `2.0` (100%) for slow mounts and `2.5` (150%) for fast mounts, both configurable.
- Flying mounts and taxi flights are not changed.
- No database changes.

The alternative built into the core, `Rate.MoveSpeed.Player` in `worldserver.conf`,
changes all player movement, not only mounts.

## Requirements

- AzerothCore WotLK (3.3.5a). The module only uses standard core APIs.
- No SQL and no core patches.

## Installation

1. Clone the module into the core's `modules` folder:

   ```sh
   cd <azerothcore>/modules
   git clone https://github.com/dudemcbro/mod-fastmounts.git
   ```

2. Re-run CMake and rebuild the core, for example with `./acore.sh compiler build`.
   AzerothCore compiles everything under `src/` and calls
   `Addmod_fastmountsScripts()`, so no `CMakeLists.txt` is needed.

3. Set up the config file (below) and restart `worldserver`.

## Configuration

The build installs `mod_fastmounts.conf.dist` into the `modules` folder next to
`worldserver.conf`. Copy it to `mod_fastmounts.conf` to change a setting; without it the
defaults are used.

| Option | Default | Description |
| --- | --- | --- |
| `FastMounts.Enable` | `1` | Apply the mount speeds below. |
| `FastMounts.FastLandMountTargetRunRate` | `2.5` | Run rate for fast (100%) ground mounts. `2.5` is 150%. |
| `FastMounts.SlowLandMountTargetRunRate` | `2.0` | Run rate for slow (60%) ground mounts. `2.0` is 100%. |

## Client tooltips

The server change works without any client change. The mount tooltips still show the
stock 60% and 100%, because they come from the client's `Spell.dbc`. A matching client
patch is not part of this repository, since it would contain a modified Blizzard
`Spell.dbc` (about 49 MB). To make one, change the mount speed values in your own
client's `Spell.dbc` and pack it as a `patch-X.MPQ`, or place it loose at
`Data/DBFilesClient/Spell.dbc` and `Data/enUS/DBFilesClient/Spell.dbc`.

## Updating

```sh
cd <azerothcore>/modules/mod-fastmounts
git pull
```

Then re-run CMake, rebuild and restart `worldserver`. Compare your
`mod_fastmounts.conf` with the new `.conf.dist` and add any new options.

To remove the module, delete its folder, re-run CMake and rebuild.

## Contributing

Bug reports and pull requests are welcome on
[GitHub](https://github.com/dudemcbro/mod-fastmounts). For larger changes, please open
an issue first, and describe how you tested your change.

## License

GNU Affero General Public License v3.0, see [LICENSE](LICENSE).
