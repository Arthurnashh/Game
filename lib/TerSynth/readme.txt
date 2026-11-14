Triedy
-------
Terrain - zaobalenie vzorkovania celeho terenu (orig. aj synt.), interpolacia medzi originalnym a syntetizovanym terenom
TerrainSynth - analyza originalneho terenu, synteza novych blokov 



-------
Na samplovanie a analyzu celeho terenu sa pouziva trieda Terrain (terrain.h, terrain.cpp).

(1) Pred pouzitim je potrebne zavolat Terrain::SetTerrain(float* pTerrain, int nWidth, int nHeight),
	pTerrain ukazuje na cele pole vyskovej mapy (nWidth * nHeight floatov) - toto pole sa NESMIE 
	dealokovat, pretoze sa pouziva pri synteze blokov hraniciacich s originalnym terenom, nWidth a 
	nHeight su rozmery (testovane len pri 2048x2048)
	- tato funkcia vykona takisto analyzu terenu - po tomto volani je mozne volat Terrain::Get...

Nasledujuce funkcie je mozne volat iba po Terrain::SetTerrain (staci len raz) - ak pred nimi nebol zavolany 
SetTerrain vsetky vracaju 0 alebo 0.0f .

int Terrain::GetPatchAligned(float* pBuffer, int nLeft, int nTop)	
	navzorkuje teren (originalny aj syntetizovany) - do pBuffer ulozi patch vyskovej mapy velkosti
	128x128px so suradnicami (nLeft, nTop) - (nLeft+128, nTop+128) - suradnice v Px
	nLeft aj nTop MUSIA byt delitelne 128-ckou 
	pBuffer musi byt zarovnany na 16 Bytov, lebo sa pouziva SSE
	- nie je thread-safe

float Terrain::GetPixel(int nX, int nY) const
	navzorkuje teren (originalny aj syntetizovany) na suradniciach nX, nY (v Px)
	generovanie terenu touto metodou je asi 30x pomalsie ako cez GetPatchAligned

float Terrain::GetPixelPrecise(float fX, float fY) const
	navzorkuje teren (originalny aj syntetizovany) na suradniciach (fX, fY) (v metroch)
	vypocet je presny - najdu sa 4 najblizsie pixely okolo suradnic fX, fY a hodnota sa interpoluje
	bilinearnou interpolaciou - vzhladom na tento fakt je to 4x pomalsie ako GetPixel


Cleanup
--------
nie je potrebne robit ziaden cleanup - vsetky alokovane polia sa dealokuju same v destruktoroch prislusnych tried


File list
---------
terrain.h - deklaracia triedy Terrain
terrain.cpp - implementacia triedy Terrain
tersynth.h - deklaracia triedy TerrainSynth
tersynth.cpp - implementacia TerrainSynth
tersynth_heplers.h - 
tersynth_helpers.cpp - pomocne funkcie pre TerrainSynth a Terrain (hlavne interpolacie)

		
