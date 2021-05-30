clear,clc

mex -I./lib/binn/src binnDecode.c ./lib/binn/src/binn.c
mex -I./lib/binn/src binnEncode.c ./lib/binn/src/binn.c
