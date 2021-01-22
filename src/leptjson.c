#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */
#include <stdio.h>   /* debug */
#include <math.h>   /* judge inf */
#include <string.h>
#include <malloc.h>

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define ISZERO(ch)          ((ch) == '0')
/*#define JUDGEBIT(n)         ((((((unsigned long)(n) & (unsigned long)0x7fffffff) << 1) >> 53) & 1))
#define JUDGEBITS(n)        (((((unsigned long)(n) & (unsigned long)0x7fffffff) << 1) >> 53) >= 0x07ff)*/

#define PUTC(c, ch)         do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)


typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
}lept_context;


static void* lept_context_push(lept_context* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0)
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            c->size += c->size >> 1;  /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v , char* s, short len, int type){
    int i = 0;
    EXPECT(c, s[0]);
    for(; i < len-1; i++){
        if(c->json[i] != s[i+1])
            return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += (len-1);
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    char* end;
    const char* s = c->json;
    /* \TODO validate number */
    v->u.n = strtod(c->json, &end);

    if (c->json == end)
        return LEPT_PARSE_INVALID_VALUE;
  

    char ch = s[0];

    /*judge the first char*/
    if((!ISDIGIT(ch)) && ch != '-')
        return LEPT_PARSE_INVALID_VALUE;

    /*judge the char after '0'*/
    if(ISZERO(ch) && !(s[1]== '.' || s[1] == 'e' || s[1] == 'E' || s[1] == '\0')){
        return LEPT_PARSE_ROOT_NOT_SINGULAR;
    }

    /*judge the least digit after '.'*/
    while(*s != '.'){
        s++;
    }
    s++;
    if(!ISDIGIT(*s))
        return LEPT_PARSE_INVALID_VALUE;

    /*judge the range of the number*/
    if(isinf(v->u.n))
        return LEPT_PARSE_NUMBER_TOO_BIG;
            

    c->json = end;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}


static const char* lept_parse_hex4(const char* p, unsigned* u, int *flag) {
    /* \TODO */
    unsigned low = 0;
    *u = 0;
    int i;
    for(i = 3; i >= 0; i--){
        if((*p) >= '0' && (*p) <= '9'){
            *u += ((unsigned)((*p) - '0') << (4 * i));
        }else if((*p) >= 'A' && (*p) <= 'F'){
            *u += (((unsigned)((*p) - 'A') + 10) << (4 * i));
        }else if((*p) >= 'a' && (*p) <= 'f'){
            *u += (((unsigned)((*p) - 'a') + 10) << (4 * i));
        }else{
            *u = 0;
            p = NULL;
            return p;
        }

        p++;
    }

    if(*u >= 0xD800 && *u <= 0xDBFF){
        if(*p == '\\' && *(p + 1) == 'u'){
            p = p + 2;
            for(i = 3; i >= 0; i--){
                if((*p) >= '0' && (*p) <= '9'){
                    low += ((unsigned)((*p) - '0') << (4 * i));
                }else if((*p) >= 'A' && (*p) <= 'F'){
                    low += (((unsigned)((*p) - 'A') + 10) << (4 * i));
                }else if((*p) >= 'a' && (*p) <= 'f'){
                    low += (((unsigned)((*p) - 'a') + 10) << (4 * i));
                }else{
                    *u = 0;
                    p = NULL;
                    return p;
                }
                p++;
            }

            if(low >= 0xDC00 && low <= 0xDFFF){
                *u = 0x10000 + (*u - 0xD800) * 0x400 + (low - 0xDC00);
            }else{
                *flag = 1;
            }
        }else {
            *flag = 1;
        }
        
        
    }
    
    return p;
}

static void lept_encode_utf8(lept_context* c, unsigned u) {
    /* \TODO */
    if(u >= 0 && u <= 0x007F){
        PUTC(c, u & 0xFF);
    }else if(u >= 0x0080 && u <= 0x07FF){
        PUTC(c, (0xC0 | ((u >> 6) & 0xFF)));
        PUTC(c, (0x80 | (u & 0x3F)));
    }else if(u >= 0x0800 && u <= 0xFFFF){
        PUTC(c, (0xE0 | ((u >> 12) & 0xFF)));
        PUTC(c, (0x80 | ((u >> 6) & 0x3F)));
        PUTC(c, (0x80 | (u & 0x3F)));
    }else if(u >= 0x10000 && u <= 0x10FFFF){
        PUTC(c, (0xF0 | ((u >> 18) & 0xFF)));
        PUTC(c, (0x80 | ((u >> 12) & 0x3F)));
        PUTC(c, (0x80 | ((u >> 6) & 0x3F)));
        PUTC(c, (0x80 | (u  & 0x3F)));
    }
    
    
}

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)


static int lept_parse_string(lept_context* c, lept_value* v) {
    size_t head = c->top, len;
    unsigned u;
    const char* p;
    int surrogate_flag = 0; 

    EXPECT(c, '\"');
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
            case '\"':
                len = c->top - head;
                lept_set_string(v, (const char*)lept_context_pop(c, len), len);
                c->json = p;
                return LEPT_PARSE_OK;
            case '\0':
               STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK); 
            case '\\':
                ch = *p++;
               
                if(ch == 'n'){
                    PUTC(c, '\n');
                    break;
                }
                else if(ch == 't'){
                    PUTC(c, '\t');
                    break;
                }
                else if(ch == 'f'){
                    PUTC(c, '\f');
                    break;
                }
                else if(ch == 'r'){
                    PUTC(c, '\r');
                    break;
                }
                else if(ch == 'b'){
                    PUTC(c, '\b');
                    break;
                }
                else if(ch == '/'){
                    PUTC(c, '/');
                    break;
                }
                else if(ch == '\\'){
                    PUTC(c, '\\');
                    break;
                }
                else if(ch == '"'){
                    PUTC(c, '"');
                    break;
                }
                else if(ch == 'u'){
                    if (!(p = lept_parse_hex4(p, &u, &surrogate_flag)))
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                        /* \TODO surrogate handling */
                        if(surrogate_flag == 1)
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                        lept_encode_utf8(c, u);
                        break;
                }
                else{
                   STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);
                }


            
                
            default:
                if((ch >= '\x01' && ch <= '\x1F') || (ch == '\x22') || (ch == '\x5C')){
                    STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
                }
                PUTC(c, ch);
        }
    }
}


static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 'n':  return lept_parse_literal(c, v, "null", 4, LEPT_NULL);
        case 't':  return lept_parse_literal(c, v, "true", 4, LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", 5, LEPT_FALSE);
        default:   return lept_parse_number(c, v);
        case '"':  return lept_parse_string(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int res;
    assert(v != NULL);
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    lept_init(v);
    lept_parse_whitespace(&c);
    if ((res = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            res = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);
    free(c.stack);
    return res;
}

void lept_free(lept_value* v) {
    assert(v != NULL);
    if (v->type == LEPT_STRING)
        free(v->u.s.s);
    v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

int lept_get_boolean(const lept_value* v) {
    /* \TODO */
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    if(v->type == LEPT_TRUE)return TRUE;
    else return FALSE;
}

void lept_set_boolean(lept_value* v, int b) {
    /* \TODO */
    if(b == 0)v->type = LEPT_FALSE;
    else v->type = LEPT_TRUE;

}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
    /* \TODO */
    lept_free(v);
    v->type = LEPT_NUMBER;
    v->u.n = n;
}

const char* lept_get_string(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    lept_free(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = LEPT_STRING;
}