/*
 * FILE based functions (can only be used AFTER relocation!)
 */

#ifndef	__DEBUGSERIAL_H
#define __DEBUGSERIAL_H
#if 0
#define _U  0x1     /* Upper case */
#define _L  0x2     /* Lower case */
#define _N  0x4     /* Numeral (digit) */
#define _S  0x8     /* Whitespace */
#define _P  0x10    /* Punctuation */
#define _C  0x20    /* Control character */
#define _X  0x40    /* Hex */
#define _B  0x80    /* blank */


#define _INTSIZEOF(n) ((sizeof(n)+sizeof(int)-1)&~(sizeof(int) - 1)) 

#define va_start(ap,v) ( ap = (va_list)&v + _INTSIZEOF(v) ) 

#define va_arg(ap,t) ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)))

#define va_end(ap) ( ap = (va_list)0 )  

#endif
#endif  /* __DEBUGSERIAL_H */
