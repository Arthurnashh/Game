cd ..\..\..

rem Create assembly from shader fragments
rem B
xsasm d3dxb\shaders\FPShaders.vsa
rem N
xsasm d3dxb\shaders\FPShadersNormal.vsa
rem I
xsasm d3dxb\shaders\FPShadersInstancing.vsa
rem J
xsasm d3dxb\shaders\FPShadersNormalInstancing.vsa
rem O
xsasm d3dxb\shaders\FPShadersPoint.vsa
rem D
xsasm d3dxb\shaders\FPShadersDirectional.vsa
rem F
xsasm d3dxb\shaders\FPShadersDirectionalADForced.vsa
rem T
xsasm d3dxb\shaders\FPShadersDirectionalStars.vsa
rem X
xsasm d3dxb\shaders\FPShadersFogNone.vsa
rem Y
xsasm d3dxb\shaders\FPShadersFog.vsa
rem Z
xsasm d3dxb\shaders\FPShadersFogAlpha.vsa
rem W
xsasm d3dxb\shaders\FPShadersWriteLights.vsa
rem A
xsasm d3dxb\shaders\FPShadersAlphaShadow.vsa
rem P
xsasm d3dxb\shaders\FPShadersPoint0.vsa
xsasm d3dxb\shaders\FPShadersPoint1.vsa
xsasm d3dxb\shaders\FPShadersPoint2.vsa
xsasm d3dxb\shaders\FPShadersPoint3.vsa
rem S
xsasm d3dxb\shaders\FPShadersSpot0.vsa
xsasm d3dxb\shaders\FPShadersSpot1.vsa
xsasm d3dxb\shaders\FPShadersSpot2.vsa
xsasm d3dxb\shaders\FPShadersSpot3.vsa

rem Create fragment combinations
cd d3dxb\shaders
md SCPool
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciOTWX.xvu FPShadersPoint.xvu            FPSHadersDirectionalStars.xvu    FPSHadersWriteLights.xvu FPSHadersFogNone.xvu

B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYPW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYPW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYPW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYSW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYSW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYSW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYSW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZASW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersSpot0.xvu FPSHadersWriteLights.xvu

B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYPPW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYPPW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYPPW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPPW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPPW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPShadersPoint1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYPSW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYPSW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYPSW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPSW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPSW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPShadersSpot1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYSPW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYSPW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYSPW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYSPW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZASPW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersSpot0.xvu FPShadersPoint1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYSSW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYSSW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYSSW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYSSW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZASSW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersSpot0.xvu FPShadersSpot1.xvu FPSHadersWriteLights.xvu

B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYPPPW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYPPPW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYPPPW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPPPW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPPPW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYPPSW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYPPSW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYPPSW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPPSW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPPSW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYPSPW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYPSPW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYPSPW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPSPW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPSPW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYPSSW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYPSSW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYPSSW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPSSW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPSSW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYSPPW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYSPPW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYSPPW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYSPPW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZASPPW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYSPSW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYSPSW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYSPSW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYSPSW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZASPSW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYSSPW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYSSPW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYSSPW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYSSPW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZASSPW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYSSSW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYSSSW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYSSSW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYSSSW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZASSSW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPSHadersWriteLights.xvu

B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYPPPPW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYPPPPW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYPPPPW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPPPPW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPPPPW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYPPPSW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYPPPSW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYPPPSW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPPPSW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPPPSW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYPPSPW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYPPSPW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYPPSPW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPPSPW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPPSPW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYPPSSW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYPPSSW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYPPSSW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPPSSW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPPSSW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYPSPPW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYPSPPW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYPSPPW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPSPPW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPSPPW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYPSPSW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYPSPSW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYPSPSW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPSPSW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPSPSW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYPSSPW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYPSSPW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYPSSPW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPSSPW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPSSPW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYPSSSW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYPSSSW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYPSSSW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPSSSW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPSSSW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYSPPPW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYSPPPW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYSPPPW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYSPPPW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZASPPPW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYSPPSW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYSPPSW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYSPPSW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYSPPSW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZASPPSW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYSPSPW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYSPSPW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYSPSPW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYSPSPW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZASPSPW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYSPSSW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYSPSSW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYSPSSW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYSPSSW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZASPSSW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersSpot0.xvu FPShadersPoint1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYSSPPW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYSSPPW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYSSPPW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYSSPPW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZASSPPW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYSSPSW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYSSPSW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYSSPSW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYSSPSW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZASSPSW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersPoint2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYSSSPW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYSSSPW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYSSSPW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYSSSPW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZASSSPW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersPoint3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciBDYSSSSW.xvu FPShaders.xvu                 FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciNFYSSSSW.xvu FPShadersNormal.xvu           FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIDYSSSSW.xvu FPShadersInstancing.xvu       FPSHadersDirectional.xvu         FPSHadersFog.xvu         FPShadersSpot0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciJFYPSSSW.xvu FPShadersNormalInstancing.xvu FPSHadersDirectionalADForced.xvu FPSHadersFog.xvu         FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu
B:\ShaderSplicer\ShaderSplicer SCPool\sciIZAPSSSW.xvu FPShadersInstancing.xvu       FPSHadersFogAlpha.xvu            FPSHadersAlphaShadow.xvu FPShadersPoint0.xvu FPShadersSpot1.xvu FPShadersSpot2.xvu FPShadersSpot3.xvu FPSHadersWriteLights.xvu

rem Create and distribute the PBO file
X:\Finalize\Update\FileBank SCPool
move /Y SCPool.pbo X:\Finalize\Poseidon\Dta