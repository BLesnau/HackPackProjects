# TurretCombined

This is all of the IRTurret projects combined into one thing that can be deployed so that you don't need to keep deploying the code to switch between projects. All turret projects are combined into a single project and can be switched to with commands like `0->1` or `0->2`. The project is split up into multiple files, so you will not be able to use the HackPack web-based code editor. You will need to you use a different editor like the Arduino IDE or Visual Studio Code.

These button pressed switch between different functionality. The links are to the non-combined instance of these projects so that the README documentation can be consolidated to that one location.
- `0->1` [TurretDance](../TurretDance)
- `0->2` [TurretRoulette](../TurretRoulette)
- `0->3` [TurretControl](../TurretControl)

## Known Issues
- TurretDance
  - Memory space runs out pretty quickly because of the actual size of the code paired with the list of the dance moves. This limits the dances to something relatively short. It can be addressed in the future by only putting part of a dacne routine in memory, finishing that portion, deleting the objects for that portion, and then proceeding to the next portion. The work just has not been done yet.