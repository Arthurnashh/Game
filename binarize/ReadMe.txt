Sample command lines

Worlds:

One wrp file:

x:
-always -addon=BISOFP -addon=O X:\Finalize\Poseidon\WorldsX X:\X\Worlds noe.wrp

x:
-always -addon=BISOFP X:\Finalize\Poseidon\WorldsX X:\X\Worlds eden.wrp

----------------------------------------------
Models
All p3d files from Data3D:

x:
-addon=BISOFP -textures=Optimize -exclude=Optimize\exclude.lst Data3D Optimize\Data3D *.p3d

One p3d file from Data3D:

x:
-always -addon=BISOFP -textures=Optimize -exclude=Optimize\exclude.lst Data3D Optimize\Data3D jeep.p3d

Xbox addon:
-always -addon=BISOFP -addon=Kolo -textures=AddOnsOpt -exclude=AddOnsOpt\exclude.lst Kolo AddOnsOpt\Kolo

Xbox O Addon:
-textures=AddOnsOpt\Opt -exclude=O\exclude.lst O AddOnsOpt\Opt\O
-always -textures=AddOnsOpt\Opt -exclude=O\exclude.lst O AddOnsOpt\Opt\O *domek_sedy*

OFP2 addon:

p:
-textures=P:\OFP2\temp\bin -binPath=b:\binarize -addon=p:\OFP2\UI p:\OFP2\UI P:\OFP2\temp\bin\OFP2\UI
-textures=P:\OFP2\temp\bin -binPath=b:\binarize -addon=p:\OFP2\TGATest p:\OFP2\TGATest P:\OFP2\temp\bin\OFP2\TGATest

ArmA p3d file:

-textures=P:\ca\temp\bin -binPath=b:\binarize p:\ca\buildings P:\ca\temp\bin\ca\buildings panelak2.p3d

ArmA addon (wrp file):

-silent -targetBonesInterval=56 -maxProcesses=20 -textures=p:\ca\temp\bin -binPath=b:\binarize -addon=p:\ca -exclude=desert\exclude.lst p:\ca\desert p:\ca\temp\bin\ca\desert 2>p:\ca\temp\bin\ca\desert\binarize.log" 
