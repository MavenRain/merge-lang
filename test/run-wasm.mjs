// Runs entries of a langc Wasm module and prints each result as `langc eval`
// does: a number, or `trap` when the module traps.
// Usage: node test/run-wasm.mjs FILE NAME [ARGS...]
//        node test/run-wasm.mjs FILE -   (one `NAME [ARGS...]` line on stdin per call)
// On stdin, an argument `@k` is the result of call line k (from 1), and the
// pseudo call `words P L` prints the L memory words at P + 8, one a line. The
// pseudo call `store P W...` writes the words W... at P + 8 and prints P.
import { readFileSync } from "node:fs";

const [file, name, ...args] = process.argv.slice(2);
if (file === undefined || name === undefined) {
  process.stderr.write("usage: node test/run-wasm.mjs FILE NAME [ARGS...] | FILE -\n");
  process.exit(2);
}
const instance = new WebAssembly.Instance(new WebAssembly.Module(readFileSync(file)), {});

const call = ([entry, ...words]) => {
  const fn = instance.exports[entry];
  if (typeof fn !== "function") throw new Error(`no exported entry '${entry}'`);
  try {
    return BigInt.asUintN(64, fn(...words.map(BigInt))).toString();
  } catch (e) {
    if (e instanceof WebAssembly.RuntimeError) return "trap";
    throw e;
  }
};

const words = ([p, l]) => {
  const view = new DataView(instance.exports.memory.buffer);
  return Array.from({ length: Number(l) }, (_, i) => view.getBigUint64(Number(p) + 8 + 8 * i, true).toString());
};

const store = ([p, ...ws]) => {
  const view = new DataView(instance.exports.memory.buffer);
  ws.forEach((w, i) => view.setBigUint64(Number(p) + 8 + 8 * i, BigInt(w), true));
  return [p];
};

const run = (c) => (c[0] === "words" ? words(c.slice(1)) : c[0] === "store" ? store(c.slice(1)) : [call(c)]);

const calls = name === "-"
  ? readFileSync(0, "utf8").split("\n").filter((line) => line !== "").map((line) => line.split(" "))
  : [[name, ...args]];
const { lines } = calls.reduce((acc, c) => {
  const out = run(c.map((word) => (word.startsWith("@") ? acc.results[Number(word.slice(1)) - 1] : word)));
  acc.results.push(out[0]);
  acc.lines.push(...out);
  return acc;
}, { results: [], lines: [] });
process.stdout.write(lines.map((line) => line + "\n").join(""));
