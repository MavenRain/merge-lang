#!/bin/sh
# Regression checks for the staged compiler review.
set -eu
tmp=$(mktemp -d "${TMPDIR:-/tmp}/lang-review.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM

cat >"$tmp/capture.lang" <<'EOF'
keep : ℕ → ℕ → ℕ
keep x y = x

value : Unit → ℕ → ℕ → ℕ
value u y = keep y
EOF
normal=$(build/langc eval "$tmp/capture.lang" value)
printf 'value : Unit → ℕ → ℕ → ℕ\nvalue = %s\n\nanswer : ℕ\nanswer = value unit 4 9\n' "$normal" >"$tmp/roundtrip.lang"
test "$(build/langc eval "$tmp/roundtrip.lang" answer)" = 4

# Reducing a closure can expose a constructor with the binder's old name.
cat >"$tmp/constructor.lang" <<'EOF'
constant : Unit → Verdict
constant u = independent

value : Unit → Verdict
value independent = constant independent
EOF
normal=$(build/langc eval "$tmp/constructor.lang" value)
printf 'value : Unit → Verdict\nvalue = %s\n\nanswer : ℕ\nanswer = fold 1 2 3 (value unit)\n' "$normal" >"$tmp/roundtrip.lang"
test "$(build/langc eval "$tmp/roundtrip.lang" answer)" = 1

# A generated eta binder must also print as a source identifier.
printf 'value : Unit → ℕ → ℕ → ℕ\nvalue u = natAdd\n' >"$tmp/eta.lang"
normal=$(build/langc eval "$tmp/eta.lang" value)
printf 'value : Unit → ℕ → ℕ → ℕ\nvalue = %s\n\nanswer : ℕ\nanswer = value unit 4 9\n' "$normal" >"$tmp/roundtrip.lang"
test "$(build/langc eval "$tmp/roundtrip.lang" answer)" = 13

# More than 64 live binders used to read past the printer's name table.
awk 'BEGIN {
  printf "value : "; for (i = 0; i < 70; i++) printf "Unit → ";
  printf "Unit\nvalue"; for (i = 0; i < 70; i++) printf " u%d", i;
  print " = u69"
}' >"$tmp/binders.lang"
awk 'BEGIN {
  for (i = 0; i < 70; i++) printf "λ u%d → ", i;
  print "u69"
}' >"$tmp/want"
build/langc eval "$tmp/binders.lang" value >"$tmp/got"
cmp "$tmp/want" "$tmp/got"

# bind keeps an absent value and can change the success type.
cat >"$tmp/option.lang" <<'EOF'
input : Option ℕ
input = none

output : Option Bool
output = bind input (λ x → some true)

present : Option ℕ
present = some 7

mapped : Option Bool
mapped = bind present (λ x → some true)
EOF
test "$(build/langc eval "$tmp/option.lang" output)" = 'none'
test "$(build/langc eval "$tmp/option.lang" mapped)" = 'some true'
# A truncated normal form must not be reported as successful evaluation.
printf 'value : Log\nvalue = fold (λ (s : Log) → logCons 0 s) logNil 10000\n' >"$tmp/large.lang"
status=0
build/langc eval "$tmp/large.lang" value >"$tmp/got" 2>"$tmp/err" || status=$?
test "$status" = 1
test ! -s "$tmp/got"
case "$(cat "$tmp/err")" in
  'langc: EVAL_PRINT:'*) ;;
  *) exit 1 ;;
esac
# A motive can be a named family or a family parameter, at any universe.
cat >"$tmp/motive.lang" <<'EOF'
M : ℕ → Type 0
M k = ℕ

value : ℕ → ℕ
value n = fold M (λ (k : ℕ) (acc : ℕ) → natAdd acc k) 0 n

induct : (P : ℕ → Type 0) → ((k : ℕ) → P k → P (natAdd k 1)) → P 0 → (n : ℕ) → P n
induct P step base n = fold P step base n

types : ℕ → Type 1
types k = Type 0

atType : ℕ → Type 0
atType n = fold types (λ (k : ℕ) (a : Type 0) → a) ℕ n
EOF
test "$(build/langc eval "$tmp/motive.lang" value 5)" = 10
build/langc check "$tmp/motive.lang" >"$tmp/got"

# Neutral dependent eliminators must retain their motive when printed.
cat >"$tmp/elim.lang" <<'EOF'
boolProof : (b : Bool) → Path Bool b b
boolProof b = elim (λ (x : Bool) → Path Bool x x) refl refl b

optionProof : (o : Option ℕ) → Path (Option ℕ) o o
optionProof o = elim (λ (x : Option ℕ) → Path (Option ℕ) x x) refl (λ (a : ℕ) → refl) o

boolCompute : (b : Bool) → Path ℕ (elim (λ (x : Bool) → ℕ) 4 9 b) (flagIf b 4 9)
boolCompute b = refl

optionCompute : (o : Option ℕ) → Path ℕ (elim (λ (x : Option ℕ) → ℕ) 7 (λ (a : ℕ) → a) o) (option 7 (λ (a : ℕ) → a) o)
optionCompute o = refl

underBinder : Unit → (b : Bool) → Path Bool b b
underBinder elim b = boolProof b
EOF
normal=$(build/langc eval "$tmp/elim.lang" boolProof)
printf 'value : (b : Bool) → Path Bool b b\nvalue = %s\n' "$normal" >"$tmp/roundtrip.lang"
build/langc check "$tmp/roundtrip.lang" >"$tmp/got"
test "$(build/langc eval "$tmp/roundtrip.lang" value)" = "$normal"
normal=$(build/langc eval "$tmp/elim.lang" optionProof)
printf 'value : (o : Option ℕ) → Path (Option ℕ) o o\nvalue = %s\n' "$normal" >"$tmp/roundtrip.lang"
build/langc check "$tmp/roundtrip.lang" >"$tmp/got"
test "$(build/langc eval "$tmp/roundtrip.lang" value)" = "$normal"
normal=$(build/langc eval "$tmp/elim.lang" underBinder)
printf 'value : Unit → (b : Bool) → Path Bool b b\nvalue = %s\n' "$normal" >"$tmp/roundtrip.lang"
build/langc check "$tmp/roundtrip.lang" >"$tmp/got"
test "$(build/langc eval "$tmp/roundtrip.lang" value)" = "$normal"
echo 'review regressions: passed'
