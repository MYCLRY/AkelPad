extrn StackSaveSysParam: proc
extrn AsmCallbackHelper: proc

SYSPARAMSTACK struct
  first       QWORD  ?
  last        QWORD  ?
  nElements   QWORD  ?
SYSPARAMSTACK ends

SYSPARAMITEM struct
  next        QWORD  ?
  prev        QWORD  ?
  dwType      QWORD  ?
  nValue      QWORD  ?
SYSPARAMITEM ends

.data

.data?
;AsmCallSysFunc variables
hCurStack QWORD ?
hSaveStack QWORD ?
lpProcedure QWORD ?

;AsmCallbackHelper variables
lpnFirstArg QWORD ?

.code

AsmCallSysFunc proc
  mov hCurStack, rcx   ; SYSPARAMSTACK *hCurStack
  mov hSaveStack, rdx  ; SYSPARAMSTACK *hSaveStack
  mov lpProcedure, r8  ; FARPROC lpProcedure

  ;Prolog (LocalSize=hCurStack->nElements * sizeof QWORD * 2 + 20h)
  push rbp
  mov rbp, rsp
  mov r10, hCurStack
  mov r10, (SYSPARAMSTACK ptr [r10]).nElements
  imul r10, r10, sizeof QWORD
  add r10, r10
  add r10, 20h
  sub rsp, r10

  ;Push parameters
  mov r10, hCurStack
  mov r10, (SYSPARAMSTACK ptr [r10]).last
  mov r11, rsp

  @loop:
  cmp r10, 0
  je @endloop
  mov rax, (SYSPARAMITEM ptr [r10]).nValue
  mov [r11], rax

  ;Next parameter
  mov r10, (SYSPARAMITEM ptr [r10]).prev
  add r11, sizeof QWORD
  jmp @loop
  @endloop:

  ;Move arguments from hCurStack to hSaveStack stack to support recursive calls
  sub rsp, 28h
  mov rcx, hCurStack
  mov rdx, hSaveStack
  call StackSaveSysParam
  add rsp, 28h

  ;Move first four parameter to registers
  mov rcx, [rsp + 0]
  mov rdx, [rsp + 8h]
  mov r8, [rsp + 10h]
  mov r9, [rsp + 18h]

  ;Call system function
  call lpProcedure

  ;Epilog
  mov  rsp, rbp
  pop  rbp

  ret
AsmCallSysFunc endp

AsmCallback1Proc proc
  ;Prolog
  push  rbp
  mov   rbp, rsp
  sub rsp, 28h

  ;Get pointer to first element in stack
  mov   lpnFirstArg, rbp

  ;To get first argument pointer we skip first two elements in stack:
  ;1-saved ebp in prolog and 2-return address
  add   lpnFirstArg, 16

  ;Move registers to memory for use in AsmCallbackHelper
  mov r10, lpnFirstArg
  mov [r10 + 0], rcx
  mov [r10 + 8h], rdx
  mov [r10 + 10h], r8
  mov [r10 + 18h], r9

  ;Call script function
  sub rsp, 28h
  mov rcx, lpnFirstArg
  mov rdx, 1
  mov r8, 0
  call AsmCallbackHelper
  add rsp, 28h

  @end:

  ;Epilog
  mov  rsp, rbp
  pop  rbp

  ret
AsmCallback1Proc endp

AsmCallback2Proc proc
  ;Prolog
  push  rbp
  mov   rbp, rsp
  sub rsp, 28h

  ;Get pointer to first element in stack
  mov   lpnFirstArg, rbp

  ;To get first argument pointer we skip first two elements in stack:
  ;1-saved ebp in prolog and 2-return address
  add   lpnFirstArg, 16

  ;Move registers to memory for use in AsmCallbackHelper
  mov r10, lpnFirstArg
  mov [r10 + 0], rcx
  mov [r10 + 8h], rdx
  mov [r10 + 10h], r8
  mov [r10 + 18h], r9

  ;Call script function
  sub rsp, 28h
  mov rcx, lpnFirstArg
  mov rdx, 2
  mov r8, 0
  call AsmCallbackHelper
  add rsp, 28h

  @end:

  ;Epilog
  mov  rsp, rbp
  pop  rbp

  ret
AsmCallback2Proc endp

AsmCallback3Proc proc
  ;Prolog
  push  rbp
  mov   rbp, rsp
  sub rsp, 28h

  ;Get pointer to first element in stack
  mov   lpnFirstArg, rbp

  ;To get first argument pointer we skip first two elements in stack:
  ;1-saved ebp in prolog and 2-return address
  add   lpnFirstArg, 16

  ;Move registers to memory for use in AsmCallbackHelper
  mov r10, lpnFirstArg
  mov [r10 + 0], rcx
  mov [r10 + 8h], rdx
  mov [r10 + 10h], r8
  mov [r10 + 18h], r9

  ;Call script function
  sub rsp, 28h
  mov rcx, lpnFirstArg
  mov rdx, 3
  mov r8, 0
  call AsmCallbackHelper
  add rsp, 28h

  @end:

  ;Epilog
  mov  rsp, rbp
  pop  rbp

  ret
AsmCallback3Proc endp

AsmCallback4Proc proc
  ;Prolog
  push  rbp
  mov   rbp, rsp
  sub rsp, 28h

  ;Get pointer to first element in stack
  mov   lpnFirstArg, rbp

  ;To get first argument pointer we skip first two elements in stack:
  ;1-saved ebp in prolog and 2-return address
  add   lpnFirstArg, 16

  ;Move registers to memory for use in AsmCallbackHelper
  mov r10, lpnFirstArg
  mov [r10 + 0], rcx
  mov [r10 + 8h], rdx
  mov [r10 + 10h], r8
  mov [r10 + 18h], r9

  ;Call script function
  sub rsp, 28h
  mov rcx, lpnFirstArg
  mov rdx, 4
  mov r8, 0
  call AsmCallbackHelper
  add rsp, 28h

  @end:

  ;Epilog
  mov  rsp, rbp
  pop  rbp

  ret
AsmCallback4Proc endp

END
