/* Grammar (× binds tighter than →; both nest to the right):
     decl   := 'postulate' NAME ':' term
             | 'rec'? NAME ':' term NAME NAME* '=' term
             | 'data' NAME group* ':' 'Type' 'where' ctor+
             | 'replicas' NAME+
             | 'atoms' NAME ':=' member ('∣' member)*
             | 'replicated' NAME item+
     ctor   := NAME ':' term
     group  := '(' NAME+ ':' term ')'
     member := NAME | NAT        (all members are names or all are numerals)
     item   := 'state' NAME ':' term ('(' 'init' NAT ')')?
             | 'op' NAME group* ':=' term
             | 'invariant' term
             | 'allocate' NAME 'by' 'escrow' '(' share (',' share)* ')'
     share  := NAME '↦' NAT
     term   := ('λ' | '\') (NAME | group)+ '→' term
             | 'fun' (NAME | group)+ '=>' term
             | prod ('→' term)?
     prod   := group '→' term | group '×' prod | app ('×' prod)?
     app    := atom atom*
     atom   := NAME | NAT | 'Type' NAT | 'Sigma' group atom | '(' term ')'
   Layout: a declaration starts at column 1. A token that is the first on its
   line, at a column <= STOP, ends the current term as end of input does. The
   clause line repeats the signature name. In a `data` block the first
   constructor sets the constructor column; each constructor starts a new
   line at that column. In a `replicated` block the first item sets the
   item column in the same way. */
#include <string.h>

#include "front/parser.h"

#define PARSE_MAX_DEPTH 1000

typedef struct {
  const Token *toks;
  size_t count;
  size_t pos;
  Arena *arena;
  Diag *diag;
  const char *source;
  const char *def;
  Origin origin;
  int depth;
  int stop;
} Parser;

/* One binder group: `(x y : A)` gives two names that share one type. */
typedef struct {
  const char **names;
  size_t count;
  const Term *type;
  const Token *tok;
} Binders;

static const Term *parse_term(Parser *p);
static const Term *parse_atom(Parser *p);
static const Term *parse_prod(Parser *p);

static const Token *peek_at(const Parser *p, size_t ahead) {
  size_t i = p->pos + ahead;
  return &p->toks[i < p->count ? i : p->count - 1];
}

static const Token *cur(const Parser *p) {
  return peek_at(p, 0);
}

static int first_on_line(const Parser *p) {
  return p->pos == 0 || p->toks[p->pos].line != p->toks[p->pos - 1].line;
}

/* A line-start token at or left of the stop column ends the term. */
static int at_stop(const Parser *p) {
  return cur(p)->kind != TOK_EOF && first_on_line(p) && cur(p)->col <= p->stop;
}

/* The kind that the grammar sees: end of input at a stop. */
static TokKind kind_of(const Parser *p) {
  return at_stop(p) ? TOK_EOF : cur(p)->kind;
}

static int at(const Parser *p, TokKind kind) {
  return kind_of(p) == kind;
}

static void bump(Parser *p) {
  p->pos += p->pos + 1 < p->count ? 1u : 0u;
}

static int eat(Parser *p, TokKind kind) {
  if (!at(p, kind)) return 0;
  bump(p);
  return 1;
}

static int oom(Parser *p) {
  return diag_fail(p->diag, "OOM", p->def, "%s: the arena limit is reached", p->source);
}

/* The source bytes of TOK, at most 80. */
static void token_found(const Token *tok, char *out, size_t size) {
  int width = (int)(tok->len < 80 ? tok->len : 80);
  snprintf(out, size, "%.*s", width, tok->text);
}

static Span token_span(const Parser *p, const Token *tok) {
  Span span = {p->source, tok->line, tok->col, tok->line, tok->kind == TOK_EOF ? tok->col : tok->end_col};
  return span;
}

/* FOUND is the token text, or the layout reason at a stop. */
static int fail_at(Parser *p, const Token *tok, const char *what) {
  int stop = tok == cur(p) && at_stop(p);
  char found[96];
  token_found(tok, found, sizeof found);
  if (tok->kind == TOK_EOF) snprintf(found, sizeof found, "end of input");
  if (stop) snprintf(found, sizeof found, "a new line at column %d", tok->col);
  return diag_fail_at(p->diag, "PARSE_EXPECT", p->def, token_span(p, tok), what, found, NULL,
                      "%s:%d:%d: expected %s, found %s", p->source, tok->line, tok->col, what,
                      stop ? found : tok_kind_name(tok->kind));
}

static int expect(Parser *p, TokKind kind, const char *what) {
  return eat(p, kind) ? 1 : fail_at(p, cur(p), what);
}

static int enter(Parser *p) {
  p->depth++;
  if (p->depth <= PARSE_MAX_DEPTH) return 1;
  const Token *tok = cur(p);
  Span span = {p->source, tok->line, tok->col, tok->line, tok->kind == TOK_EOF ? tok->col : tok->end_col};
  return diag_fail_at(p->diag, "PARSE_DEPTH", p->def, span, NULL, NULL, NULL, "%s:%d:%d: terms nest deeper than %d levels",
                      p->source, tok->line, tok->col, PARSE_MAX_DEPTH);
}

static const char *token_text(Parser *p, const Token *tok) {
  size_t len = 0;
  const char *core = tok_core_text(tok, &len);
  char *text = arena_strndup(p->arena, core, len);
  if (text == NULL) oom(p);
  return text;
}

static const char *take_name(Parser *p, const char *what) {
  if (!at(p, TOK_IDENT)) {
    fail_at(p, cur(p), what);
    return NULL;
  }
  const Token *tok = cur(p);
  bump(p);
  return token_text(p, tok);
}

