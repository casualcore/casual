#include "ngx_casual_frontend_ddebug.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//void dd(const char * format, ...)
//{
//   FILE* file = fopen("/tmp/hb.log", "a+");
//   fprintf(file, "casual ***\n");
//   va_list args;
//   va_start(args,format);
//   vfprintf(file,format,args);
//   va_end(args);
//   fclose( file);
//}

void dd_func(const char * text, const char * function )
{
   char outstr[20];
   time_t t;
   struct tm *tmp;
   t = time(NULL);
   tmp = localtime(&t);
   strftime(outstr, sizeof(outstr), "%F-%T", tmp);
   FILE* file = fopen("/tmp/hb.log", "a+");
   fprintf(file, "%s: casual: %s *** %s\n", outstr, function, text);
   fclose( file);
}
void dd(const char * text)
{
   char outstr[20];
   time_t t;
   struct tm *tmp;
   t = time(NULL);
   tmp = localtime(&t);
   strftime(outstr, sizeof(outstr), "%F-%T", tmp);
   FILE* file = fopen("/tmp/hb.log", "a+");
   fprintf(file, "%s: casual: ***\n", outstr);
   fprintf(file, "%s:", text);
   fprintf(file, "\n");
   fclose( file);
}
