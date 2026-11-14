# Coordinate audit candidate index

Generated from tracked C++ at `27dff9a32608`. Each row is a search candidate,
not a confirmed defect. Line numbers can shift after this baseline.

Use `python3 specs/toroidal-map/tools/audit_coordinates.py --path <path>`
to regenerate full matching lines. Review status covers this snapshot only;
regenerating the index resets manually recorded statuses.

| File | Matching lines | Review |
| --- | --- | --- |
| `AI/BattleAI/AttackPossibility.cpp` | 247 | unreviewed |
| `AI/BattleAI/BattleEvaluator.cpp` | 589, 985, 1023, 1027 | unreviewed |
| `AI/BattleAI/BattleExchangeVariant.cpp` | 374, 392, 461, 496, 568, 1007, 1073 | unreviewed |
| `AI/BattleAI/BattleExchangeVariant.h` | 212 | unreviewed |
| `AI/BattleAI/ThreatMap.cpp` | 45, 58 | unreviewed |
| `AI/MMAI/BAI/v13/battlefield_v13.cpp` | 335, 436 | unreviewed |
| `AI/MMAI/BAI/v13/nn_model_v13.cpp` | 207, 208 | unreviewed |
| `AI/MMAI/BAI/v13/render_v13.cpp` | 833 | unreviewed |
| `AI/MMAI/BAI/v15/fastbfs_v15.h` | 167, 218 | unreviewed |
| `AI/MMAI/BAI/v15/graph/edge_store_v15.h` | 242 | unreviewed |
| `AI/MMAI/BAI/v15/graph/edges/hex_adjacent_hex_v15.h` | 38, 39 | unreviewed |
| `AI/MMAI/BAI/v15/graph/node_store_v15.h` | 177 | unreviewed |
| `AI/MMAI/BAI/v15/graph/nodes/unit_v15.cpp` | 328 | unreviewed |
| `AI/MMAI/BAI/v15/nn_model_v15.cpp` | 235, 249 | unreviewed |
| `AI/MMAI/BAI/v15/render_v15.cpp` | 675 | unreviewed |
| `AI/MMAI/BAI/v15/state_v15.cpp` | 347, 1014, 1193, 1601, 1613, 1626, 1665, 1757, 1758, 1840, 2430 | unreviewed |
| `AI/MMAI/BAI/v15/verify_v15.cpp` | 866, 966, 978 | unreviewed |
| `AI/MMAI/common.h` | 77 | unreviewed |
| `AI/Nullkiller2/AIUtility.cpp` | 384 | reviewed candidate: artifact relevance constant, not map coordinates |
| `AI/Nullkiller2/AIUtility.h` | 144, 145, 147, 149, 160, 165, 172, 177 | unreviewed |
| `AI/Nullkiller2/Analyzers/DangerHitMapAnalyzer.cpp` | 81, 83, 133, 219, 221, 257 | reviewed candidates: map dimensions allocate the hit map; distance limits bound pathfinder turns |
| `AI/Nullkiller2/Analyzers/HeroManager.cpp` | 146 | unreviewed |
| `AI/Nullkiller2/Analyzers/ObjectClusterizer.cpp` | 22, 37, 60, 61, 97, 103, 496 | partial: Dimension Door reach and cluster representative fixed and tested after baseline; other candidates unreviewed |
| `AI/Nullkiller2/Behaviors/EscapeBehavior.cpp` | 186 | reviewed candidate: map dimensions bound enumeration of pathfinder results |
| `AI/Nullkiller2/Behaviors/ExplorationBehavior.cpp` | 64, 65, 216, 217, 878 | reviewed candidates: canonical same-level object ordering uses wrapped displacement, fallback handles other levels; portal limits are path turns; neighbor exploration uses topology-aware enumeration |
| `AI/Nullkiller2/Behaviors/GatherArmyBehavior.cpp` | 235, 297 | reviewed candidates: compare pathfinder turn counts, not coordinate distances |
| `AI/Nullkiller2/Behaviors/StartupBehavior.cpp` | 55, 105 | reviewed: nearest-hero threshold uses topology; map width scales recruitment, not distance |
| `AI/Nullkiller2/Engine/Nullkiller.cpp` | 378, 380 | unreviewed |
| `AI/Nullkiller2/Engine/PriorityEvaluator.cpp` | 1651 | unreviewed |
| `AI/Nullkiller2/Engine/Settings.cpp` | 29, 30, 31 | unreviewed |
| `AI/Nullkiller2/Engine/Settings.h` | 50, 51, 52 | unreviewed |
| `AI/Nullkiller2/Goals/AdventureSpellCast.cpp` | 77 | unreviewed |
| `AI/Nullkiller2/Goals/ExploreNeighbourTile.cpp` | 26, 43, 56, 89, 94 | reviewed candidates: target selection enumerates neighbors through topology-aware helper; other matches score discovery and path cost |
| `AI/Nullkiller2/Goals/ExploreNeighbourTile.h` | 44, 55 | unreviewed |
| `AI/Nullkiller2/Helpers/ExplorationHelper.cpp` | 41, 42, 43, 44, 45, 70, 72, 76, 117, 118, 124, 166, 177, 179, 194, 195, 205, 246, 613, 647, 665, 666, 668, 669, 726, 773, 784, 788 | unreviewed |
| `AI/Nullkiller2/Helpers/ExplorationHelper.h` | 51, 112 | unreviewed |
| `AI/Nullkiller2/Pathfinding/AINodeStorage.cpp` | 47, 53, 418, 428, 1001, 1185, 1195, 1256, 1257, 1258, 1259, 1260, 1413 | partial: nearest-town and Dimension Door planning fixed and tested after baseline; other candidates unreviewed |
| `AI/Nullkiller2/Pathfinding/AINodeStorage.h` | 241, 310, 319, 320 | unreviewed |
| `AI/Nullkiller2/Pathfinding/AIPathfinder.cpp` | 97, 98 | reviewed candidates: configure limits in pathfinder turns, not map coordinate distances |
| `AI/Nullkiller2/Pathfinding/AIPathfinder.h` | 35, 36 | unreviewed |
| `AI/Nullkiller2/Pathfinding/ObjectGraph.cpp` | 52 | unreviewed |
| `AI/Nullkiller2/Pathfinding/ObjectGraphCalculator.cpp` | 59, 68, 91, 108, 138 | reviewed candidates: neighbor iteration uses topology-aware foreach_neighbour; cost comes from pathfinder |
| `AI/Nullkiller2/Pathfinding/ObjectGraphCalculator.h` | 43, 44 | unreviewed |
| `AI/Nullkiller2/Pathfinding/Rules/AIMovementAfterDestinationRule.cpp` | 86, 158 | reviewed candidates: distance-limit helper compares path turn counts |
| `AI/Nullkiller2/pforeach.h` | 12, 21, 23 | unreviewed |
| `AI/StupidAI/StupidAI.cpp` | 98, 214, 218, 332 | reviewed candidates: combat hex geometry, outside adventure-map topology |
| `client/ClientCommandManager.cpp` | 378, 379, 397, 398 | unreviewed |
| `client/GameEngine.cpp` | 259, 261, 263 | unreviewed |
| `client/adventureMap/AdventureMapInterface.cpp` | 232, 235, 238, 241, 254, 266, 272 | unreviewed |
| `client/adventureMap/AdventureMapShortcuts.cpp` | 578 | unreviewed |
| `client/adventureMap/AdventureMapWidget.cpp` | 113, 117, 121, 124, 130, 365, 451 | unreviewed |
| `client/adventureMap/CInGameConsole.cpp` | 58 | unreviewed |
| `client/adventureMap/CInfoBar.cpp` | 169, 173, 177, 181, 349 | unreviewed |
| `client/adventureMap/CInfoBar.h` | 41, 42, 61, 62 | unreviewed |
| `client/adventureMap/CList.cpp` | 217, 230, 288, 289, 297, 298, 329, 330, 336, 341 | unreviewed |
| `client/adventureMap/CMinimap.cpp` | 79, 82, 83, 91, 114, 115, 117, 118, 132, 134, 135, 142, 144, 145, 147, 148, 155, 208, 233, 234, 235, 243, 244, 245, 246 | unreviewed |
| `client/adventureMap/CResDataBar.cpp` | 39, 40, 108 | unreviewed |
| `client/adventureMap/MapAudioPlayer.cpp` | 85, 87, 99, 114, 175, 176, 210 | partial: wrapped attenuation and object source cells fixed and tested after baseline; audible playback unverified |
| `client/adventureMap/TurnTimerWidget.cpp` | 40, 52, 75 | unreviewed |
| `client/battle/BattleAnimationClasses.cpp` | 303, 311, 403, 404, 408, 409, 473, 712, 732, 733, 737, 738, 742, 743, 1008, 1009, 1010, 1049, 1052, 1055, 1057, 1175 | unreviewed |
| `client/battle/BattleConsole.cpp` | 56, 57, 94, 134, 140, 141, 152 | unreviewed |
| `client/battle/BattleConstants.h` | 45 | unreviewed |
| `client/battle/BattleFieldController.cpp` | 175, 176, 283, 324, 478, 575, 576, 642, 756, 770, 784, 791, 803, 849, 875, 879, 928, 929, 1000, 1028, 1072, 1161, 1162, 1163, 1177, 1178, 1197, 1205, 1207, 1208, 1220, 1221, 1224, 1225, 1228, 1229, 1230, 1231, 1232, 1238, 1241, 1248, 1252 | unreviewed |
| `client/battle/BattleFieldController.h` | 127, 187 | unreviewed |
| `client/battle/BattleHero.cpp` | 69, 71, 139, 189 | unreviewed |
| `client/battle/BattleObstacleController.cpp` | 181, 267, 273 | unreviewed |
| `client/battle/BattleOverlayLogVisualizer.cpp` | 38 | unreviewed |
| `client/battle/BattleProjectileController.cpp` | 34, 35, 57, 58, 114, 116, 117, 124, 125, 279, 280, 298, 375, 409, 415, 416, 418, 419, 427, 429, 432, 433, 434, 441, 448, 463 | unreviewed |
| `client/battle/BattleProjectileController.h` | 132 | unreviewed |
| `client/battle/BattleResultWindow.cpp` | 128, 154, 155 | unreviewed |
| `client/battle/BattleStacksController.cpp` | 333, 343, 834, 835, 840, 842, 850, 855 | unreviewed |
| `client/battle/BattleWindow.cpp` | 162, 208, 212, 354, 425, 426, 439, 443, 459, 460, 467, 468, 475, 476, 483, 484, 1010 | unreviewed |
| `client/battle/CreatureAnimation.cpp` | 156, 248, 249 | unreviewed |
| `client/battle/CreatureAnimation.h` | 55 | unreviewed |
| `client/battle/StackInfoBasicPanel.cpp` | 31, 87, 108, 110, 121 | unreviewed |
| `client/battle/StackQueue.cpp` | 46, 47, 51, 52, 58, 59 | unreviewed |
| `client/filters/FSR/fsr_rcas.cpp` | 33, 34, 35, 39, 56, 57, 61, 77, 78, 79, 80, 101 | unreviewed |
| `client/filters/xBRZ/xbrz.cpp` | 307, 309, 310, 418, 419, 439, 445, 446, 1098, 1110, 1130, 1137, 1255, 1258, 1260, 1276, 1279 | unreviewed |
| `client/filters/xBRZ/xbrz.h` | 54, 72 | unreviewed |
| `client/filters/xBRZ/xbrz_tools.h` | 70, 72, 107, 109 | unreviewed |
| `client/globalLobby/GlobalLobbyWidget.cpp` | 132 | unreviewed |
| `client/gui/CIntObject.cpp` | 174, 175, 176, 177, 184, 185, 193, 234, 295, 305, 306 | unreviewed |
| `client/gui/EventDispatcher.cpp` | 307, 317, 323 | unreviewed |
| `client/gui/InterfaceObjectConfigurable.cpp` | 374, 690, 693, 695, 698, 700 | unreviewed |
| `client/lobby/BattleOnlyModeTab.cpp` | 245, 248, 259, 260, 471, 569, 666, 786 | unreviewed |
| `client/lobby/CBonusSelection.cpp` | 123, 499, 697, 777, 787 | unreviewed |
| `client/lobby/CSelectionBase.cpp` | 162, 163, 267, 418, 422, 423, 424, 460, 511, 527, 578, 599, 604 | unreviewed |
| `client/lobby/OptionsTab.cpp` | 724, 725, 876, 1056, 1057 | unreviewed |
| `client/lobby/RandomMapTab.cpp` | 272, 276, 277, 376, 383, 786, 867, 870 | unreviewed |
| `client/lobby/SelectionTab.cpp` | 250, 251, 353, 577, 583, 615, 827, 878, 912, 935, 1097, 1099, 1406, 1411, 1437, 1464 | unreviewed |
| `client/mainmenu/CCampaignScreen.cpp` | 149, 150 | unreviewed |
| `client/mainmenu/CHighScoreScreen.cpp` | 63 | unreviewed |
| `client/mainmenu/CMainMenu.cpp` | 879 | unreviewed |
| `client/mainmenu/CPrologEpilogVideo.cpp` | 35, 83 | unreviewed |
| `client/mainmenu/CStatisticScreen.cpp` | 271, 400, 401, 403, 404, 430, 431, 432, 435, 460, 501, 513, 533, 563, 573, 594 | unreviewed |
| `client/mapView/IMapRendererContext.h` | 39 | unreviewed |
| `client/mapView/MapOverlayLogVisualizer.cpp` | 85, 90 | unreviewed |
| `client/mapView/MapRenderer.cpp` | 52, 56, 253, 255, 293, 298, 313, 316, 319, 322, 374, 594, 870, 871, 934, 935, 936, 937, 941, 972, 1017 | unreviewed |
| `client/mapView/MapRendererContext.cpp` | 123, 186, 451, 486, 500, 501, 511, 617, 670 | unreviewed |
| `client/mapView/MapRendererContext.h` | 31 | unreviewed |
| `client/mapView/MapRendererContextState.cpp` | 45, 48, 50, 51, 110, 158 | unreviewed |
| `client/mapView/MapView.cpp` | 67, 130 | unreviewed |
| `client/mapView/MapViewActions.cpp` | 58, 69, 86, 161 | unreviewed |
| `client/mapView/MapViewCache.cpp` | 128, 158, 159, 238, 271, 272, 274, 282, 283, 284, 289, 293, 306, 307, 308, 309, 311, 312, 456, 458, 461, 481, 487, 492, 494, 502, 513 | partial: cache-band geometry extracted and tested at seams and multiple zooms; actual rendering and other candidates unreviewed |
| `client/mapView/MapViewController.cpp` | 90, 91, 94, 95, 110, 136, 138 | unreviewed |
| `client/mapView/MapViewModel.cpp` | 97, 98, 114, 138, 139, 142, 143, 153 | unreviewed |
| `client/mapView/mapHandler.cpp` | 93, 95 | unreviewed |
| `client/mapView/mapHandler.h` | 43 | unreviewed |
| `client/render/AssetGenerator.cpp` | 321, 461, 462, 491, 492, 619, 620, 718, 719, 720, 721, 728, 729, 730, 731, 789, 821, 822, 881, 1067, 1068, 1069, 1236, 1237, 1259, 1266, 1277, 1279, 1312, 1313, 1366, 1373, 1374, 1399, 1400, 1409, 1410, 1420, 1421, 1501, 1503, 1505, 1513, 1534, 1544, 1545, 1566, 1568, 1569, 1640, 1653, 1699, 1711, 1747, 1750, 1751, 1759, 1764 | unreviewed |
| `client/render/CDefFile.cpp` | 56, 57, 111, 115, 116, 123, 152, 181, 206, 209, 283, 284 | unreviewed |
| `client/render/CDefFile.h` | 34 | unreviewed |
| `client/render/IFont.cpp` | 48, 52, 65, 71, 81, 88, 93, 100, 105 | unreviewed |
| `client/render/IFont.h` | 54 | unreviewed |
| `client/render/hdEdition/HdImageLoader.cpp` | 159, 160, 169 | unreviewed |
| `client/render/hdEdition/PakLoader.cpp` | 119, 120 | unreviewed |
| `client/render/hdEdition/PakLoader.h` | 31, 32 | unreviewed |
| `client/replay/GameplayReplayer.cpp` | 173 | unreviewed |
| `client/replay/ReplayAbortOverlay.cpp` | 66 | unreviewed |
| `client/widgets/Buttons.cpp` | 41, 364, 365 | unreviewed |
| `client/widgets/CArtifactsOfHeroBackpack.cpp` | 51, 113 | unreviewed |
| `client/widgets/CArtifactsOfHeroBase.cpp` | 61 | unreviewed |
| `client/widgets/CComponent.cpp` | 117, 121, 122, 125, 465, 470, 497, 498, 511, 514, 523, 526, 535, 538, 540, 542, 544, 547, 553, 568, 577, 594, 612 | unreviewed |
| `client/widgets/CComponent.h` | 109 | unreviewed |
| `client/widgets/CComponentHolder.cpp` | 135 | unreviewed |
| `client/widgets/CGarrisonInt.cpp` | 461, 462, 498, 508, 590, 594, 600, 604, 793 | unreviewed |
| `client/widgets/CTextInput.cpp` | 111 | unreviewed |
| `client/widgets/CViewport.cpp` | 26, 27, 127, 144, 169, 219, 220, 322, 337, 338 | unreviewed |
| `client/widgets/ComboBox.cpp` | 81 | unreviewed |
| `client/widgets/ControllerActionButton.cpp` | 39, 78, 79, 80 | unreviewed |
| `client/widgets/ControllerPrompt.cpp` | 89 | unreviewed |
| `client/widgets/CreatureCostBox.cpp` | 22 | unreviewed |
| `client/widgets/GraphicalPrimitiveCanvas.cpp` | 23, 24, 73, 77, 82 | unreviewed |
| `client/widgets/GraphicalPrimitiveCanvas.h` | 47 | unreviewed |
| `client/widgets/Images.cpp` | 46, 47, 48, 65, 66, 71, 72, 132, 133, 217, 218, 230, 231, 358, 359, 366, 367, 417, 421, 422 | unreviewed |
| `client/widgets/Images.h` | 199 | unreviewed |
| `client/widgets/MiscWidgets.cpp` | 81, 134, 136, 208, 239, 244, 300, 396, 479, 578, 670, 683, 684, 729, 735, 744, 745, 760, 763 | unreviewed |
| `client/widgets/MiscWidgets.h` | 266, 268, 271 | unreviewed |
| `client/widgets/ObjectLists.cpp` | 47, 88, 105, 117, 213 | unreviewed |
| `client/widgets/ObjectLists.h` | 45, 46, 72, 73, 78 | unreviewed |
| `client/widgets/RadialMenu.cpp` | 56, 94 | unreviewed |
| `client/widgets/Scrollable.cpp` | 76, 77, 113 | unreviewed |
| `client/widgets/Scrollable.h` | 15, 89 | unreviewed |
| `client/widgets/Slider.cpp` | 40, 42, 53, 59, 97, 99, 159, 163, 171, 175, 199, 202, 207, 236, 238, 245 | unreviewed |
| `client/widgets/TextControls.cpp` | 53, 54, 112, 114, 164, 222, 225, 228, 231, 234, 237, 273, 295, 319, 350, 357, 358, 360, 392, 393, 460, 591, 599 | unreviewed |
| `client/widgets/TextControls.h` | 59 | unreviewed |
| `client/widgets/VideoWidget.cpp` | 33 | unreviewed |
| `client/windows/CCastleInterface.cpp` | 216, 217, 316, 416, 452, 453, 464, 465, 790, 837, 842, 844, 846, 1339, 1361, 1463, 1479, 1480, 1674, 1870, 1871, 2097, 2098, 2107, 2108, 2231, 2232, 2266, 2268, 2270, 2272, 2274, 2415, 2432 | unreviewed |
| `client/windows/CCreatureWindow.cpp` | 208, 258, 259, 260, 309, 310, 311, 312, 330, 331, 614, 723, 724, 735, 756 | unreviewed |
| `client/windows/CExchangeWindow.cpp` | 119, 121, 144, 149, 154 | unreviewed |
| `client/windows/CHeroBackpackWindow.cpp` | 67, 73, 146 | unreviewed |
| `client/windows/CHeroOverview.cpp` | 72, 73, 82, 99, 112, 113, 130, 163, 195, 203, 210, 228 | unreviewed |
| `client/windows/CHeroWindow.cpp` | 66, 169, 180, 182, 183 | unreviewed |
| `client/windows/CJournalWindow.cpp` | 138, 165 | unreviewed |
| `client/windows/CKingdomInterface.cpp` | 66, 504, 707, 709, 731, 737 | unreviewed |
| `client/windows/CMapOverview.cpp` | 62, 65, 66, 147 | unreviewed |
| `client/windows/CMarketWindow.cpp` | 149 | unreviewed |
| `client/windows/CMessage.cpp` | 73, 125, 128, 131, 132, 144, 156, 199, 285, 338, 339, 384, 402, 403, 404, 413, 417, 418, 419, 428, 433, 434, 435 | unreviewed |
| `client/windows/CPuzzleWindow.cpp` | 61 | unreviewed |
| `client/windows/CScenarioEventJournal.cpp` | 38, 39 | unreviewed |
| `client/windows/CSpellWindow.cpp` | 67, 201, 220, 246, 247, 248, 249, 250, 260, 261, 264, 265, 273, 279, 283, 593, 671 | unreviewed |
| `client/windows/CWindowObject.cpp` | 111, 112, 116, 118, 119, 124, 126, 127, 140, 141, 142, 143, 146, 147, 148, 160, 208, 209, 248 | unreviewed |
| `client/windows/CWindowWithArtifacts.cpp` | 146 | unreviewed |
| `client/windows/CreaturePurchaseCard.cpp` | 37, 42, 47, 56, 57, 64, 65, 77, 82, 106, 119, 120 | unreviewed |
| `client/windows/GUIClasses.cpp` | 656, 816, 948, 949, 1001, 1067, 1068, 1154, 1155, 1229, 1298, 1302, 1729, 1777, 1899, 1900, 1977 | unreviewed |
| `client/windows/InfoWindows.cpp` | 116, 168, 266, 294, 384, 411, 412, 427, 428, 432, 435, 437 | unreviewed |
| `client/windows/QuickRecruitmentWindow.cpp` | 60, 63, 68, 69, 76 | unreviewed |
| `client/windows/settings/KeyBindingsWindow.cpp` | 60, 63, 152, 153, 154, 201, 202, 203 | unreviewed |
| `client/windows/settings/SettingsMainWindow.cpp` | 187 | unreviewed |
| `client/windows/wiki/WikiArtifactContent.cpp` | 218 | unreviewed |
| `client/windows/wiki/WikiCreatureContent.cpp` | 105 | unreviewed |
| `client/windows/wiki/WikiHeroContent.cpp` | 232 | unreviewed |
| `client/windows/wiki/WikiModContent.cpp` | 212, 213, 243, 244, 278, 279, 314, 315, 347, 348, 379, 380 | unreviewed |
| `client/windows/wiki/WikiTownContent.cpp` | 84, 85, 118, 119, 200, 210, 228, 231, 306, 309 | unreviewed |
| `client/windows/wiki/WikiWindow.cpp` | 123, 129, 163, 194, 213, 250, 251, 255, 258, 904, 913, 947, 1525 | unreviewed |
| `client/windows/wiki/markdown/MDParser.cpp` | 158, 177, 403 | unreviewed |
| `client/windows/wiki/markdown/MDRenderer.cpp` | 89, 104, 153, 302, 365, 366, 379, 380, 394, 441, 443, 455, 457, 528, 530, 548, 552 | unreviewed |
| `client/windows/wiki/markdown/MDRenderer.h` | 101 | unreviewed |
| `client/windows/wiki/markdown/MDWidgets.cpp` | 41, 42, 137, 148, 149, 173 | unreviewed |
| `lib/CStack.cpp` | 96 | unreviewed |
| `lib/IHandlerBase.h` | 42 | unreviewed |
| `lib/ObstacleHandler.cpp` | 98, 99 | unreviewed |
| `lib/Point.h` | 41, 58, 63, 82 | unreviewed |
| `lib/Rect.cpp` | 42, 44, 94, 97, 98, 99, 100, 137, 139, 140 | unreviewed |
| `lib/Rect.h` | 112, 117, 156 | unreviewed |
| `lib/battle/BattleAttackInfo.cpp` | 20 | unreviewed |
| `lib/battle/BattleHex.cpp` | 14, 25, 42, 49, 51, 54, 56, 59, 61 | unreviewed |
| `lib/battle/BattleHex.h` | 196, 213, 239, 242, 245 | unreviewed |
| `lib/battle/BattleHexArray.cpp` | 37, 57, 72, 110, 111, 114, 115 | unreviewed |
| `lib/battle/BattleHexArray.h` | 195, 206, 217, 320, 321, 322 | unreviewed |
| `lib/battle/BattleInfo.cpp` | 232, 252, 256, 285, 489 | unreviewed |
| `lib/battle/CBattleInfoCallback.cpp` | 59, 248, 934, 989, 1377, 1393, 1396, 1461, 1484, 1513, 1610, 1680, 1802, 1804, 1967, 1980, 2004, 2014, 2022, 2099, 2107, 2115 | unreviewed |
| `lib/battle/CBattleInfoCallback.h` | 109, 124, 145 | unreviewed |
| `lib/battle/CObstacleInstance.cpp` | 76 | unreviewed |
| `lib/battle/CUnitState.cpp` | 655, 674 | unreviewed |
| `lib/battle/CUnitState.h` | 227, 228 | unreviewed |
| `lib/battle/CombatValue.cpp` | 446, 566, 816, 855, 862 | unreviewed |
| `lib/battle/CombatValue.h` | 109 | unreviewed |
| `lib/battle/ReachabilityInfo.cpp` | 43, 51, 65, 91 | unreviewed |
| `lib/battle/ReachabilityInfo.h` | 58, 62 | unreviewed |
| `lib/battle/Unit.cpp` | 140, 142 | unreviewed |
| `lib/battle/actions/MeleeAttackActions.cpp` | 103 | unreviewed |
| `lib/battle/actions/SpellCastActions.cpp` | 404, 477, 544 | unreviewed |
| `lib/bonuses/BonusCache.cpp` | 31 | unreviewed |
| `lib/bonuses/BonusCustomTypes.cpp` | 34 | unreviewed |
| `lib/bonuses/Limiters.cpp` | 270 | unreviewed |
| `lib/bonuses/Limiters.h` | 296 | unreviewed |
| `lib/callback/CCallback.cpp` | 393 | reviewed: guarding-creature query requires a canonical map tile and returns the invalid sentinel outside; neighbor enumeration is handled separately |
| `lib/callback/CGameInfoCallback.cpp` | 391, 411, 509, 887, 889, 892 | unreviewed |
| `lib/callback/CNonConstInfoCallback.cpp` | 21 | unreviewed |
| `lib/callback/IGameInfoCallback.h` | 80, 104 | unreviewed |
| `lib/callback/MapInfoCallback.cpp` | 79, 81, 102 | unreviewed |
| `lib/callback/MapInfoCallback.h` | 32 | unreviewed |
| `lib/constants/EntityIdentifiers.h` | 397 | unreviewed |
| `lib/entities/artifact/CArtifactSet.cpp` | 267, 285, 286 | unreviewed |
| `lib/entities/building/CBuilding.cpp` | 72 | unreviewed |
| `lib/entities/building/CBuilding.h` | 82 | unreviewed |
| `lib/filesystem/CCompressedStream.cpp` | 27, 33, 52 | unreviewed |
| `lib/filesystem/CMemoryBuffer.cpp` | 29, 40, 41, 65 | unreviewed |
| `lib/filesystem/CMemoryStream.cpp` | 22, 23, 42 | unreviewed |
| `lib/filesystem/FileInfo.cpp` | 22 | unreviewed |
| `lib/filesystem/PngStripWriter.cpp` | 24, 27, 29, 30, 42, 43, 97, 100, 115 | unreviewed |
| `lib/filesystem/PngStripWriter.h` | 32, 35 | unreviewed |
| `lib/gameState/CGameState.cpp` | 527, 530, 536, 538, 545, 554, 598, 674, 795, 1001, 1007, 1074, 1291, 1310, 1333, 1335, 1336, 1343 | partial: Grail placement and Favorable Winds initialization use topology and have focused tests; guard neighborhood wraps; other candidates unreviewed |
| `lib/gameState/GameStatePackVisitor.cpp` | 322, 545, 551, 555, 559, 563, 567, 571, 575, 579, 787, 1655 | partial: movement direction uses wrapped displacement; guard cache removal updates across one corner and both adjoining edges, other candidates unreviewed |
| `lib/gameState/UpgradeInfo.cpp` | 24, 44, 46, 47 | unreviewed |
| `lib/gameState/UpgradeInfo.h` | 45 | unreviewed |
| `lib/int3.h` | 31, 32, 103, 108, 110, 112, 114, 121, 123, 124, 128, 130, 133, 135, 138, 140, 143, 145, 182, 196 | partial: `findClosestTile` is used only by the conditional, unsupported RMG; other general coordinate operations remain unchanged |
| `lib/json/JsonBonus.cpp` | 725 | unreviewed |
| `lib/json/JsonNode.cpp` | 29 | unreviewed |
| `lib/json/JsonParser.cpp` | 49, 129, 165, 167, 174, 236, 242, 252, 257, 633 | unreviewed |
| `lib/json/JsonValidator.cpp` | 34, 650, 668 | unreviewed |
| `lib/json/JsonWriter.cpp` | 69, 79, 81, 85 | unreviewed |
| `lib/mapObjectConstructors/DwellingInstanceConstructor.cpp` | 70, 73, 75, 76 | unreviewed |
| `lib/mapObjects/CGCreature.cpp` | 617 | reviewed: coordinates seed a deterministic creature-upgrade choice, not a distance or neighborhood calculation |
| `lib/mapObjects/CGHeroInstance.cpp` | 164, 169, 1715 | partial: Visions range corrected |
| `lib/mapObjects/CGHeroInstance.h` | 144 | unreviewed |
| `lib/mapObjects/CGObjectInstance.cpp` | 91, 96, 101, 106, 134, 160, 206, 383 | partial: tile queries for wrapped footprints fixed and tested; other candidates unreviewed |
| `lib/mapObjects/MapObjectDrawOrder.cpp` | 23, 24, 57, 59, 68 | unreviewed |
| `lib/mapObjects/MapObjectDrawOrder.h` | 54 | unreviewed |
| `lib/mapObjects/MiscObjects.cpp` | 613 | partial: pairing and server traversal of synthetic gates tested after baseline; GUI unverified |
| `lib/mapObjects/ObjectTemplate.cpp` | 255, 256, 260, 341, 342, 347, 350, 351, 352, 365, 372, 375, 377, 429, 430, 444, 447, 552, 553 | unreviewed |
| `lib/mapObjects/ObjectTemplate.h` | 72 | unreviewed |
| `lib/mapObjects/Quest.cpp` | 590, 593, 594, 595, 596, 598 | reviewed: seer-hut compass text divides the canonical map rectangle into absolute regions; relative toroidal directions are not computed |
| `lib/mapping/CDrawRoadsOperation.cpp` | 316, 317 | unreviewed |
| `lib/mapping/CMap.cpp` | 277, 292, 293, 331, 334, 354, 357, 366, 374, 395, 396, 412, 413, 459, 470, 497, 509, 525, 545, 547, 548, 608, 622, 653, 779, 783, 784, 786, 790, 908, 1003, 1231, 1236 | unreviewed |
| `lib/mapping/CMap.h` | 107, 386, 390, 391, 397, 403 | unreviewed |
| `lib/mapping/CMapHeader.h` | 109, 111, 112 | unreviewed |
| `lib/mapping/CMapInfo.cpp` | 129, 130, 176 | unreviewed |
| `lib/mapping/CMapOperation.cpp` | 36, 41, 265, 384, 385, 391, 394, 396, 405, 409, 413, 417, 436, 554, 607 | unreviewed |
| `lib/mapping/MapEditUtils.cpp` | 26, 41, 42, 58, 68, 121, 127, 347, 351, 354 | unreviewed |
| `lib/mapping/MapEditUtils.h` | 24 | unreviewed |
| `lib/mapping/MapFormatH3M.cpp` | 214, 1067, 1069, 2217, 2248, 3132, 3152 | unreviewed |
| `lib/mapping/MapFormatJson.cpp` | 306, 982, 983, 1010, 1045, 1050, 1064, 1082, 1087, 1102, 1109, 1114, 1136, 1139, 1143, 1146, 1457, 1458, 1504, 1506, 1510, 1512 | unreviewed |
| `lib/mapping/MapTilesStorage.h` | 25, 26, 44, 51, 72 | unreviewed |
| `lib/mapping/MapTopology.h` | 24, 26, 37, 42, 52, 53, 68, 69, 70, 71, 76 | unreviewed |
| `lib/mapping/ObstacleProxy.cpp` | 406, 408, 416, 485, 545, 547 | unreviewed |
| `lib/mapping/ObstacleProxy.h` | 49, 82 | unreviewed |
| `lib/modding/CModHandler.cpp` | 598 | unreviewed |
| `lib/modding/ModManager.cpp` | 254, 625 | unreviewed |
| `lib/networkPacks/PacksForClientBattle.h` | 531 | unreviewed |
| `lib/pathfinder/CGPathNode.h` | 73 | unreviewed |
| `lib/pathfinder/CPathfinder.cpp` | 54, 61, 107, 113, 162, 589, 606, 618, 619, 697, 705, 720 | unreviewed |
| `lib/pathfinder/CPathfinder.h` | 112, 115 | unreviewed |
| `lib/pathfinder/INodeStorage.h` | 32 | unreviewed |
| `lib/pathfinder/NodeStorage.cpp` | 75, 86 | unreviewed |
| `lib/pathfinder/NodeStorage.h` | 41 | unreviewed |
| `lib/pathfinder/PathfindingRules.cpp` | 45, 46, 69, 75, 76 | unreviewed |
| `lib/rewardable/Reward.h` | 30 | unreviewed |
| `lib/rmg/CMapGenOptions.cpp` | 42, 53, 872, 873 | unreviewed |
| `lib/rmg/CMapGenerator.cpp` | 186, 187, 467, 468, 530, 532, 534, 541, 564 | unreviewed |
| `lib/rmg/CMapGenerator.h` | 75 | unreviewed |
| `lib/rmg/CRmgTemplate.cpp` | 719, 720, 721, 1174 | unreviewed |
| `lib/rmg/CZoneGridPlacer.cpp` | 36, 47, 54, 59, 65, 75, 76, 88, 172, 382, 387, 401, 406, 454, 490, 492, 533 | unreviewed |
| `lib/rmg/CZoneGridPlacer.h` | 61, 66, 99, 106 | unreviewed |
| `lib/rmg/CZonePlacer.cpp` | 39, 47, 50, 104, 105, 501, 502, 544, 548, 577, 578, 581, 590, 595, 597, 621, 622, 623, 693, 694, 700, 811, 821, 837, 847, 859, 860, 901, 903, 908, 991, 1012, 1015, 1017, 1023, 1030, 1034, 1036, 1040, 1042, 1076, 1096, 1098, 1100, 1125 | unreviewed |
| `lib/rmg/CZonePlacer.h` | 40, 48 | unreviewed |
| `lib/rmg/Functions.cpp` | 64 | unreviewed |
| `lib/rmg/PenroseTiling.cpp` | 31, 36 | unreviewed |
| `lib/rmg/RmgArea.cpp` | 24, 197, 224, 234, 268, 270, 273, 275, 278, 284, 286, 305, 307 | unreviewed |
| `lib/rmg/RmgArea.h` | 41, 53, 54, 55 | unreviewed |
| `lib/rmg/RmgMap.cpp` | 54, 58, 61, 66, 70, 71, 76, 80, 81, 90, 91, 195, 200, 223, 225, 235, 339, 343, 346, 350, 381, 382, 385, 387 | unreviewed |
| `lib/rmg/RmgMap.h` | 45, 46, 47, 55, 68, 69 | unreviewed |
| `lib/rmg/RmgObject.cpp` | 58, 88, 99, 105, 111, 154, 373, 427, 439, 496, 504, 522, 527 | unreviewed |
| `lib/rmg/RmgPath.cpp` | 134, 194, 198, 201, 220, 230, 247, 248, 250, 251, 271 | unreviewed |
| `lib/rmg/RmgPath.h` | 48 | unreviewed |
| `lib/rmg/TileInfo.cpp` | 15, 20, 25 | unreviewed |
| `lib/rmg/TileInfo.h` | 21, 22 | unreviewed |
| `lib/rmg/Zone.cpp` | 71, 73, 197, 349, 393, 416 | unreviewed |
| `lib/rmg/float3.h` | 27, 31, 44, 46, 47, 59, 61 | unreviewed |
| `lib/rmg/modificators/ConnectionsPlacer.cpp` | 52, 180, 197, 234, 248, 249, 261, 310, 318, 319, 391, 392, 393, 402, 433, 438, 453, 537, 542, 567, 580, 596 | unreviewed |
| `lib/rmg/modificators/ConnectionsPlacer.h` | 59 | unreviewed |
| `lib/rmg/modificators/Modificator.cpp` | 126, 127, 130, 132 | unreviewed |
| `lib/rmg/modificators/ObjectManager.cpp` | 73, 76, 84, 106, 108, 110, 114, 116, 118, 122, 138, 139, 140, 176, 256, 262, 284, 324, 497, 500, 586, 614, 629, 644, 821, 831 | unreviewed |
| `lib/rmg/modificators/ObjectManager.h` | 76, 77, 78, 79 | unreviewed |
| `lib/rmg/modificators/ObstaclePlacer.cpp` | 87, 133, 135 | unreviewed |
| `lib/rmg/modificators/ObstaclePlacer.h` | 28 | unreviewed |
| `lib/rmg/modificators/QuestArtifactPlacer.cpp` | 115 | unreviewed |
| `lib/rmg/modificators/RiverPlacer.cpp` | 179, 222, 227, 241, 290 | unreviewed |
| `lib/rmg/modificators/RoadPlacer.cpp` | 77, 100, 128, 143 | unreviewed |
| `lib/rmg/modificators/TownPlacer.cpp` | 185, 200, 215, 231 | unreviewed |
| `lib/rmg/modificators/TreasurePlacer.cpp` | 862, 1043, 1055 | unreviewed |
| `lib/rmg/modificators/WaterAdopter.cpp` | 45, 79, 87, 120, 152, 153, 199, 236 | unreviewed |
| `lib/rmg/modificators/WaterProxy.cpp` | 52, 119, 121, 274, 313, 404 | unreviewed |
| `lib/rmg/modificators/WaterRoutes.cpp` | 36, 65, 67, 76 | unreviewed |
| `lib/serializer/CLoadFile.cpp` | 90, 106 | unreviewed |
| `lib/serializer/CMemorySerializer.cpp` | 15, 16, 19 | unreviewed |
| `lib/serializer/GameConnection.cpp` | 45, 49 | unreviewed |
| `lib/spells/BattleSpellMechanics.cpp` | 50, 267 | unreviewed |
| `lib/spells/ISpellMechanics.h` | 402 | unreviewed |
| `lib/spells/adventure/AdventureSpellEffect.cpp` | 50, 61, 64 | partial: ranged spell check fixed and tested after baseline |
| `lib/spells/adventure/AdventureSpellMechanics.cpp` | 109, 112 | unreviewed |
| `lib/spells/adventure/AdventureSpellMechanics.h` | 40 | unreviewed |
| `lib/spells/adventure/ReinforcementsEffect.cpp` | 72 | unreviewed |
| `lib/spells/adventure/SummonBoatEffect.cpp` | 107 | reviewed: nearest-boat Manhattan distance uses topology; basic server casts tested |
| `lib/spells/adventure/TownPortalEffect.cpp` | 86 | unreviewed |
| `lib/spells/adventure/TownRelatedAdventureSpellEffect.cpp` | 65, 67, 71, 75 | partial: nearest-town helper corrected |
| `lib/texts/TextOperations.cpp` | 357, 478, 512 | unreviewed |
| `lib/texts/TextOperations.h` | 85 | unreviewed |
| `lib/vstd/StringUtils.cpp` | 36 | unreviewed |
| `mapeditor/Animation.cpp` | 44, 212, 213, 274, 275, 283, 287, 288, 301, 330, 359, 384, 387, 450, 459, 468 | unreviewed |
| `mapeditor/BitmapHandler.cpp` | 36, 37, 38, 53, 54, 56, 58, 63, 89 | unreviewed |
| `mapeditor/helper.cpp` | 52 | unreviewed |
| `mapeditor/inspector/questwidget.cpp` | 504 | unreviewed |
| `mapeditor/inspector/townspellswidget.cpp` | 194 | unreviewed |
| `mapeditor/mainwindow.cpp` | 168, 170, 171, 175, 1552, 1570 | unreviewed |
| `mapeditor/mapcontroller.cpp` | 434, 435, 553, 586 | unreviewed |
| `mapeditor/maphandler.cpp` | 58, 124, 130, 139, 150, 169, 290, 327, 362, 363, 364, 391, 392, 401, 402, 554, 577, 600, 601 | unreviewed |
| `mapeditor/maphandler.h` | 105 | unreviewed |
| `mapeditor/mapsettings/generalsettings.cpp` | 40 | unreviewed |
| `mapeditor/mapview.cpp` | 32, 135, 155, 194, 198, 204, 206, 219, 221, 235, 237, 260, 263, 278, 336, 357, 393, 396, 525, 528 | unreviewed |
| `mapeditor/scenelayer.cpp` | 40, 45, 118, 119, 120, 201, 202, 249, 284, 334, 358, 360, 411, 451, 452, 465, 610, 612, 619, 644, 661, 704, 811, 815, 817, 835 | unreviewed |
| `mapeditor/templateeditor/GeometryAlgorithm.cpp` | 14, 25, 26, 27, 28, 34, 35, 40, 50, 64, 65, 66, 84, 85, 86, 120, 121, 122, 123, 124, 125, 126, 127, 135, 136, 147, 148 | unreviewed |
| `mapeditor/templateeditor/GeometryAlgorithm.h` | 30, 32 | unreviewed |
| `mapeditor/templateeditor/templateeditor.cpp` | 174, 301 | unreviewed |
| `mapeditor/validator.cpp` | 94, 364 | unreviewed |
| `mapeditor/validator.h` | 85 | unreviewed |
| `mapeditor/windownewmap.cpp` | 42, 84, 85, 125, 224, 225, 234, 237, 238, 291, 350 | unreviewed |
| `mapeditor/windownewmap.h` | 29, 30, 69 | unreviewed |
| `server/CGameHandler.cpp` | 784, 798, 907, 942, 952, 1828, 2508, 2588, 3860, 4621, 4644 | unreviewed |
| `server/CreatureValueEstimator.cpp` | 67, 68, 95, 105, 256 | unreviewed |
| `server/NetPacksServer.cpp` | 328 | unreviewed |
| `server/battles/BattleFlowProcessor.cpp` | 701, 861 | unreviewed |
| `server/lobby/BattleOnlyMap.cpp` | 40, 41 | unreviewed |
| `server/processors/NewTurnProcessor.cpp` | 579, 580 | reviewed: Grail rumor names an absolute region of the canonical map rectangle, not a distance from the hero |
| `server/processors/PlayerMessageProcessor.cpp` | 688, 689, 692, 693, 694, 819 | reviewed: reveal cheat enumerates canonical tiles; teleport cheat explicitly accepts an in-map destination |
| `server/processors/TurnOrderProcessor.cpp` | 132, 161, 162, 163, 177, 178, 179, 184, 185, 186 | unreviewed |
