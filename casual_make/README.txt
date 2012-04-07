
Själva funktionaliteten är realiserad i följande filer, med följande ansvar.


casual_make -> casual_make.engine(
			casual_make.function_definitions
			casual_make.pre_user_config_solaris
			casual_make.post_user_config_solaris
			casual_make.prepare_function_definitions
			casual_make.normalize_imakefile
			<"användarens imakefil> )

casual_make.engine:
	Sjäva motorn :) Dispatchar imake-funktionsanrop till de som är definerade i 
	casual_make.function_definitions. Ganska simpel realisering då både casual_make.prepare_function_definitions
	och användarens imake-fil normaliseras till ett känt och strikt format.
	Det finns säkert förbättringsmöjligheter att öka på prestandan lite i denna (dock tjänar vi betydligt 
	mer när vi kan parallelisera kompilering och länkning, så det spelar inte så stor roll). 



casual_make.pre_user_config_solaris:
	Definerar den statiska konfigurationen. Lite paths och grejer. Inkluderas före användarens
	definitioner har inkluderats.

casual_make.post_user_config_solaris:
	Definerar kompilator/länknings-konfiguration (vissa delar är beroende av 
	casual_make.pre_user_config_solaris, men som kan "overridas" av användarens imakefil. Inkluderas
	efter det att användarens definitioner har inkluderas.


casual_make.function_definitions: 
	Definerar samtliga "funtions-mallar", det som bestämmer vad som blir i make-filen. Det är här vi går in
	och förändrar eller lägger till beteenden för casual_make.






casual_make.prepare_function_definitions:
	Transformerar casual_make.function_definitions till ett mer lättanvänt format

casual_make.normalize_imakefile:
	Normaliserar användarens imakefil till ett känt format så det blir lättare för casual_make.engine
	





