pushd
attrib -A -R c:\DevP\Docs\ComRef\browserIndependent\comref.xml
copy /Y c:\comref.xml c:\DevP\Docs\ComRef\browserIndependent\comref.xml
attrib -A -R c:\DevP\Docs\ComRef\comref.xml
copy /Y c:\comref.xml c:\DevP\Docs\ComRef\comref.xml
c:
cd \DevP\Docs\ComRef\browserIndependent
call c:\DevP\Docs\ComRef\browserIndependent\generateComRef.bat noopen
cd \DevP\Docs\ComRef
svn ci -m "AB2: automatic comref update"
popd
pause
