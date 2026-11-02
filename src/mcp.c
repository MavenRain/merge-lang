#define _POSIX_C_SOURCE 200809L
/* The MCP stdio server of `langc mcp` (mcp.h; S8 design sec 2; port of
   evolve-lang 8c5d8e1 src/mcp.c): the framing, the lifecycle (Q85) and the
   tools check, fmt, sim and explain-code (Q77 (a), Q78 (a)). */
#include "mcp.h"

#include <stdlib.h>
#include <string.h>

#include "front/base.h"
#include "front/jsonin.h"

/* The arena of one request line. arena_init is lazy, so the limit costs
   nothing up front. */
#define MCP_ARENA_LIMIT_BYTES ((size_t)1 << 30)

/* JSON-RPC 2.0 error codes. */
enum {
  RPC_PARSE = -32700,
  RPC_BAD_REQUEST = -32600,
  RPC_UNKNOWN_METHOD = -32601,
  RPC_BAD_PARAMS = -32602,
  RPC_INTERNAL = -32603
};

/* The tools/list result (design sec 2). */
static const char MCP_TOOLS[] =
  "{\"tools\":["
  "{\"name\":\"check\",\"description\":\"Checks the merge-lang program at path. The text is the langc check --json report document.\","
  "\"inputSchema\":{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"}},\"required\":[\"path\"]}},"
  "{\"name\":\"fmt\",\"description\":\"Gives the canonical text of the merge-lang program at path, as langc fmt.\","
  "\"inputSchema\":{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"}},\"required\":[\"path\"]}},"
  "{\"name\":\"sim\",\"description\":\"Runs the bounded schedule search of the merge-lang program at path, as langc sim.\","
  "\"inputSchema\":{\"type\":\"object\",\"properties\":{\"path\":{\"type\":\"string\"},\"replicas\":{\"type\":\"integer\"},"
  "\"prefix\":{\"type\":\"integer\"},\"ops\":{\"type\":\"integer\"}},\"required\":[\"path\"]}},"
  "{\"name\":\"explain-code\",\"description\":\"Gives the row of the diagnostic code in docs/DIAGNOSTICS.md, as langc explain-code.\","
  "\"inputSchema\":{\"type\":\"object\",\"properties\":{\"code\":{\"type\":\"string\"}},\"required\":[\"code\"]}}"
  "]}";

/* Writes the LEN bytes of TEXT as a JSON string by the rule of put_string
   in front/json.c: `"` and `\` give \" and \\, a byte below 0x20 and each
   byte of an invalid UTF-8 sequence give \u00xx, valid UTF-8 stays raw. */
static void put_string(const char *text, size_t len, FILE *out) {
  fputc('"', out);
  for (size_t at = 0; at < len;) {
    unsigned char c = (unsigned char)text[at];
    size_t width = json_utf8_size(text + at, len - at);
    if (c == '"' || c == '\\') fprintf(out, "\\%c", c);
    else if (c < 0x20 || width == 0) fprintf(out, "\\u%04x", (unsigned)c);
    else fwrite(text + at, 1, width, out);
    at += width > 0 ? width : 1;
  }
  fputc('"', out);
}

/* Writes the id of a request: a string, a number (its source text) or
   null. */
static void put_id(const JsonValue *id, FILE *out) {
  if (id != NULL && id->kind == JSON_STRING) {
    put_string(id->text, id->len, out);
    return;
  }
  fputs(id != NULL && id->kind == JSON_NUMBER ? id->text : "null", out);
}

static void rpc_head(const JsonValue *id, FILE *out) {
  fputs("{\"jsonrpc\":\"2.0\",\"id\":", out);
  put_id(id, out);
}

static void rpc_error(const JsonValue *id, int code, const char *message, FILE *out) {
  rpc_head(id, out);
  fprintf(out, ",\"error\":{\"code\":%d,\"message\":", code);
  put_string(message, strlen(message), out);
  fputs("}}\n", out);
}

/* `initialize` always gives MCP_PROTOCOL_VERSION, whatever revision the
   client asks for (Q85). */
static void mcp_initialize(const JsonValue *id, const JsonValue *params, McpRunTool run_tool, FILE *out) {
  (void)run_tool;
  if (json_get_string(json_member(params, "protocolVersion")) == NULL) {
    rpc_error(id, RPC_BAD_PARAMS, "Invalid params", out);
    return;
  }
  rpc_head(id, out);
  fputs(",\"result\":{\"protocolVersion\":\"" MCP_PROTOCOL_VERSION "\",\"capabilities\":{\"tools\":{}},"
        "\"serverInfo\":{\"name\":\"merge-lang\",\"version\":\"S8\"}}}\n",
        out);
}

