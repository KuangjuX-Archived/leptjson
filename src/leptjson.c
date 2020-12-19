#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */
#include <stdio.h>   /* debug */
#include <math.h>


#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define ISZERO(ch)          ((ch) == '0')
#define JUDGEBIT(n)         ((((((unsigned long)(n) & (unsigned long)0x7fffffff) << 1) >> 53) & 1))
#define JUDGEBITS(n)        (((((unsigned long)(n) & (unsigned long)0x7fffffff) << 1) >> 53) >= 0x07ff)

typedef struct {
    const char* json;
}lept_context;

static int lept_judge_singular(lept_context* c, lept_value* v);

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v , char* s, short len, int type){
    int i = 0;
    EXPECT(c, s[0]);
    for(i; i < len-1; i++){
        if(c->json[i] != s[i+1])
            return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += (len-1);
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    char* end;
    /* \TODO validate number */
    v->n = strtod(c->json, &end);

    if (c->json == end)
        return LEPT_PARSE_INVALID_VALUE;
  

    const char* s = c->json;
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
    if(isinf(v->n))
        return LEPT_PARSE_NUMBER_TOO_BIG;
            

    c->json = end;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_judge_singular(lept_context* c, lept_value* v){
    lept_parse_whitespace(c);
    if(*(c->json) != '\0'){
        v->type = LEPT_NULL;
        return LEPT_PARSE_ROOT_NOT_SINGULAR;
    }
    return LEPT_PARSE_OK;
}


static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 'n':  return lept_parse_literal(c, v, "null", 4, LEPT_NULL);
        case 't':  return lept_parse_literal(c, v, "true", 4, LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", 5, LEPT_FALSE);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    int res = lept_parse_value(&c, v);
    if(res == LEPT_PARSE_OK){
        return lept_judge_singular(&c, v);
    }

    return res;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}