static Term *make_term(Parser *p, TermKind kind, int line, int col) {
  Term *term = arena_alloc(p->arena, sizeof *term);
  if (term == NULL) {
    oom(p);
    return NULL;
  }
  term->kind = kind;
  term->line = line;
  term->col = col;
  return term;
}

/* Grows an arena array by doubling. The old block stays in the arena. */
static void *grow(Parser *p, void *items, size_t count, size_t *cap, size_t size) {
  if (count < *cap) return items;
  size_t next = *cap == 0 ? 4 : *cap * 2;
  void *fresh = arena_alloc(p->arena, next * size);
  if (fresh == NULL) {
    oom(p);
    return NULL;
  }
  if (count > 0) memcpy(fresh, items, count * size);
  *cap = next;
  return fresh;
}

static int starts_atom(TokKind kind) {
  switch (kind) {
    case TOK_IDENT:
    case TOK_NAT:
    case TOK_LPAREN:
    case TOK_KW_TYPE:
    case TOK_KW_SIGMA:
    case TOK_LBRACK:
      return 1;
    case TOK_EOF:
    case TOK_RPAREN:
    case TOK_COLON:
    case TOK_DEFINE:
    case TOK_EQUALS:
    case TOK_ARROW:
    case TOK_FATARROW:
    case TOK_TIMES:
    case TOK_LAMBDA:
    case TOK_KW_REC:
    case TOK_KW_FUN:
    case TOK_KW_DATA:
    case TOK_KW_CODATA:
    case TOK_KW_WHERE:
    case TOK_KW_POSTULATE:
    case TOK_TILDE: /* `p ~ i` is not an application; write `p (~ i)` */
    case TOK_MEET:
    case TOK_JOIN:
    case TOK_RBRACK:
    case TOK_COMMA:
    case TOK_MAPSTO:
    case TOK_LARROW:
    case TOK_SEMI:
    case TOK_BAR:
    case TOK_KW_REPLICAS:
    case TOK_KW_ATOMS:
    case TOK_KW_REPLICATED:
    case TOK_KW_STATE:
    case TOK_KW_OP:
    case TOK_KW_INVARIANT:
    case TOK_KW_ALLOCATE:
    case TOK_KW_BY:
    case TOK_KW_ESCROW:
    case TOK_KW_INIT:
      return 0;
  }
  return 0;
}

/* `(` NAME+ `:` starts a binder group. `(f x)` does not. */
static int at_binder_group(const Parser *p) {
  if (!at(p, TOK_LPAREN) || peek_at(p, 1)->kind != TOK_IDENT) return 0;
  size_t ahead = 2;
  while (peek_at(p, ahead)->kind == TOK_IDENT) ahead++;
  return peek_at(p, ahead)->kind == TOK_COLON;
}

static int parse_group(Parser *p, Binders *out) {
  out->tok = cur(p);
  if (!expect(p, TOK_LPAREN, "'('")) return 0;
  size_t start = p->pos;
  while (at(p, TOK_IDENT)) bump(p);
  out->count = p->pos - start;
  if (out->count == 0) return fail_at(p, cur(p), "a binder name");
  out->names = arena_alloc(p->arena, out->count * sizeof *out->names);
  if (out->names == NULL) return oom(p);
  for (size_t i = 0; i < out->count; i++) {
    out->names[i] = token_text(p, &p->toks[start + i]);
    if (out->names[i] == NULL) return 0;
  }
  if (!expect(p, TOK_COLON, "':'")) return 0;
  out->type = parse_term(p);
  return out->type != NULL && expect(p, TOK_RPAREN, "')'");
}

/* A bare lambda binder `x` has no type. The checker infers it. */
static int parse_bare(Parser *p, Binders *out) {
  out->tok = cur(p);
  out->names = arena_alloc(p->arena, sizeof *out->names);
  if (out->names == NULL) return oom(p);
  out->names[0] = take_name(p, "a binder");
  out->count = 1;
  out->type = NULL;
  return out->names[0] != NULL;
}

/* Builds the nested binders of B around BODY, the last name innermost. */
static const Term *wrap(Parser *p, TermKind kind, const Binders *b, const Term *body) {
  const Term *acc = body;
  for (size_t i = b->count; i > 0 && acc != NULL; i--) {
    Term *node = make_term(p, kind, b->tok->line, b->tok->col);
    if (node == NULL) return NULL;
    node->name = b->names[i - 1];
    node->left = b->type;
    node->right = acc;
    acc = node;
  }
  return acc;
}

/* Reads binders up to CLOSE, then the body. */
static const Term *parse_lambda_tail(Parser *p, TokKind close, const char *what) {
  if (eat(p, close)) return parse_term(p);
  Binders b;
  int ok = at(p, TOK_IDENT) ? parse_bare(p, &b) : at(p, TOK_LPAREN) ? parse_group(p, &b) : fail_at(p, cur(p), what);
  if (!ok || !enter(p)) return NULL;
  const Term *body = parse_lambda_tail(p, close, what);
  p->depth--;
  return body == NULL ? NULL : wrap(p, TERM_LAM, &b, body);
}

/* `λ x → t`, and the aliases `\ x → t` and `fun x => t`. */
static const Term *parse_lambda(Parser *p) {
  TokKind close = at(p, TOK_KW_FUN) ? TOK_FATARROW : TOK_ARROW;
  const char *what = close == TOK_FATARROW ? "a binder or '=>'" : "a binder or '→'";
  bump(p);
  if (at(p, close)) {
    fail_at(p, cur(p), "a binder after the lambda");
    return NULL;
  }
  return parse_lambda_tail(p, close, what);
}