static void mcp_ping(const JsonValue *id, const JsonValue *params, McpRunTool run_tool, FILE *out) {
  (void)params;
  (void)run_tool;
  rpc_head(id, out);
  fputs(",\"result\":{}}\n", out);
}

static void mcp_tools_list(const JsonValue *id, const JsonValue *params, McpRunTool run_tool, FILE *out) {
  (void)params;
  (void)run_tool;
  rpc_head(id, out);
  fputs(",\"result\":", out);
  fputs(MCP_TOOLS, out);
  fputs("}\n", out);
}

/* JSON Schema integers are numeric values, including 1.0 and 1e0. Check
   the decimal scale exactly before conversion, so rounding and underflow
   cannot turn a fractional bound into an integer. VALUE is parsed JSON. */
static int integer_number(const JsonValue *value) {
  if (value == NULL || value->kind != JSON_NUMBER) return 0;
  const char *at = value->text + (value->text[0] == '-');
  long fraction = 0, zeros = 0, exponent = 0;
  int point = 0, nonzero = 0;
  for (; *at != '\0' && *at != 'e' && *at != 'E'; at++) {
    if (*at == '.') {
      point = 1;
    } else {
      fraction += point;
      zeros = *at == '0' ? zeros + 1 : 0;
      nonzero |= *at != '0';
    }
  }
  if (!nonzero) return 1;
  if (*at != '\0') {
    at++;
    int negative = *at == '-';
    at += *at == '-' || *at == '+';
    /* A scale beyond the whole request length has a known ordering
       against both FRACTION and ZEROS. Saturate before long overflows. */
    for (; *at != '\0'; at++)
      if (exponent <= (long)MCP_LINE_MAX_BYTES) exponent = exponent * 10 + (*at - '0');
    if (negative) exponent = -exponent;
  }
  return exponent >= fraction - zeros;
}

/* Runs the tool NAME with ARGS. Returns the exit code of the verb, or -1
   for an unknown tool, a missing argument or a non-integer sim bound. */
static int call_tool(const char *name, const JsonValue *args, McpRunTool run_tool, FILE *out, FILE *err) {
  static const char *const BOUND_KEYS[MCP_BOUND_COUNT] = {"replicas", "prefix", "ops"};
  const char *path = json_get_string(json_member(args, "path"));
  const char *code = json_get_string(json_member(args, "code"));
  const char *bounds[MCP_BOUND_COUNT];
  char decimal[MCP_BOUND_COUNT][sizeof "4294967295"];
  int bounds_ok = 1;
  for (size_t k = 0; k < MCP_BOUND_COUNT; k++) {
    const JsonValue *v = json_member(args, BOUND_KEYS[k]);
    int integer = integer_number(v);
    bounds_ok &= v == NULL || integer;
    bounds[k] = v != NULL && v->kind == JSON_NUMBER ? v->text : NULL;
    if (integer) {
      double n = strtod(v->text, NULL);
      /* Every uint32 integer is exact in double. Preserve other integer
         text for the CLI's range diagnostic instead of narrowing it. */
      if (n >= 0 && n <= UINT32_MAX) {
        snprintf(decimal[k], sizeof decimal[k], "%u", (unsigned)n);
        bounds[k] = decimal[k];
      }
    }
  }
  if (strcmp(name, "explain-code") == 0) return code != NULL ? run_tool(MCP_TOOL_EXPLAIN_CODE, code, NULL, out, err) : -1;
  if (path == NULL) return -1;
  if (strcmp(name, "check") == 0) return run_tool(MCP_TOOL_CHECK, path, NULL, out, err);
  if (strcmp(name, "fmt") == 0) return run_tool(MCP_TOOL_FMT, path, NULL, out, err);
  if (strcmp(name, "sim") == 0) return bounds_ok ? run_tool(MCP_TOOL_SIM, path, bounds, out, err) : -1;
  return -1;
}

/* `tools/call`: item 1 is the CLI stdout of the verb, item 2 is the CLI
   stderr when it is not empty, and isError is exit != 0 (design sec 2). */
