
Själva funktionaliteten är realiserad i följande filer, med följande ansvar.


imake2make -> imake2make.engine(
			imake2make.function_definitions
			imake2make.pre_user_config_solaris
			imake2make.post_user_config_solaris
			imake2make.prepare_function_definitions
			imake2make.normalize_imakefile
			<"användarens imakefil> )

imake2make.engine:
	Sjäva motorn :) Dispatchar imake-funktionsanrop till de som är definerade i 
	imake2make.function_definitions. Ganska simpel realisering då både imake2make.prepare_function_definitions
	och användarens imake-fil normaliseras till ett känt och strikt format.
	Det finns säkert förbättringsmöjligheter att öka på prestandan lite i denna (dock tjänar vi betydligt 
	mer när vi kan parallelisera kompilering och länkning, så det spelar inte så stor roll). 



imake2make.pre_user_config_solaris:
	Definerar den statiska konfigurationen. Lite paths och grejer. Inkluderas före användarens
	definitioner har inkluderats.

imake2make.post_user_config_solaris:
	Definerar kompilator/länknings-konfiguration (vissa delar är beroende av 
	imake2make.pre_user_config_solaris, men som kan "overridas" av användarens imakefil. Inkluderas
	efter det att användarens definitioner har inkluderas.


imake2make.function_definitions: 
	Definerar samtliga "funtions-mallar", det som bestämmer vad som blir i make-filen. Det är här vi går in
	och förändrar eller lägger till beteenden för imake2make.






imake2make.prepare_function_definitions:
	Transformerar imake2make.function_definitions till ett mer lättanvänt format

imake2make.normalize_imakefile:
	Normaliserar användarens imakefil till ett känt format så det blir lättare för imake2make.engine
	





