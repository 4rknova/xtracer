mkdir ..\bin
mkdir lib
mkdir inc
mkdir inc\nmath
mkdir inc\ncf
mkdir inc\nmesh
mkdir inc\nimg

copy ..\..\libnmath\bin\libnmath.dll 	..\bin
copy ..\..\libncf\bin\libncf.dll 	..\bin
copy ..\..\libnmesh\bin\libnmesh.dll 	..\bin
copy ..\..\libnimg\bin\libnimg.dll	..\bin

copy ..\..\libnmath\bin\*.lib 	lib\
copy ..\..\libnmath\bin\*.dll 	lib\

copy ..\..\libncf\bin\*.lib 	lib\
copy ..\..\libncf\bin\*.dll 	lib\

copy ..\..\libnmesh\bin\*.lib 	lib\
copy ..\..\libnmesh\bin\*.dll 	lib\

copy ..\..\libnimg\bin\*.lib 	lib\
copy ..\..\libnimg\bin\*.dll 	lib\

copy ..\..\libnmath\src\*.h 	inc\nmath\
copy ..\..\libnmath\src\*.hpp	inc\nmath\
copy ..\..\libnmath\src\*.inl	inc\nmath\
copy ..\..\libnmath\src\*.tml	inc\nmath\

copy ..\..\libncf\src\*.h 	inc\ncf\
copy ..\..\libncf\src\*.hpp	inc\ncf\
copy ..\..\libncf\src\*.inl	inc\ncf\
copy ..\..\libncf\src\*.tml	inc\ncf\

copy ..\..\libnmesh\src\*.h 	inc\nmesh\
copy ..\..\libnmesh\src\*.hpp	inc\nmesh\
copy ..\..\libnmesh\src\*.inl	inc\nmesh\
copy ..\..\libnmesh\src\*.tml	inc\nmesh\

copy ..\..\libnimg\src\*.h 	inc\nimg\
copy ..\..\libnimg\src\*.hpp	inc\nimg\
copy ..\..\libnimg\src\*.inl	inc\nimg\
copy ..\..\libnimg\src\*.tml	inc\nimg\