static void mcp_call(const JsonValue *id, const JsonValue *params, McpRunTool run_tool, FILE *out) {
  const char *name = json_get_string(json_member(params, "name"));
  const JsonValue *args = json_member(params, "arguments");
  if (name == NULL || (args != NULL && args->kind != JSON_OBJECT)) {
    rpc_error(id, RPC_BAD_PARAMS, "Invalid params", out);
    return;
  }
  char *text = NULL;
  size_t len = 0;
  char *err_text = NULL;
  size_t err_len = 0;
  FILE *capture = open_memstream(&text, &len);
  FILE *err_capture = open_memstream(&err_text, &err_len);
  int opened = capture != NULL && err_capture != NULL;
  int status = opened ? call_tool(name, args, run_tool, capture, err_capture) : 0;
  /* Close both captures, also when the first close fails. */
  int closed = capture == NULL || fclose(capture) == 0;
  closed = (err_capture == NULL || fclose(err_capture) == 0) && closed;
  if (!opened || !closed) {
    rpc_error(id, RPC_INTERNAL, "Internal error", out);
  } else if (status < 0) {
    rpc_error(id, RPC_BAD_PARAMS, "Invalid params", out);
  } else {
    rpc_head(id, out);
    fputs(",\"result\":{\"content\":[{\"type\":\"text\",\"text\":", out);
    put_string(text != NULL ? text : "", text != NULL ? len : 0, out);
    if (err_len > 0) {
      fputs("},{\"type\":\"text\",\"text\":", out);
      put_string(err_text, err_len, out);
    }
    fprintf(out, "}],\"isError\":%s}}\n", status != 0 ? "true" : "false");
  }
  free(text);
  free(err_text);
}

typedef void (*McpMethod)(const JsonValue *id, const JsonValue *params, McpRunTool run_tool, FILE *out);

static const struct {
  const char *name;
  McpMethod run;
} MCP_METHODS[] = {
  {"initialize", mcp_initialize},
  {"ping", mcp_ping},
  {"tools/list", mcp_tools_list},
  {"tools/call", mcp_call},
};

/* An id is absent, a string, a number or null. */
static int id_ok(const JsonValue *id) {
  return id == NULL || id->kind == JSON_NUMBER || id->kind == JSON_NULL || id->kind == JSON_STRING;
}

/* Answers the message in the LEN bytes of LINE on OUT. A notification (a
   valid request without an id) gets no reply. Any other method gives
   -32601 (Q85). */
static void mcp_message(Arena *arena, const char *line, size_t len, McpRunTool run_tool, FILE *out) {
  JsonValue msg;
  Diag diag;
  diag_init(&diag);
  if (!json_parse(arena, line, len, &msg, &diag)) {
    rpc_error(NULL, RPC_PARSE, "Parse error", out);
    return;
  }
  const JsonValue *id = json_member(&msg, "id");
  const JsonValue *params = json_member(&msg, "params");
  const char *version = json_get_string(json_member(&msg, "jsonrpc"));
  const JsonValue *method = json_member(&msg, "method");
  int valid = version != NULL && strcmp(version, "2.0") == 0 && method != NULL && method->kind == JSON_STRING &&
              id_ok(id) && (params == NULL || params->kind == JSON_OBJECT || params->kind == JSON_ARRAY);
  if (!valid) {
    rpc_error(id_ok(id) ? id : NULL, RPC_BAD_REQUEST, "Invalid Request", out);
    return;
  }
  if (id == NULL) return;
  for (size_t i = 0; i < sizeof MCP_METHODS / sizeof MCP_METHODS[0]; i++) {
    if (strlen(MCP_METHODS[i].name) == method->len && memcmp(MCP_METHODS[i].name, method->text, method->len) == 0) {
      MCP_METHODS[i].run(id, params, run_tool, out);
      return;
    }
  }
  rpc_error(id, RPC_UNKNOWN_METHOD, "Method not found", out);
}

/* Reads one line of IN without its newline into LINE (MCP_LINE_MAX_BYTES
   bytes) and sets *LEN. Returns 0 at EOF before the first byte, 1 for a
   line, or -1 for a longer line: the rest of it up to the newline is read
   and dropped. */
static int read_line(FILE *in, char *line, size_t *len) {
  int c = getc(in);
  if (c == EOF) return 0;
  size_t n = 0;
  int over = 0;
  for (; c != EOF && c != '\n'; c = getc(in)) {
    over |= n == MCP_LINE_MAX_BYTES;
    if (!over) line[n++] = (char)c;
  }
  *len = n;
  return over ? -1 : 1;
}

int mcp_serve(FILE *in, FILE *out, McpRunTool run_tool) {
  char *line = malloc(MCP_LINE_MAX_BYTES);
  if (line == NULL) return 2;
  size_t len = 0;
  int got;
  int ok = 1;
  while (ok && (got = read_line(in, line, &len)) != 0) {
    len -= got > 0 && len > 0 && line[len - 1] == '\r';
    Arena arena;
    arena_init(&arena, MCP_ARENA_LIMIT_BYTES);
    if (got < 0) {
      rpc_error(NULL, RPC_PARSE, "Parse error", out);
    } else if (len > 0) {
      mcp_message(&arena, line, len, run_tool, out);
    }
    arena_release(&arena);
    ok = fflush(out) == 0 && !ferror(out);
  }
  free(line);
  return ok && !ferror(in) ? 0 : 2;
}
