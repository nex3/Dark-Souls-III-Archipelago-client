## 3.0.10

* Update the expected Archipelago version number to 0.6.1.

## 3.0.9

* Delete the old-style save file so Archipelago doesn't keep reloading it every startup.

* Have error pop-ups default to the "Cancel" button instead of "OK", so players default to not breaking their saves.

* Display error pop-ups over the fullscreen game.

## 3.0.8

* Use _Dark Souls III_'s save data to store per-save Archipelago data. This allows us to ask the user for confirmation before loading a save that doesn't match the room they're connected to.

  **Please note:** If you upgrade to 3.0.8 or later and then load an existing save, you'll see a warning that it's not an Archipelago save even if it was created with an older version of Archipelago. It's safe to click through this warning, and once you do it won't show up again.

* Fix a bug where unsent foreign items in DS3 wouldn't get sent on restart if the player connected to the server before loading the game.

## 3.0.7

* Fix a bug where, if all the items in a given lot *were* randomized, only one would be awarded.

## 3.0.6

* Fix a bug where the static randomizer could crash when placing a foreign item with a very long name whose player had a very short name.

* Fix a bug where, if some but not all items in a given lot were randomized, the unrandomized items would never be awarded.

## 3.0.5

* Overhaul the way the player logs in through the console window. Rather than needing to type in the full room URL and slot name every time, the static randomizer will now write an `apconfig.json` file with the player's room URL, slot name, and optionally password. The player will then be prompted to re-use this upon starting the client, and only have to retype the information if they need to update it.

* Fix a bug where the client always believed the save file to be corrupted and so gave additional copies of every foreign item on each log-in.

* Remove a few leftover debug prints.

## 3.0.4

* Fix another player name bug that caused the static randomizer to fail.

## 3.0.3

* Fix a bug in the new player name trimming where it was only using the player's slotname, not their alias.

## 3.0.2

* Improve the way save data is handled. It's now harder for the `last_received_index` to get corrupted and cause a player to be unable to receive new items, and the mod will automatically recover when this happens.

* Fix some bugs where locations that hold the same item by default could occasionally be swapped with one another.

* Properly include only Sister Friede as a "simple early boss", not the Friede & Ariandel + Blackflame Friede sequence.

* Trim player names before trimming item names when displaying long foreign item names.

* Improve various aspects of the DS3Randomizer.exe UI.

* Update dependencies for better compatibility with the Archipelago server and to avoid annoying warning messages.

## 3.0.1

* Fix a bug where picking up a stack of items that includes Path of the Dragon could cause strange side effects, including having other items not register or crashing the game.

* Change the set of simple early bosses to exclude early/midgame bosses that are actually quite difficult to fight with few upgrades and include late-game bosses that scale down gracefully. The current list is Oceiros, Vordt, Dragonslayer Armour, Ancient Wyvern, Crystal Sage, both Twin Princes, Yhorm, both Gundyrs, Friede 1, and Gael 1.

* Add a checkbox to the static randomizer allowing users to disable the enemy randomizer. It has known upstream bugs that can cause the game to crash if the wrong enemy is loaded into the wrong area, so this is necessary to avoid users becoming softlocked.

* Link to nex3's fork for updates.

## 3.0.0

This release debuts a new architecture for randomizing Dark Souls III for the Archipelago multiworld randomizer. It has a number of major feature improvements over the old 2.x.x line, including:

* Support for randomizing all item locations, not just unique items, without needing progressive pickup lists.

* Support for randomizing items in shops, starting loadouts, Path of the Dragon, and more.

* Built-in integration with the enemy randomizer, including consistent seeding for races and configuration via the web interface and the standard Archipelago YAML file.

* Support for the latest patch for Dark Souls III, 1.15.2. Note that older patches are currently *not* supported, although if ModEngine2 ever adds support they should work fine.

* Optional smooth distribution for upgrade items, upgraded weapons, and soul items so you're more likely to see weaker items earlier and more powerful items later.

* More detailed location names that indicate where a location is, not just what it replaces.

* Other players' item names are visible in DS3.

* If you pick up items while offline, they'll still send once you reconnect.
 
* Many more improvements.

Note that not all YAML fields for Dark Souls III's 2.x.x Archipelago randomizer are supported as-is, and some new fields are available. Consider generating a new YAML configuration using the `Dark Souls III Options Template.yaml` file included in this release.

The following options have been removed:

* `enable_boss_locations` is now controlled by the `soul_locations` option.

* `enable_progressive_locations` was removed because all locations are now individually randomized rather than replaced with a progressive list.

* `pool_type` has been removed. Since there are no longer any non-randomized items in randomized categories, there's not a meaningful distinction between "shuffle" and "various" mode.

* `enable_*_locations` options have all been removed. Instead, there are a number of named location groups that you can add to the `exclude_locations` option to prevent them from containing important items. These location groups are:

  * Prominent: A small number of locations that are in very obvious locations. Mostly boss drops. Ideal for setting as priority locations.
  * Progression: Locations that contain items in vanilla which unlock other locations.
  * Boss Rewards: Boss drops. Does not include soul transfusions or shop items.
  * Miniboss Rewards: Miniboss drops. Only includes enemies considered minibosses by the enemy randomizer.
  * Mimic Rewards: "Drops from enemies that are mimics in vanilla.
  * Hostile NPC Rewards: "Drops from NPCs that are hostile to you. This includes scripted invaders and initially-friendly NPCs that must be fought as part of their quest.
  * Friendly NPC Rewards: Items given by friendly NPCs as part of their quests or from non-violent interaction.
  * Upgrade: Locations that contain upgrade items in vanilla, including titanite, gems, and Shriving Stones.
  * Small Souls: Locations that contain soul items in vanilla, not including boss souls.
  * Boss Souls: Locations that contain boss souls in vanilla, as well as Soul of Rosaria.
  * Unique: Locations that contain items in vanilla that are unique per NG cycle, such as scrolls, keys, ashes, and so on. Doesn't cover equipment, spells, or souls.
  * Healing: Locations that contain Undead Bone Shards and Estus Shards in vanilla.
  * Miscellaneous: Locations that contain generic stackable items in vanilla, such as arrows, firebombs, buffs, and so on.
  * Hidden: Locations that are particularly difficult to find, such as behind illusory walls, down hidden drops, and so on. Does not include large locations like Untended Graves or Archdragon Peak.
  * Weapons: Locations that contain weapons in vanilla.
  * Shields: Locations that contain shields in vanilla.
  * Armor: Locations that contain armor in vanilla.
  * Rings: Locations that contain rings in vanilla.
  * Spells: Locations that contain spells in vanilla.

  By default, the Hidden, Small Crystal Lizards, Upgrade, Small Souls, and Miscellaneous groups are in `exclude_locations`. Once you've chosen your excluded locations, you can set `excluded_locations: unrandomized` to preserve the default vanilla item placements for all excluded locations.

In addition, the following options have changed:

* The location names used in options like `exclude_locations` have changed. A full description of each location is included in `dark_souls_3.apworld`. To see it, run Archipelago's WebHost and go to http://localhost/tutorial/Dark%20Souls%20III/locations/en.