
Sj�lva funktionaliteten �r realiserad i f�ljande filer, med f�ljande ansvar.


casual_make -> casual_make.engine(
			casual_make.function_definitions
			casual_make.pre_user_config_solaris
			casual_make.post_user_config_solaris
			casual_make.prepare_function_definitions
			casual_make.normalize_imakefile
			<"anv�ndarens imakefil> )

casual_make.engine:
	Sj�va motorn :) Dispatchar imake-funktionsanrop till de som �r definerade i 
	casual_make.function_definitions. Ganska simpel realisering d� b�de casual_make.prepare_function_definitions
	och anv�ndarens imake-fil normaliseras till ett k�nt och strikt format.
	Det finns s�kert f�rb�ttringsm�jligheter att �ka p� prestandan lite i denna (dock tj�nar vi betydligt 
	mer n�r vi kan parallelisera kompilering och l�nkning, s� det spelar inte s� stor roll). 



casual_make.pre_user_config_solaris:
	Definerar den statiska konfigurationen. Lite paths och grejer. Inkluderas f�re anv�ndarens
	definitioner har inkluderats.

casual_make.post_user_config_solaris:
	Definerar kompilator/l�nknings-konfiguration (vissa delar �r beroende av 
	casual_make.pre_user_config_solaris, men som kan "overridas" av anv�ndarens imakefil. Inkluderas
	efter det att anv�ndarens definitioner har inkluderas.


casual_make.function_definitions: 
	Definerar samtliga "funtions-mallar", det som best�mmer vad som blir i make-filen. Det �r h�r vi g�r in
	och f�r�ndrar eller l�gger till beteenden f�r casual_make.






casual_make.prepare_function_definitions:
	Transformerar casual_make.function_definitions till ett mer l�ttanv�nt format

casual_make.normalize_imakefile:
	Normaliserar anv�ndarens imakefil till ett k�nt format s� det blir l�ttare f�r casual_make.engine
	





