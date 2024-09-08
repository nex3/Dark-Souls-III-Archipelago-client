# Dark Souls III Archipelago Randomizer 3.1.0-alpha.15

This is a preview release of a new architecture for randomizing Dark Souls III for the Archipelago multiworld randomizer. It has a number of major feature improvements over the old 2.x.x line, including:

* Support for randomizing all item locations, not just unique items, without needing progressive pickup lists.

* Support for randomizing items in shops, starting loadouts, Path of the Dragon, and more.

* Built-in integration with the enemy randomizer, including consistent seeding for races and configuration via the web interface and the standard Archipelago YAML file.

* Support for the latest patch for Dark Souls III, 1.15.2. Note that older patches are currently *not* supported, although if ModEngine2 ever adds support they should work fine.

* Optional smooth distribution for upgrade items, upgraded weapons, and soul items so you're more likely to see weaker items earlier and more powerful items later.

* More detailed location names that indicate where a location is, not just what it replaces.

* Other players' item names are visible in DS3.

* If you pick up items while offline, they'll still send once you reconnect.
 
* Many more improvements.

## Generating a Multiworld

To use this preview:

* Delete the old Dark Souls 3 world from your Archipelago installation. In the default installation, this is at `C:\ProgramData\Archipelago\lib\worlds\dark_souls_3`.
* Pass the `dark_souls_3.apworld` file included in this release to the Archipelago generator. If you're using the graphical Archipelago app, click "General > Install APWorld" to load it in.
* Copy `Dark Souls III Options Template.yaml` to Archipelago's `Players` directory and update it with your chosen configuration.
* Add any other YAMLs you want for your multiworld, and generate the multiworld ("General > Generate" in the GUI).
* Your multiworld `.zip` file will now be in Archipelago's `output` directory.

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

## Running the Static Randomizer

Once you've generated a multiworld, uploaded it to archipelago.gg, and started a room for it, you need to run the "static" randomizer (which generates custom mod files for DS3) once to get your game ready for Archipelago. To do so:

* Run `randomizer\DS3Randomizer.exe` in this directory.
* Click "Load Archipelago run" in the lower right.
* Input your Archipelago room URL (such as "archipelago.gg:12345"), your slot name, and your password if you're running a password-protected session.
* Click "Load".

It may take a minute or two to run. Once it's done, it'll say "Archipelago config loaded" in the lower left of the main window.

## Running Dark Souls 3

Now you're ready to start playing! Run `launchmod_darksouls3.bat` to start the game, input your Archipelago URL and slot name in the command prompt, and have fun!

**Note:** Do _not_ run `modengine2_launcher.exe` directly, it will not work.

## Acknowledgements

This release stands on the shoulders of many people who have worked tirelessly to help you have fun with random Dark Souls. In particular, it uses:

* The original Archipelago mod and server code by Marechal-L and many other contributors, which is the foundation of the whole thing.

* The single-player "static" randomizer by thefifthmatt a.k.a. gracenotes, which is incredibly impressive in its own right.

* ModEngine2 by grayttierney and katalash, which not only makes the "static" randomizer possible in the first place but also makes it easy to combine mods in creative ways.

* All the incredible and thankless reverse engineering, documentation, and tooling work done by countless people at The Grand Archives, in the ?ServerName? discord, and across the internet.

* Debugging help and coding assistance from members of the Archipelago discord server, particularly Exempt-Medic and eldsmith.
