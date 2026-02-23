# ChoreoCompiler
A small program to create Anki cards of `.choreo` files from ChoreoMaster (http://www.jahn-dc.de/choreomaster).

## Dependencies
This program requires Qt to work:
```
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-base-dev-tools
```
## Build and Install
```mkdir build
cd build
cmake ..
make
sudo make install
```
This will copy the `coco` executable to `/usr/local/bin`.

## Usage
To create Anki cards for each dancer do the following:
```
coco MyChoreography.choreo
```
It will generate a directory for each dancer containing renderings of the position and a text file with the Anki-notes. To import them into Anki copy the images to `~/.local/share/Anki/[User]/collection.media` and then import the text file into Anki. You can find a detailed guide [here](https://docs.ankiweb.net/importing/text-files.html).
