cl -c -Ml -Zi main.c
cl -c -Ml -Zi graf.c
cl -c -Ml -Zi setpxl.c

link /CO main.obj graf.obj setpxl.obj,demo.exe,,doscalls.lib,main.def
