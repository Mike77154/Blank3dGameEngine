/* gfo_parser.c */
#include "gfo_parser.h"

/* helpers */
static void next(gfo_parser* p){ p->cur = gfo_lex_next(&p->lx); }
static int is(gfo_parser* p, gfo_tok_kind k){ return p->cur.kind == k; }
static void skip_newlines(gfo_parser* p){ while(is(p, GFO_TOK_NEWLINE)) next(p); }

static int expect(gfo_parser* p, gfo_tok_kind k){
  if(!is(p,k)){ p->error = GFO_ERR_PARSE; return 0; }
  next(p);
  return 1;
}

/* parse value: number | bool | string | ident(path/symbol) | '?' */
static gfo_value parse_value(gfo_parser* p){
  gfo_value v; v.t = GFO_VAL_NONE;
  if(is(p, GFO_TOK_NUMBER)){
    /* int32 */
    gfo_i32 sign = 1;
    const char* s = p->cur.lexeme.ptr;
    gfo_u16 n = p->cur.lexeme.len;
    gfo_u16 i = 0;
    gfo_i32 acc = 0;
    if(n>0 && s[0]=='-'){ sign = -1; i=1; }
    else if(n>0 && s[0]=='+'){ sign = 1; i=1; }
    for(; i<n; i++){
      if(s[i]<'0'||s[i]>'9') break;
      acc = (acc*10) + (gfo_i32)(s[i]-'0');
    }
    v.t = GFO_VAL_I32;
    v.v.i = acc * sign;
    next(p);
    return v;
  }
  if(is(p, GFO_TOK_STRING)){
    v.t = GFO_VAL_STR;
    v.v.s = p->cur.lexeme;
    next(p);
    return v;
  }
  if(is(p, GFO_TOK_IDENT)){
    /* true/false/? */
    if(p->cur.lexeme.len==4 && p->cur.lexeme.ptr[0]=='t' && p->cur.lexeme.ptr[1]=='r' &&
       p->cur.lexeme.ptr[2]=='u' && p->cur.lexeme.ptr[3]=='e'){
      v.t = GFO_VAL_BOOL; v.v.b = 1; next(p); return v;
    }
    if(p->cur.lexeme.len==5 && p->cur.lexeme.ptr[0]=='f' && p->cur.lexeme.ptr[1]=='a' &&
       p->cur.lexeme.ptr[2]=='l' && p->cur.lexeme.ptr[3]=='s' && p->cur.lexeme.ptr[4]=='e'){
      v.t = GFO_VAL_BOOL; v.v.b = 0; next(p); return v;
    }
    if(p->cur.lexeme.len==1 && p->cur.lexeme.ptr[0]=='?'){
      v.t = GFO_VAL_UNKNOWN; next(p); return v;
    }
    /* heuristics: if contains '/' or '.' treat as PATH else SYMBOL */
    {
      gfo_u16 i;
      int is_path = 0;
      for(i=0;i<p->cur.lexeme.len;i++){
        char c = p->cur.lexeme.ptr[i];
        if(c=='/' || c=='.'){ is_path = 1; break; }
      }
      if(is_path){ v.t = GFO_VAL_PATH; v.v.s = p->cur.lexeme; }
      else {
        gfo_u16 sid = 0;
        /* Symbols are interned and stored as u16 ids. */
        if(gfo_sym_intern(p->sym, p->cur.lexeme, &sid) != GFO_OK){
          p->error = GFO_ERR_OOM;
          return v;
        }
        v.t = GFO_VAL_SYM;
        v.v.sym = sid;
      }
    }
    next(p);
    return v;
  }
  /* allow bare '?' as ident via lexer fallback */
  if(is(p, GFO_TOK_IDENT) && p->cur.lexeme.len==1 && p->cur.lexeme.ptr[0]=='?'){
    v.t = GFO_VAL_UNKNOWN; next(p); return v;
  }
  p->error = GFO_ERR_PARSE;
  return v;
}

