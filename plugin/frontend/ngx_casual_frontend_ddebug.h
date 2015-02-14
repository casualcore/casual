
#ifndef _NGX_CASUAL_DDEBUG_H_
#define _NGX_CASUAL_DDEBUG_H_


#define DD( A ) dd_func( A, __func__);
void dd_func(const char * text, const char * function);
void dd(const char * text);

#endif /* _NGX_CASUAL_DDEBUG_H_ */