static const Term *parse_universe(Parser *p) {
  const Token *tok = cur(p);
  bump(p);
  if (!at(p, TOK_NAT)) {
    fail_at(p, cur(p), "a universe level after 'Type'");
    return NULL;
  }
  Term *term = make_term(p, TERM_UNIV, tok->line, tok->col);
  if (term == NULL) return NULL;
  term->nat = cur(p)->nat;
  bump(p);
  return term;
}

static const Term *parse_sigma_tail(Parser *p) {
  if (!at_binder_group(p)) {
    fail_at(p, cur(p), "a binder (x : A) after 'Sigma'");
    return NULL;
  }
  Binders b;
  if (!parse_group(p, &b)) return NULL;
  const Term *body = parse_atom(p);
  return body == NULL ? NULL : wrap(p, TERM_SIGMA, &b, body);
}

/* `Sigma (x : A) Sigma (y : B) C` nests without parentheses, so it counts
   toward the depth limit. */
static const Term *parse_sigma(Parser *p) {
  bump(p);
  const Term *term = enter(p) ? parse_sigma_tail(p) : NULL;
  p->depth--;
  return term;
}

static const Term *parse_paren(Parser *p) {
  bump(p);
  const Term *inner = parse_term(p);
  return inner != NULL && expect(p, TOK_RPAREN, "')'") ? inner : NULL;
}

static const Term *parse_leaf(Parser *p, TermKind kind) {
  const Token *tok = cur(p);
  Term *term = make_term(p, kind, tok->line, tok->col);
  if (term == NULL) return NULL;
  term->nat = tok->nat;
  term->name = kind == TERM_VAR ? token_text(p, tok) : NULL;
  bump(p);
  return kind == TERM_VAR && term->name == NULL ? NULL : term;
}

/* `~ r`, `r ∧ s` and `r ∨ s` are applications of the builtin NAME. No
   identifier can spell these names. */
static const Term *make_op(Parser *p, const Token *op, const char *name, const Term *left, const Term *right) {
  Term *head = make_term(p, TERM_VAR, op->line, op->col);
  Term *one = head == NULL ? NULL : make_term(p, TERM_APP, right == NULL ? op->line : left->line, right == NULL ? op->col : left->col);
  if (one == NULL) return NULL;
  head->name = name;
  head->nat = 0;
  head->left = NULL;
  head->right = NULL;
  one->left = head;
  one->right = left;
  if (right == NULL) return one;
  Term *two = make_term(p, TERM_APP, left->line, left->col);
  if (two == NULL) return NULL;
  two->left = one;
  two->right = right;
  return two;
}

/* `~ r`: the operand is an atom, as in Agda. `~ ~ i` nests, so it counts
   toward the depth limit. */
static const Term *parse_neg(Parser *p) {
  const Token *op = cur(p);
  bump(p);
  const Term *arg = enter(p) ? parse_atom(p) : NULL;
  p->depth--;
  return arg == NULL ? NULL : make_op(p, op, "~", arg, NULL);
}

/* The empty system `[]` is the builtin name `[]`. No identifier can spell
   it. */
static const Term *make_nil(Parser *p, const Token *tok) {
  Term *nil = make_term(p, TERM_VAR, tok->line, tok->col);
  if (nil == NULL) return NULL;
  nil->name = "[]";
  nil->nat = 0;
  nil->left = NULL;
  nil->right = NULL;
  return nil;
}

/* The cells after `[` or `,`: `φ ↦ u`, then `,` and the next cells, or
   `]`. The cell `φ ↦ u` before REST is APP(APP(APP(VAR "↦", φ), u),
   REST), so no new term kind is necessary. Each cell counts toward the
   depth limit. */
static const Term *parse_cells(Parser *p) {
  const Term *face = parse_term(p);
  const Token *op = face == NULL ? NULL : cur(p);
  const Term *body = op != NULL && expect(p, TOK_MAPSTO, "'↦'") ? parse_term(p) : NULL;
  const Token *close = body == NULL ? NULL : cur(p);
  const Term *rest = NULL;
  if (close != NULL && eat(p, TOK_COMMA)) {
    rest = enter(p) ? parse_cells(p) : NULL;
    p->depth--;
  } else if (close != NULL && expect(p, TOK_RBRACK, "',' or ']'")) {
    rest = make_nil(p, close);
  }
  const Term *cell = rest == NULL ? NULL : make_op(p, op, "↦", face, body);
  Term *app = cell == NULL ? NULL : make_term(p, TERM_APP, face->line, face->col);
  if (app == NULL) return NULL;
  app->left = cell;
  app->right = rest;
  return app;
}

/* `[ φ ↦ u , ψ ↦ v ]` and `[]`. */
static const Term *parse_system(Parser *p) {
  const Token *open = cur(p);
  bump(p);
  if (eat(p, TOK_RBRACK)) return make_nil(p, open);
  const Term *cells = enter(p) ? parse_cells(p) : NULL;
  p->depth--;
  return cells;
}

