
Sj�lva funktionaliteten �r realiserad i f�ljande filer, med f�ljande ansvar.


imake2make -> imake2make.engine(
			imake2make.function_definitions
			imake2make.pre_user_config_solaris
			imake2make.post_user_config_solaris
			imake2make.prepare_function_definitions
			imake2make.normalize_imakefile
			<"anv�ndarens imakefil> )

imake2make.engine:
	Sj�va motorn :) Dispatchar imake-funktionsanrop till de som �r definerade i 
	imake2make.function_definitions. Ganska simpel realisering d� b�de imake2make.prepare_function_definitions
	och anv�ndarens imake-fil normaliseras till ett k�nt och strikt format.
	Det finns s�kert f�rb�ttringsm�jligheter att �ka p� prestandan lite i denna (dock tj�nar vi betydligt 
	mer n�r vi kan parallelisera kompilering och l�nkning, s� det spelar inte s� stor roll). 



imake2make.pre_user_config_solaris:
	Definerar den statiska konfigurationen. Lite paths och grejer. Inkluderas f�re anv�ndarens
	definitioner har inkluderats.

imake2make.post_user_config_solaris:
	Definerar kompilator/l�nknings-konfiguration (vissa delar �r beroende av 
	imake2make.pre_user_config_solaris, men som kan "overridas" av anv�ndarens imakefil. Inkluderas
	efter det att anv�ndarens definitioner har inkluderas.


imake2make.function_definitions: 
	Definerar samtliga "funtions-mallar", det som best�mmer vad som blir i make-filen. Det �r h�r vi g�r in
	och f�r�ndrar eller l�gger till beteenden f�r imake2make.






imake2make.prepare_function_definitions:
	Transformerar imake2make.function_definitions till ett mer l�ttanv�nt format

imake2make.normalize_imakefile:
	Normaliserar anv�ndarens imakefil till ett k�nt format s� det blir l�ttare f�r imake2make.engine
	





