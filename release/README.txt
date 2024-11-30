# Dark Souls III Archipelago Randomizer 3.1.0-alpha.16

This package contains the static randomizer and the DS3 mod for integrating _Dark Souls III_ into the [Archipelago] multiworld randomizer. You can download this from the "Assets" dropdown on [the Releases page]. If you're already reading this on the Releases page, it's just below this documentation. See [the Archipelago DS3 setup guide] for details on how to get this running, and the [game page] for more details on how randomization works with _Dark Souls III_.

[Archipelago]: https://archipelago.gg
[the Releases page]: https://github.com/nex3/Dark-Souls-III-Archipelago-client/releases/
[the Archipelago DS3 setup guide]: https://archipelago.gg/tutorial/Dark%20Souls%20III/setup/en
[game page]: https://archipelago.gg/games/Dark%20Souls%20III/info/en

You can also check out [the changelog] for information about the changes in the latest randomizer release.

[the changelog]: https://github.com/nex3/Dark-Souls-III-Archipelago-client/blog/main/CHANGELOG.md

**Note:** This is a preview release release, with features that will be added to a version the the DS3 Archipelago randomizer in a future stable version. These features are not yet complete, so if you choose to use this release please be aware that it may be more buggy or inconsistent than usual.

## Generating a Multiworld

To use this preview:

* Delete the old Dark Souls 3 world from your Archipelago installation. In the default installation, this is at `C:\ProgramData\Archipelago\lib\worlds\dark_souls_3`.
* Pass the `dark_souls_3.apworld` file included in this release to the Archipelago generator. If you're using the graphical Archipelago app, click "General > Install APWorld" to load it in.
* Copy `Dark Souls III Options Template.yaml` to Archipelago's `Players` directory and update it with your chosen configuration.
* Add any other YAMLs you want for your multiworld, and generate the multiworld ("General > Generate" in the GUI).
* Your multiworld `.zip` file will now be in Archipelago's `output` directory.

Otherwise, the instructions are the same as for the stable release.

## Acknowledgements

This release stands on the shoulders of many people who have worked tirelessly to help you have fun with random Dark Souls. In particular, it uses:

* The original Archipelago mod and server code by Marechal-L and many other contributors, which is the foundation of the whole thing.

* The single-player "static" randomizer by thefifthmatt a.k.a. gracenotes, which is incredibly impressive in its own right.

* ModEngine2 by grayttierney and katalash, which not only makes the "static" randomizer possible in the first place but also makes it easy to combine mods in creative ways.

* All the incredible and thankless reverse engineering, documentation, and tooling work done by countless people at The Grand Archives, in the ?ServerName? discord, and across the internet.

* Debugging help and coding assistance from members of the Archipelago discord server, particularly Exempt-Medic and eldsmith.