/* parse arglist: value (',' value)* ) */
static int parse_args_into_ast(gfo_parser* p, gfo_u16 inv_node){
  /* store argc in node->b; then store each arg as child node kind ASSIGN-like with val */
  gfo_u16 argc = 0;
  if(is(p, GFO_TOK_RPAREN)){
    return 1;
  }
  for(;;){
    gfo_u16 argn;
    gfo_value v = parse_value(p);
    if(p->error) return 0;
    if(gfo_ast_new(p->ast, GFO_AST_ASSIGN, &argn) != GFO_OK){ p->error = GFO_ERR_OOM; return 0; }
    p->ast->nodes[argn].val = v;
    gfo_ast_add_child(p->ast, inv_node, argn);
    argc++;
    if(is(p, GFO_TOK_COMMA)){ next(p); continue; }
    break;
  }
  p->ast->nodes[inv_node].b = argc;
  return 1;
}

void gfo_parse_init(gfo_parser* p, const char* src, gfo_u32 len, gfo_symtab* sym, gfo_ast* ast){
  gfo_lex_init(&p->lx, src, len);
  p->sym = sym;
  p->ast = ast;
  p->error = GFO_OK;
  p->cur.kind = GFO_TOK_EOF;
  p->cur.lexeme.ptr = src;
  p->cur.lexeme.len = 0;
  p->cur.line = 1;
  p->cur.col = 1;
  next(p);
}

static int intern(gfo_parser* p, gfo_str s, gfo_u16* out){
  return gfo_sym_intern(p->sym, s, out) == GFO_OK;
}

