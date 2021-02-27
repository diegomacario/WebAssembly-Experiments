### To create a local server

Execute this command:

```
python -m SimpleHTTPServer 8080
```

### To build the HelloTriangle.cpp example

Open `\emsdk\emcmdprompt.bat`, navigate to the root of this repository and execute this command:

```
emcc src\HelloTriangle.cpp -O3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_GLFW=3 -s WASM=1 -o build/index.html
```

### To build the Ten3DCubesWithCameraGLTF.cpp example

Open an `x64 Native Tools Command Prompt for VS 20XX`", execute `\emsdk\emcmdprompt.bat` in it, navigate to the build folder of this repository and execute these commands:

```
cmake --build . --target clean
emcmake cmake ..
ninja -v
```