static const Term *parse_atom(Parser *p) {
  const Token *tok = cur(p);
  switch (kind_of(p)) {
    case TOK_IDENT: return parse_leaf(p, TERM_VAR);
    case TOK_NAT: return parse_leaf(p, TERM_NAT);
    case TOK_LPAREN: return parse_paren(p);
    case TOK_KW_TYPE: return parse_universe(p);
    case TOK_KW_SIGMA: return parse_sigma(p);
    case TOK_TILDE: return parse_neg(p);
    case TOK_LBRACK: return parse_system(p);
    case TOK_BAR:
    case TOK_KW_REPLICAS:
    case TOK_KW_ATOMS:
    case TOK_KW_REPLICATED:
    case TOK_KW_STATE:
    case TOK_KW_OP:
    case TOK_KW_INVARIANT:
    case TOK_KW_ALLOCATE:
    case TOK_KW_BY:
    case TOK_KW_ESCROW:
    case TOK_KW_INIT:
    case TOK_EOF:
    case TOK_RPAREN:
    case TOK_COLON:
    case TOK_DEFINE:
    case TOK_EQUALS:
    case TOK_ARROW:
    case TOK_FATARROW:
    case TOK_TIMES:
    case TOK_LAMBDA:
    case TOK_KW_REC:
    case TOK_KW_FUN:
    case TOK_KW_DATA:
    case TOK_KW_CODATA:
    case TOK_KW_WHERE:
    case TOK_KW_POSTULATE:
    case TOK_MEET:
    case TOK_JOIN:
    case TOK_RBRACK:
    case TOK_COMMA:
    case TOK_MAPSTO:
    case TOK_LARROW:
    case TOK_SEMI:
      break;
  }
  fail_at(p, tok, "a term");
  return NULL;
}

/* `(init` follows the part type of a `state` item; no atom starts so. */
static int at_init(const Parser *p) {
  return at(p, TOK_LPAREN) && peek_at(p, 1)->kind == TOK_KW_INIT;
}

static const Term *parse_app(Parser *p) {
  const Term *head = parse_atom(p);
  while (head != NULL && starts_atom(kind_of(p)) && !at_init(p)) {
    const Term *arg = parse_atom(p);
    Term *app = arg == NULL ? NULL : make_term(p, TERM_APP, head->line, head->col);
    if (app == NULL) return NULL;
    app->left = head;
    app->right = arg;
    head = app;
  }
  return head;
}

/* A non-dependent `A → B` or `A × B` at the operator OP. */
static const Term *make_binary(Parser *p, TermKind kind, const Token *op, const Term *left, const Term *right) {
  Term *node = right == NULL ? NULL : make_term(p, kind, op->line, op->col);
  if (node == NULL) return NULL;
  node->name = NULL;
  node->left = left;
  node->right = right;
  return node;
}

/* The interval connectives. Level 0 is `∨`, level 1 is `∧` (tighter). */
static const struct {
  TokKind kind;
  const char *name;
} INFIX[] = {
  {TOK_JOIN, "∨"},
  {TOK_MEET, "∧"},
};

/* `r ∧ s` and `r ∨ s` bind looser than application and tighter than `×`
   and `→`. Both group to the right, as the value printer does. A chain
   nests, so its right operand counts toward the depth limit. */
static const Term *parse_infix(Parser *p, size_t level) {
  if (level == sizeof INFIX / sizeof INFIX[0]) return parse_app(p);
  const Term *head = parse_infix(p, level + 1u);
  if (head == NULL || !at(p, INFIX[level].kind)) return head;
  const Token *op = cur(p);
  bump(p);
  const Term *rest = enter(p) ? parse_infix(p, level) : NULL;
  p->depth--;
  return rest == NULL ? NULL : make_op(p, op, INFIX[level].name, head, rest);
}

static const Term *parse_binder_type(Parser *p) {
  Binders b;
  if (!parse_group(p, &b)) return NULL;
  int sigma = at(p, TOK_TIMES);
  if (!sigma && !at(p, TOK_ARROW)) {
    fail_at(p, cur(p), "'→' or '×' after a binder");
    return NULL;
  }
  bump(p);
  const Term *body = sigma ? parse_prod(p) : parse_term(p);
  return body == NULL ? NULL : wrap(p, sigma ? TERM_SIGMA : TERM_PI, &b, body);
}

static const Term *parse_prod_body(Parser *p) {
  if (at_binder_group(p)) return parse_binder_type(p);
  const Term *head = parse_infix(p, 0u);
  if (head == NULL || !at(p, TOK_TIMES)) return head;
  const Token *op = cur(p);
  bump(p);
  return make_binary(p, TERM_SIGMA, op, head, parse_prod(p));
}

/* `A × B × C` nests without parentheses, so it counts toward the depth
   limit. */
static const Term *parse_prod(Parser *p) {
  const Term *term = enter(p) ? parse_prod_body(p) : NULL;
  p->depth--;
  return term;
}

static const Term *parse_term_body(Parser *p) {
  if (at(p, TOK_LAMBDA) || at(p, TOK_KW_FUN)) return parse_lambda(p);
  const Term *head = parse_prod(p);
  if (head == NULL || !at(p, TOK_ARROW)) return head;
  const Token *op = cur(p);
  bump(p);
  return make_binary(p, TERM_PI, op, head, parse_term(p));
}

static const Term *parse_term(Parser *p) {
  const Term *term = enter(p) ? parse_term_body(p) : NULL;
  p->depth--;
  return term;
}

static int parse_fields(Parser *p, const Field **out, size_t *out_count) {
  Field *items = NULL;
  size_t count = 0;
  size_t cap = 0;
  while (at_binder_group(p)) {
    Binders b;
    if (!parse_group(p, &b)) return 0;
    for (size_t i = 0; i < b.count; i++) {
      items = grow(p, items, count, &cap, sizeof *items);
      if (items == NULL) return 0;
      items[count].name = b.names[i];
      items[count].type = b.type;
      /* The span of a field is its name: the names follow the '(' token. */
      items[count].line = b.tok[1 + i].line;
      items[count].col = b.tok[1 + i].col;
      count++;
    }
  }
  *out = items;
  *out_count = count;
  return 1;
}

