deltree /y DayZDoc\html\*.*
"C:\Program Files\doxygen\bin\doxygen" DayZDoc.cfg 2> DayZDoc.log
"C:\Program Files (x86)\HTML Help Workshop\hhc" DayZDoc\html\index.hhp
move /Y DayZDoc\html\index.chm DayZ_ProgDoc.chm
start DayZ_ProgDoc.chm
