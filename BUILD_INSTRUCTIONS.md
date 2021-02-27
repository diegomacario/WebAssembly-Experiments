To create local server:

python -m SimpleHTTPServer 8080

To build the HelloTriangle.cpp example:

Open "\emsdk\emcmdprompt.bat" and navigate to the root of the repository

emcc src\HelloTriangle.cpp -O3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_GLFW=3 -s WASM=1 -o build/index.html

To build the Ten3DCubesWithCameraGLTF.cpp example:

In a build directory outside of the repo:

Open a "Native Tools Command Prompt for VS", execute "\emsdk\emcmdprompt.bat" in it and navigate to the build folder of the repository
cmake --build . --target clean
emcmake cmake ..
ninja -v