/* NAME ':' term. The name is the first token of its line when no keyword
   comes before it, so the stop column is set only after it. */
static int parse_signature(Parser *p, Decl *d, const char *what) {
  d->name = take_name(p, what);
  p->stop = 1;
  if (d->name == NULL) return 0;
  p->def = d->name;
  if (!expect(p, TOK_COLON, "':'")) return 0;
  d->type = parse_term(p);
  return d->type != NULL;
}

static int parse_postulate(Parser *p, Decl *d) {
  bump(p);
  d->kind = DECL_AXIOM;
  return parse_signature(p, d, "a postulate name");
}

static int clause_name_fail(Parser *p, const Token *tok, const char *name) {
  char found[96];
  token_found(tok, found, sizeof found);
  return diag_fail_at(p->diag, "PARSE_CLAUSE_NAME", p->def, token_span(p, tok), name, found, name,
                      "%s:%d:%d: the clause name %s is not the signature name %s", p->source, tok->line, tok->col,
                      found, name);
}

static int no_signature_fail(Parser *p, const Token *tok) {
  char found[96];
  token_found(tok, found, sizeof found);
  return diag_fail_at(p->diag, "PARSE_CLAUSE_NAME", found, token_span(p, tok), "a signature `NAME : T` before the clause",
                      found, NULL, "%s:%d:%d: the clause %s has no signature before it", p->source, tok->line,
                      tok->col, found);
}

/* The clause `name x y = body` on a new line at column 1. The arguments
   become bare lambdas around the body. */
static int parse_clause(Parser *p, Decl *d) {
  p->stop = 0;
  const Token *head = cur(p);
  int line_start = head->kind == TOK_IDENT && first_on_line(p) && head->col == 1;
  if (!line_start || peek_at(p, 1)->kind == TOK_COLON) {
    return fail_at(p, head, "the clause of the signature on a new line at column 1");
  }
  size_t len = 0;
  const char *core = tok_core_text(head, &len);
  if (strlen(d->name) != len || memcmp(core, d->name, len) != 0) return clause_name_fail(p, head, d->name);
  bump(p);
  p->stop = 1;
  size_t start = p->pos;
  while (at(p, TOK_IDENT)) bump(p);
  Binders b = {NULL, p->pos - start, NULL, &p->toks[start]};
  b.names = b.count == 0 ? NULL : arena_alloc(p->arena, b.count * sizeof *b.names);
  if (b.count > 0 && b.names == NULL) return oom(p);
  for (size_t i = 0; i < b.count; i++) {
    b.names[i] = token_text(p, &p->toks[start + i]);
    if (b.names[i] == NULL) return 0;
  }
  if (!expect(p, TOK_EQUALS, "'='")) return 0;
  const Term *body = parse_term(p);
  d->body = body == NULL ? NULL : wrap(p, TERM_LAM, &b, body);
  return d->body != NULL;
}

static int parse_def(Parser *p, Decl *d) {
  d->kind = DECL_DEF;
  d->is_rec = eat(p, TOK_KW_REC);
  TokKind next = peek_at(p, 1)->kind;
  int clause = at(p, TOK_IDENT) && (next == TOK_EQUALS || next == TOK_IDENT);
  if (clause) return no_signature_fail(p, cur(p));
  return parse_signature(p, d, "a definition name") && parse_clause(p, d);
}

/* A constructor result is the data type applied to its parameters in
   order, then to one term for each index. INDICES gets the index terms. */
static int ctor_result_ok(const Decl *d, const Term *t, const Term **indices) {
  const Term *head = t;
  for (size_t i = d->index_count; i > 0; i--) {
    if (head->kind != TERM_APP) return 0;
    indices[i - 1] = head->right;
    head = head->left;
  }
  for (size_t i = d->param_count; i > 0; i--) {
    int ok = head->kind == TERM_APP && head->right->kind == TERM_VAR && strcmp(head->right->name, d->params[i - 1].name) == 0;
    if (!ok) return 0;
    head = head->left;
  }
  return head->kind == TERM_VAR && strcmp(head->name, d->name) == 0;
}

/* A path constructor result `Path X s t` (Q-S2b-4 (a)): the name Path
   applied to three terms. check_pctor checks X, s and t. */
static int path_result(const Term *t) {
  return t->kind == TERM_APP && t->left->kind == TERM_APP && t->left->left->kind == TERM_APP &&
         t->left->left->left->kind == TERM_VAR && strcmp(t->left->left->left->name, "Path") == 0;
}

/* FOUND is the source text of the constructor type. */
static int ctor_fail(Parser *p, const Ctor *ctor, size_t type_start, const char *expected, const char *why) {
  const Token *first = &p->toks[type_start];
  const Token *last = &p->toks[p->pos - 1];
  Span span = {p->source, ctor->line, ctor->col, last->line, last->end_col};
  size_t len = (size_t)(last->text + last->len - first->text);
  char found[256];
  snprintf(found, sizeof found, "%.*s", (int)(len < 255 ? len : 255), first->text);
  return diag_fail_at(p->diag, "PARSE_CTOR_RESULT", p->def, span, expected, found, NULL,
                      "%s:%d:%d: the constructor %s %s", p->source, ctor->line, ctor->col, ctor->name, why);
}

