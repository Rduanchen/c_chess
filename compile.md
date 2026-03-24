
before compile, you have to make terminal know where gcc is
$env:Path += ";C:\msys64\ucrt64\bin"

compile inst.
gcc *.c -o C_Chess -I"C:/msys64/ucrt64/include" -L"./libs" -lmingw32 -lSDL2main -lSDL2 -lSDL2_image