/* section header: [ ident [# ident] ] */
static int parse_section(gfo_parser* p, gfo_u16 file_node){
  gfo_u16 sec, type_id=0, inst_id=0;
  if(!expect(p, GFO_TOK_LBRACK)) return 0;
  if(!is(p, GFO_TOK_IDENT)){ p->error=GFO_ERR_PARSE; return 0; }
  if(!intern(p, p->cur.lexeme, &type_id)){ p->error=GFO_ERR_OOM; return 0; }
  next(p);

  if(is(p, GFO_TOK_HASH)){
    next(p);
    if(!is(p, GFO_TOK_IDENT)){ p->error=GFO_ERR_PARSE; return 0; }
    if(!intern(p, p->cur.lexeme, &inst_id)){ p->error=GFO_ERR_OOM; return 0; }
    next(p);
  } else {
    inst_id = 0xFFFF;
  }

  if(!expect(p, GFO_TOK_RBRACK)) return 0;
  /* optional newline */
  if(is(p, GFO_TOK_NEWLINE)) next(p);

  if(gfo_ast_new(p->ast, GFO_AST_SECTION, &sec) != GFO_OK){ p->error=GFO_ERR_OOM; return 0; }
  p->ast->nodes[sec].a = type_id;      /* type symbol */
  p->ast->nodes[sec].b = inst_id;      /* optional instance id symbol */
  p->ast->nodes[sec].line = p->cur.line;
  gfo_ast_add_child(p->ast, file_node, sec);

  /* parse statements until next [ or EOF */
  for(;;){
    skip_newlines(p);
    if(is(p, GFO_TOK_EOF) || is(p, GFO_TOK_LBRACK)) break;

    /* lifecycle: *ident */
    if(is(p, GFO_TOK_STAR)){
      gfo_u16 lc_node, lc_sym;
      next(p);
      if(!is(p, GFO_TOK_IDENT)){ p->error=GFO_ERR_PARSE; return 0; }
      if(!intern(p, p->cur.lexeme, &lc_sym)){ p->error=GFO_ERR_OOM; return 0; }
      next(p);
      if(is(p, GFO_TOK_NEWLINE)) next(p);

      if(gfo_ast_new(p->ast, GFO_AST_LIFECYCLE, &lc_node) != GFO_OK){ p->error=GFO_ERR_OOM; return 0; }
      p->ast->nodes[lc_node].a = lc_sym; /* lifecycle name symbol */
      gfo_ast_add_child(p->ast, sec, lc_node);

      /* actions until blank line or next directive */
      for(;;){
        /* stop on new section or new lifecycle or EOF */
        if(is(p, GFO_TOK_EOF) || is(p, GFO_TOK_LBRACK)) break;
        if(is(p, GFO_TOK_STAR)) break;

        skip_newlines(p);
        if(is(p, GFO_TOK_EOF) || is(p, GFO_TOK_LBRACK) || is(p, GFO_TOK_STAR)) break;

        /* handler: @ident */
        if(is(p, GFO_TOK_AT)){
          gfo_u16 hnode, hid;
          next(p);
          if(!is(p, GFO_TOK_IDENT)){ p->error=GFO_ERR_PARSE; return 0; }
          if(!intern(p, p->cur.lexeme, &hid)){ p->error=GFO_ERR_OOM; return 0; }
          next(p);
          if(is(p, GFO_TOK_NEWLINE)) next(p);

          if(gfo_ast_new(p->ast, GFO_AST_HANDLER, &hnode) != GFO_OK){ p->error=GFO_ERR_OOM; return 0; }
          p->ast->nodes[hnode].a = hid;
          gfo_ast_add_child(p->ast, lc_node, hnode);
          continue;
        }

        /* assignment or invoker or script block */
        if(is(p, GFO_TOK_IDENT)){
          gfo_token name = p->cur;
          gfo_u16 name_id;
          if(!intern(p, name.lexeme, &name_id)){ p->error=GFO_ERR_OOM; return 0; }
          next(p);

          /* script block: script : lang NEWLINE ... : NEWLINE */
          if(name.lexeme.len==6 &&
             name.lexeme.ptr[0]=='s'&&name.lexeme.ptr[1]=='c'&&name.lexeme.ptr[2]=='r'&&name.lexeme.ptr[3]=='i'&&name.lexeme.ptr[4]=='p'&&name.lexeme.ptr[5]=='t' &&
             is(p, GFO_TOK_COLON))
          {
            gfo_u16 sbnode, lang_id;
            const char* block_start;
            const char* block_end;
            next(p); /* consume ':' */
            if(!is(p, GFO_TOK_IDENT)){ p->error=GFO_ERR_PARSE; return 0; }
            if(!intern(p, p->cur.lexeme, &lang_id)){ p->error=GFO_ERR_OOM; return 0; }
            next(p);
            /* expect newline */
            if(is(p, GFO_TOK_NEWLINE)) next(p);

            /* now capture raw lines until a line that is exactly ":" */
            block_start = p->cur.lexeme.ptr; /* start of first token after the newline */
            /* brute-scan: find "\n:\n" or "\n:\r\n" etc */
            block_end = block_start;
            {
              const char* s = block_start;
              const char* end = p->lx.src + p->lx.len;
              int found = 0;
              while(s < end){
                if(*s=='\n'){
                  const char* t = s+1;
                  if(t < end && *t==':'){
                    const char* u = t+1;
                    /* allow optional \r then \n or \n */
                    if(u < end && *u=='\r') u++;
                    if(u < end && *u=='\n'){
                      found = 1;
                      block_end = s+1; /* include newline before ':'? store up to this newline start+1? */
                      /* advance lexer position to after ":\n" */
                      p->lx.pos = (gfo_u32)(u - p->lx.src) + 1;
                      p->lx.line += 0; /* line/col will be re-synced by next token reads; accept slight mismatch */
                      p->lx.col = 1;
                      next(p); /* set current token after block (from new position) */
                      break;
                    }
                  }
                }
                s++;
              }
              if(!found){ p->error=GFO_ERR_PARSE; return 0; }
            }

            if(gfo_ast_new(p->ast, GFO_AST_SCRIPT_BLOCK, &sbnode) != GFO_OK){ p->error=GFO_ERR_OOM; return 0; }
            p->ast->nodes[sbnode].a = lang_id;
            /* store block text slice */
            p->ast->nodes[sbnode].text.ptr = block_start;
            p->ast->nodes[sbnode].text.len = (gfo_u16)(block_end - block_start);
            gfo_ast_add_child(p->ast, lc_node, sbnode);
            continue;
          }

          /* assignment: ident = value */
          if(is(p, GFO_TOK_EQ)){
            gfo_u16 anode;
            next(p);
            {
              gfo_value v = parse_value(p);
              if(p->error) return 0;
              if(is(p, GFO_TOK_NEWLINE)) next(p);

              if(gfo_ast_new(p->ast, GFO_AST_ASSIGN, &anode) != GFO_OK){ p->error=GFO_ERR_OOM; return 0; }
              p->ast->nodes[anode].a = name_id; /* key symbol */
              p->ast->nodes[anode].val = v;
              gfo_ast_add_child(p->ast, lc_node, anode);
              continue;
            }
          }

          /* invoker: ident ( args? ) */
          if(is(p, GFO_TOK_LPAREN)){
            gfo_u16 invnode;
            next(p);
            if(gfo_ast_new(p->ast, GFO_AST_INVOKER, &invnode) != GFO_OK){ p->error=GFO_ERR_OOM; return 0; }
            p->ast->nodes[invnode].a = name_id; /* invoker symbol */
            /* parse args */
            if(!parse_args_into_ast(p, invnode)) return 0;
            if(!expect(p, GFO_TOK_RPAREN)) return 0;
            if(is(p, GFO_TOK_NEWLINE)) next(p);
            gfo_ast_add_child(p->ast, lc_node, invnode);
            continue;
          }

          /* bare ident line: treat as invoker with 0 args (GM style) */
          {
            gfo_u16 invnode;
            if(gfo_ast_new(p->ast, GFO_AST_INVOKER, &invnode) != GFO_OK){ p->error=GFO_ERR_OOM; return 0; }
            p->ast->nodes[invnode].a = name_id;
            p->ast->nodes[invnode].b = 0;
            if(is(p, GFO_TOK_NEWLINE)) next(p);
            gfo_ast_add_child(p->ast, lc_node, invnode);
            continue;
          }
        }

        /* fallback */
        p->error = GFO_ERR_PARSE;
        return 0;
      }

      continue;
    }

    /* top-level section assignments (outside lifecycle) */
    if(is(p, GFO_TOK_IDENT)){
      gfo_token key = p->cur;
      gfo_u16 key_id, anode;
      if(!intern(p, key.lexeme, &key_id)){ p->error=GFO_ERR_OOM; return 0; }
      next(p);
      if(!expect(p, GFO_TOK_EQ)) return 0;
      {
        gfo_value v = parse_value(p);
        if(p->error) return 0;
        if(is(p, GFO_TOK_NEWLINE)) next(p);
        if(gfo_ast_new(p->ast, GFO_AST_ASSIGN, &anode) != GFO_OK){ p->error=GFO_ERR_OOM; return 0; }
        p->ast->nodes[anode].a = key_id;
        p->ast->nodes[anode].val = v;
        gfo_ast_add_child(p->ast, sec, anode);
        continue;
      }
    }

    /* unknown statement */
    p->error = GFO_ERR_PARSE;
    return 0;
  }

  return 1;
}

int gfo_parse_file(gfo_parser* p){
  gfo_u16 file_node;
  if(gfo_ast_new(p->ast, GFO_AST_FILE, &file_node) != GFO_OK) return GFO_ERR_OOM;
  p->ast->root = file_node;

  skip_newlines(p);
  while(!is(p, GFO_TOK_EOF)){
    if(is(p, GFO_TOK_LBRACK)){
      if(!parse_section(p, file_node)) return p->error;
      skip_newlines(p);
      continue;
    }
    /* allow stray newlines/comments; otherwise error */
    if(is(p, GFO_TOK_NEWLINE)){ next(p); continue; }
    p->error = GFO_ERR_PARSE;
    return p->error;
  }
  return GFO_OK;
}