/* `(x : A) → .. → F p1 .. pn` gives the fields x ... */
static int split_ctor(Parser *p, const Decl *d, Ctor *ctor, const Term *type, size_t type_start) {
  size_t count = 0;
  const Term *t = type;
  while (t->kind == TERM_PI && t->name != NULL) {
    count++;
    t = t->right;
  }
  if (t->kind == TERM_PI) return ctor_fail(p, ctor, type_start, "(name : A) →", "has a field without a name");
  char result[256];
  size_t used = (size_t)snprintf(result, sizeof result, "%s", d->name);
  for (size_t i = 0; i < d->param_count && used < sizeof result; i++) {
    used += (size_t)snprintf(result + used, sizeof result - used, " %s", d->params[i].name);
  }
  for (size_t i = 0; i < d->index_count && used < sizeof result; i++) {
    used += (size_t)snprintf(result + used, sizeof result - used, " i%zu", i + 1);
  }
  char why[320];
  snprintf(why, sizeof why, "must end in %s", result);
  const Term **indices = d->index_count == 0 ? NULL : arena_alloc(p->arena, d->index_count * sizeof *indices);
  if (d->index_count > 0 && indices == NULL) return oom(p);
  ctor->result = d->index_count == 0 && path_result(t) ? t : NULL;
  if (ctor->result == NULL && !ctor_result_ok(d, t, indices)) return ctor_fail(p, ctor, type_start, result, why);
  ctor->indices = indices;
  Field *fields = count == 0 ? NULL : arena_alloc(p->arena, count * sizeof *fields);
  if (count > 0 && fields == NULL) return oom(p);
  const Term *f = type;
  for (size_t i = 0; i < count; i++) {
    fields[i].name = f->name;
    fields[i].type = f->left;
    fields[i].line = f->line;
    fields[i].col = f->col;
    f = f->right;
  }
  ctor->fields = fields;
  ctor->field_count = count;
  return 1;
}

static int parse_ctor(Parser *p, const Decl *d, Ctor *ctor, int column) {
  ctor->line = cur(p)->line;
  ctor->col = cur(p)->col;
  ctor->name = take_name(p, "a constructor name");
  if (ctor->name == NULL || !expect(p, TOK_COLON, "':'")) return 0;
  size_t type_start = p->pos;
  p->stop = column;
  const Term *type = parse_term(p);
  p->stop = 1;
  return type != NULL && split_ctor(p, d, ctor, type, type_start);
}

static int at_ctor(const Parser *p, int column) {
  return cur(p)->kind != TOK_EOF && first_on_line(p) && cur(p)->col == column;
}

/* `A1 → .. → Am → Type`: each Ai is an index type. A `Type` with a level
   after it is a universe index, not the end of the head. */
static int parse_head(Parser *p, Decl *d) {
  const Term **items = NULL;
  size_t count = 0;
  size_t cap = 0;
  while (!at(p, TOK_KW_TYPE) || peek_at(p, 1)->kind == TOK_NAT) {
    items = grow(p, items, count, &cap, sizeof *items);
    if (items == NULL) return 0;
    items[count] = parse_app(p);
    if (items[count] == NULL || !expect(p, TOK_ARROW, "'→'")) return 0;
    count++;
  }
  d->indices = items;
  d->index_count = count;
  return expect(p, TOK_KW_TYPE, "'Type'");
}

/* `data` or `codata`: the keyword gives the kind. */
static int parse_data(Parser *p, Decl *d) {
  d->kind = at(p, TOK_KW_CODATA) ? DECL_COFAMILY : DECL_FAMILY;
  bump(p);
  d->name = take_name(p, d->kind == DECL_COFAMILY ? "a codata type name" : "a data type name");
  p->stop = 1;
  if (d->name == NULL) return 0;
  p->def = d->name;
  if (!parse_fields(p, &d->params, &d->param_count)) return 0;
  if (!expect(p, TOK_COLON, "':'") || !parse_head(p, d) || !expect(p, TOK_KW_WHERE, "'where'")) return 0;
  if (!at(p, TOK_IDENT) || !first_on_line(p)) return fail_at(p, cur(p), "a constructor on a new indented line");
  int column = cur(p)->col;
  Ctor *ctors = NULL;
  size_t count = 0;
  size_t cap = 0;
  do {
    ctors = grow(p, ctors, count, &cap, sizeof *ctors);
    if (ctors == NULL) return 0;
    Ctor *ctor = &ctors[count];
    count++;
    if (!parse_ctor(p, d, ctor, column)) return 0;
  } while (at_ctor(p, column));
  d->ctors = ctors;
  d->ctor_count = count;
  return 1;
}

/* The forms (Q32 (a)). The model is the evolve-lang 9595979 `protocol`
   form. */
static int take_form_name(Parser *p, FormName *out, const char *what) {
  out->line = cur(p)->line;
  out->col = cur(p)->col;
  out->nat = 0;
  out->text = take_name(p, what);
  return out->text != NULL;
}

/* A new name at the end of NAMES. */
static FormName *next_name(Parser *p, FormName **names, size_t *count, size_t *cap) {
  *names = grow(p, *names, *count, cap, sizeof **names);
  if (*names == NULL) return NULL;
  FormName *n = &(*names)[*count];
  (*count)++;
  return n;
}

/* The head `KEYWORD NAME` of a form. */
static int parse_form_head(Parser *p, Decl *d, DeclKind kind, const char *what) {
  d->kind = kind;
  bump(p);
  d->name = take_name(p, what);
  p->stop = 1;
  if (d->name == NULL) return 0;
  p->def = d->name;
  return 1;
}

