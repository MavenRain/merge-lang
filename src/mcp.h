/* The MCP stdio server of `langc mcp` (S8 design sec 2; port of
   evolve-lang 8c5d8e1 src/mcp.h). This file calls no function of main.c:
   main.c gives the verbs as a callback, so build/mcp-test links without
   main.c. */
#ifndef LANG_MCP_H
#define LANG_MCP_H

#include <stdio.h>

/* The longest request line that the server reads, without the newline. A
   longer line gives -32700 with id null; the server drops the rest of the
   line and reads on. */
#define MCP_LINE_MAX_BYTES ((size_t)1 << 20)

/* The protocol revision that `initialize` always gives (Q85). */
#define MCP_PROTOCOL_VERSION "2025-11-25"

/* The tools of design sec 2 (Q77 (a), Q78 (a)). */
typedef enum {
  MCP_TOOL_CHECK,
  MCP_TOOL_FMT,
  MCP_TOOL_SIM,
  MCP_TOOL_EXPLAIN_CODE
} McpTool;

/* The sim bounds of a `sim` call, in the order of the CLI flags. */
enum { MCP_BOUND_REPLICAS, MCP_BOUND_PREFIX, MCP_BOUND_OPS, MCP_BOUND_COUNT };

/* Runs the verb of TOOL as the CLI does (`check --json ARG`, `fmt ARG`,
   `sim ARG [--replicas N] [--prefix P] [--ops M]`, `explain-code ARG`)
   with fresh Options and a fresh Arena. BOUNDS (sim only, else NULL) has
   MCP_BOUND_COUNT items: a JSON integer, normalized to decimal when it
   fits uint32, or NULL when the call does not give the bound. Writes the
   CLI stdout to OUT and the CLI stderr to ERR. Returns the CLI exit code. */
typedef int (*McpRunTool)(McpTool tool, const char *arg, const char *const *bounds, FILE *out, FILE *err);

/* Reads one JSON-RPC 2.0 message on each line of IN until EOF and writes
   each reply on one line of OUT. Returns 0, or 2 when IN cannot be read or
   OUT cannot be written. */
int mcp_serve(FILE *in, FILE *out, McpRunTool run_tool);

#endif
