deltree /y OFPDoc\html\*.*
"C:\Program Files\doxygen\bin\doxygen" OFPDoc.cfg 2> OFPDoc.log
"C:\Program Files\HTML Help Workshop\hhc" OFPDoc\html\index.hhp
move /Y OFPDoc\html\index.chm OFP_ProgDoc.chm
start OFP_ProgDoc.chm