/* `replicas A B ..`: one replica name at least. */
static int parse_replicas(Parser *p, Decl *d) {
  d->kind = DECL_REPLICAS;
  bump(p);
  p->stop = 1;
  FormName *names = NULL;
  size_t count = 0;
  size_t cap = 0;
  do {
    FormName *n = next_name(p, &names, &count, &cap);
    if (n == NULL || !take_form_name(p, n, "a replica name")) return 0;
  } while (at(p, TOK_IDENT));
  d->names = names;
  d->name_count = count;
  return 1;
}

/* A member of `atoms`: a numeral when NUMERIC (the first member is a
   numeral), else a name. */
static int take_member(Parser *p, FormName *out, int numeric, int first) {
  out->line = cur(p)->line;
  out->col = cur(p)->col;
  out->text = NULL;
  out->nat = cur(p)->nat;
  if (numeric) return eat(p, TOK_NAT) ? 1 : fail_at(p, cur(p), "a Nat literal (the first member is a numeral)");
  if (!at(p, TOK_IDENT))
    return fail_at(p, cur(p), first ? "a member name or a Nat literal" : "a member name (the first member is a name)");
  out->text = take_name(p, "a member name");
  return out->text != NULL;
}

/* `atoms NAME := m ∣ ..`: all members are names or all are numerals
   (Q34 (a)). */
static int parse_atoms(Parser *p, Decl *d) {
  if (!parse_form_head(p, d, DECL_ATOMS, "an atoms name") || !expect(p, TOK_DEFINE, "':='")) return 0;
  int numeric = at(p, TOK_NAT);
  FormName *names = NULL;
  size_t count = 0;
  size_t cap = 0;
  do {
    FormName *n = next_name(p, &names, &count, &cap);
    if (n == NULL || !take_member(p, n, numeric, count == 1)) return 0;
  } while (eat(p, TOK_BAR));
  d->names = names;
  d->name_count = count;
  return 1;
}

/* `state NAME : T`, then `(init n)` or nothing. */
static int parse_state(Parser *p, FormItem *it) {
  if (!take_form_name(p, &it->name, "a state name") || !expect(p, TOK_COLON, "':'")) return 0;
  it->type = parse_term(p);
  if (it->type == NULL || !at_init(p)) return it->type != NULL;
  bump(p);
  bump(p);
  if (!at(p, TOK_NAT)) return fail_at(p, cur(p), "a Nat literal");
  it->has_init = 1;
  it->init = cur(p)->nat;
  bump(p);
  return expect(p, TOK_RPAREN, "')'");
}

/* One op step: `x ← read P`, `x ← stock P` or a write term. `read` and
   `stock` are names, not words. */
static int parse_step(Parser *p, OpStep *s) {
  s->line = cur(p)->line;
  s->col = cur(p)->col;
  if (!at(p, TOK_IDENT) || peek_at(p, 1)->kind != TOK_LARROW) {
    s->write = parse_term(p);
    return s->write != NULL;
  }
  s->name = take_name(p, "a name");
  bump(p);
  const Token *word = cur(p);
  const char *text = take_name(p, "'read' or 'stock'");
  if (text == NULL) return 0;
  if (strcmp(text, "read") != 0 && strcmp(text, "stock") != 0) return fail_at(p, word, "'read' or 'stock'");
  s->stock = strcmp(text, "stock") == 0;
  return take_form_name(p, &s->part, "a state name");
}

/* `op NAME (x : A) .. := STEP; ..; WRITE`: each step is a read or one part
   operation, and the last step is a write (Q35 (a), Q41 (a)). */
static int parse_op(Parser *p, FormItem *it) {
  if (!take_form_name(p, &it->name, "an operation name") || !parse_fields(p, &it->params, &it->param_count)
      || !expect(p, TOK_DEFINE, "a binder group or ':='"))
    return 0;
  OpStep *steps = NULL;
  size_t count = 0;
  size_t cap = 0;
  OpStep last;
  int more = 0;
  do {
    memset(&last, 0, sizeof last);
    if (!parse_step(p, &last)) return 0;
    more = eat(p, TOK_SEMI);
    steps = more ? grow(p, steps, count, &cap, sizeof *steps) : steps;
    if (more && steps == NULL) return 0;
    if (more) steps[count++] = last;
  } while (more);
  if (last.write == NULL) return fail_at(p, cur(p), "';' and a write step");
  it->steps = steps;
  it->step_count = count;
  it->body = last.write;
  return 1;
}

static int parse_invariant(Parser *p, FormItem *it) {
  it->body = parse_term(p);
  return it->body != NULL;
}

/* `allocate NAME by escrow (A ↦ n, ..)`. */
static int parse_allocate(Parser *p, FormItem *it) {
  if (!take_form_name(p, &it->name, "a state name") || !expect(p, TOK_KW_BY, "'by'")
      || !expect(p, TOK_KW_ESCROW, "'escrow'") || !expect(p, TOK_LPAREN, "'('"))
    return 0;
  FormName *shares = NULL;
  size_t count = 0;
  size_t cap = 0;
  do {
    FormName *s = next_name(p, &shares, &count, &cap);
    if (s == NULL || !take_form_name(p, s, "a replica name") || !expect(p, TOK_MAPSTO, "'↦'")) return 0;
    if (!at(p, TOK_NAT)) return fail_at(p, cur(p), "a Nat literal");
    s->nat = cur(p)->nat;
    bump(p);
  } while (eat(p, TOK_COMMA));
  it->shares = shares;
  it->share_count = count;
  return expect(p, TOK_RPAREN, "',' or ')'");
}

