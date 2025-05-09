; RUN: not --crash llc -mtriple=mipsel -mcpu=mips32r2 -mattr=+soft-float \
; RUN:         -O0 -fast-isel-abort=3 -relocation-model=pic < %s

; Test that FastISel aborts instead of trying to lower arguments for soft-float.

define void @__signbit(double %__x) {
entry:
  %__x.addr = alloca double, align 8
  store double %__x, ptr %__x.addr, align 8
  ret void
}
