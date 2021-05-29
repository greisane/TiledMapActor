# TiledMapActor
Simple UE4 actor to import JSON maps made in Tiled. Not production ready.

![Level](https://user-images.githubusercontent.com/59247540/120086062-bc4daa80-c0cc-11eb-8adf-78202f151290.gif)

The attached .blend contains a sample tileset and a script to generate a .csv with asset paths.

# Usage
1. Add `"JSon"` to `PrivateDependencyModuleNames` in your build rules file (MyGame.Build.cs)
2. Include the source files TiledMapActor.h and TiledMapActor.cpp in your C++ project and build.
3. Create a data table with *TiledMapMeshTableRow* data structure and fill it in manually.  
**OR**  
Run the script in the sample .blend file to generate a .csv data table which you can import.  
3. Put your JSON map somewhere in the game content folder.
3. Drop a TiledMapActor in the level and configure *Map Path*, *Mesh Table* and *Tile Size*. ![Actor properties](https://user-images.githubusercontent.com/59247540/120084439-aa193f80-c0bf-11eb-85e4-00c9b932f64a.png)
5. Press *Import Map*.