/* The item words and their parsers. */
static const struct {
  TokKind word;
  ItemKind kind;
  int (*parse)(Parser *p, FormItem *it);
} ITEM_WORDS[] = {
  {TOK_KW_STATE, ITEM_STATE, parse_state},
  {TOK_KW_OP, ITEM_OP, parse_op},
  {TOK_KW_INVARIANT, ITEM_INVARIANT, parse_invariant},
  {TOK_KW_ALLOCATE, ITEM_ALLOCATE, parse_allocate},
};

#define ITEM_WORD_COUNT (sizeof ITEM_WORDS / sizeof ITEM_WORDS[0])

/* One item line at COLUMN: the item word gives the kind. */
static int parse_item(Parser *p, FormItem *it, int column) {
  size_t i = 0;
  while (i < ITEM_WORD_COUNT && ITEM_WORDS[i].word != cur(p)->kind) i++;
  if (i == ITEM_WORD_COUNT) return fail_at(p, cur(p), "'state', 'op', 'invariant' or 'allocate'");
  memset(it, 0, sizeof *it);
  it->kind = ITEM_WORDS[i].kind;
  it->line = cur(p)->line;
  it->col = cur(p)->col;
  bump(p);
  p->stop = column;
  int ok = ITEM_WORDS[i].parse(p, it);
  p->stop = 1;
  return ok;
}

/* `replicated NAME`, then one item a line at one column (the evolve-lang
   `protocol` layout). */
static int parse_replicated(Parser *p, Decl *d) {
  if (!parse_form_head(p, d, DECL_REPLICATED, "a replicated name")) return 0;
  if (kind_of(p) == TOK_EOF || !first_on_line(p)) return fail_at(p, cur(p), "an item on a new indented line");
  int column = cur(p)->col;
  FormItem *items = NULL;
  size_t count = 0;
  size_t cap = 0;
  do {
    items = grow(p, items, count, &cap, sizeof *items);
    if (items == NULL || !parse_item(p, &items[count], column)) return 0;
    count++;
  } while (at_ctor(p, column));
  d->items = items;
  d->item_count = count;
  return 1;
}

static int parse_decl_body(Parser *p, Decl *d) {
  switch (cur(p)->kind) {
    case TOK_KW_POSTULATE: return parse_postulate(p, d);
    case TOK_KW_DATA:
    case TOK_KW_CODATA: return parse_data(p, d);
    case TOK_KW_REC:
    case TOK_IDENT: return parse_def(p, d);
    case TOK_KW_REPLICAS: return parse_replicas(p, d);
    case TOK_KW_ATOMS: return parse_atoms(p, d);
    case TOK_KW_REPLICATED: return parse_replicated(p, d);
    case TOK_BAR:
    case TOK_KW_STATE:
    case TOK_KW_OP:
    case TOK_KW_INVARIANT:
    case TOK_KW_ALLOCATE:
    case TOK_KW_BY:
    case TOK_KW_ESCROW:
    case TOK_KW_INIT:
    case TOK_EOF:
    case TOK_NAT:
    case TOK_LPAREN:
    case TOK_RPAREN:
    case TOK_COLON:
    case TOK_DEFINE:
    case TOK_EQUALS:
    case TOK_ARROW:
    case TOK_FATARROW:
    case TOK_TIMES:
    case TOK_LAMBDA:
    case TOK_KW_FUN:
    case TOK_KW_WHERE:
    case TOK_KW_TYPE:
    case TOK_KW_SIGMA:
    case TOK_TILDE:
    case TOK_MEET:
    case TOK_JOIN:
    case TOK_LBRACK:
    case TOK_RBRACK:
    case TOK_COMMA:
    case TOK_MAPSTO:
    case TOK_LARROW:
    case TOK_SEMI:
      break;
  }
  return fail_at(p, cur(p), "a signature, 'rec', 'postulate', 'data', 'codata', 'replicas', 'atoms' or 'replicated'");
}

/* The head token is read with stop 0; the parts after it set stop 1. */
static int parse_decl(Parser *p, Decl *d) {
  const Token *tok = cur(p);
  p->def = NULL;
  p->stop = 0;
  d->origin = p->origin;
  d->line = tok->line;
  d->col = tok->col;
  if (tok->col != 1) return fail_at(p, tok, "a declaration at column 1");
  if (!parse_decl_body(p, d)) return 0;
  const Token *last = &p->toks[p->pos - 1];
  d->end_line = last->line;
  d->end_col = last->end_col;
  if (!at(p, TOK_EOF)) return fail_at(p, cur(p), "a new line after the declaration");
  p->stop = 0;
  return 1;
}

int parse_source(Arena *arena, const char *source, Origin origin, const TokenList *toks, DeclList *list, Diag *diag) {
  Parser p = {
    .toks = toks->items, .count = toks->count, .pos = 0, .arena = arena, .diag = diag,
    .source = source, .def = NULL, .origin = origin, .depth = 0, .stop = 0,
  };
  Decl *items = list->items;
  size_t count = list->count;
  size_t cap = list->cap;
  int ok = 1;
  /* After a failure LIST keeps the complete declarations (the progress
     counts of `check --json`). */
  while (ok && !at(&p, TOK_EOF)) {
    Decl *grown = grow(&p, items, count, &cap, sizeof *items);
    ok = grown != NULL && parse_decl(&p, &grown[count]);
    items = grown != NULL ? grown : items;
    count += (size_t)ok;
  }
  list->items = items;
  list->count = count;
  list->cap = cap;
  return ok;
}
