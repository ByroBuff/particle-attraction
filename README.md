# particle-attraction
A particle simulation with asyemtrical attractions between colors. A matrix is randomly generated to determine the attraction/repulsion force that each color dot has for each other color dot including itself. This simple system creates surprisingly complex forms that can move freely and exhibit predictable behavior. I decided to make this after watching [This video](https://www.youtube.com/watch?v=p4YirERTVF0) by [Tom Mohr](https://github.com/tom-mohr). The initial code was inspired by his [math breakdown video](https://www.youtube.com/watch?v=scvuli-zcRc) but was heavily optimized and written in cpython.

## Usage
### Setup
```
python setup.py build_ext --inplace
pip install -r requirements.txt
```
### Running
```
python main.py [seed]
```
