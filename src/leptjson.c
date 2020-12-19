#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

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

static int lept_parse_basic(lept_context* c, lept_value* v , char* s, short len, int type){
    EXPECT(c, s[0]);
    int i;
    for(i = 0; i < len-1; i++){
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
        case 'n':  return lept_parse_basic(c, v, "null", 4, LEPT_NULL);
        case 't':  return lept_parse_basic(c, v, "true", 4, LEPT_TRUE);
        case 'f':  return lept_parse_basic(c, v, "false", 5, LEPT_FALSE);
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