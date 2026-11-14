IFDEF WIN64

INCLUDE Callx64.inc

PUBLIC Callx64

PUBLIC Callx64iii
PUBLIC Callx64iif
PUBLIC Callx64ifi
PUBLIC Callx64fii
PUBLIC Callx64iff
PUBLIC Callx64fif
PUBLIC Callx64ffi
PUBLIC Callx64fff

PUBLIC ReturnFloatx64

.CODE


    Callx64 PROC func:PTR DWORD
    ;, arg1:PTR DWORD, arg2:PTR DWORD, arg3:PTR DWORD, f1: REAL4, f2: REAL4, f3: REAL4 

; the parameters are rcx,rdx,r8 and r9,
; which are void *this_ptr,void* func,void* arg1,void* arg2

; The following is example code for a typical prolog:

	;	mov   [RSP + 8], RCX
		push R15
		push R14
		push R13

		sub   RSP, 68h
		lea   R13, 128[RSP]
		
; if that worked ok, now do the rest:
		;sub		rsp, 68h
		mov			func, rcx
		mov			rcx, rdx
		mov			rdx, r8
		mov			r8, r9
		movss	xmm0,xmm1
		movss	xmm1,xmm2
		movss	xmm2,xmm3
		call	func

		add		rsp, 68h
		
		pop   R13
		pop   R14
		pop   R15
		
		ret
    Callx64 ENDP


    Callx64iii PROC func:PTR DWORD
		push	R15
		push	R14
		push	R13

		sub   RSP, 68h
		lea   R13, 128[RSP]

		mov		func, rcx
		mov		rcx, rdx
		mov		rdx, r8
		mov		r8, r9

		call	func

		add		rsp, 68h

		pop		R13
		pop		R14
		pop		R15
		ret
    Callx64iii ENDP

    Callx64iif PROC func:PTR DWORD
		push	R15
		push	R14
		push	R13

		sub   RSP, 68h
		lea   R13, 128[RSP]
	
		mov		func, rcx
		mov		rcx, rdx
		mov		rdx, r8
		mov		r8, r9
		movss	xmm0,xmm1
		movss	xmm1,xmm2
		movss	xmm2,xmm3
		call	func

		add		rsp, 68h

		pop		R13
		pop		R14
		pop		R15
		ret
    Callx64iif ENDP

    Callx64ifi PROC func:PTR DWORD
	;	mov   [RSP + 8], RCX
		push	R15
		push	R14
		push	R13

		sub   RSP, 68h
		lea   R13, 128[RSP]

		mov		func, rcx
		mov		rcx, rdx
		mov		rdx, r8
		mov		r8, r9
		
		movss	xmm0,xmm1
		movss	xmm1,xmm2
		movss	xmm2,xmm3
		
		call	func

		add		rsp, 68h


; epilog
		pop		R13
		pop		R14
		pop		R15
		ret
    Callx64ifi ENDP

    Callx64fii PROC func:PTR DWORD
		push	R15
		push	R14
		push	R13

		sub   RSP, 68h
		lea   R13, 128[RSP]

		mov		func, rcx
		mov		rcx, rdx
		mov		rdx, r8
		mov		r8, r9
		movss	xmm0,xmm1
		movss	xmm1,xmm2
		movss	xmm2,xmm3
		call	func

		add		rsp, 68h

		pop		R13
		pop		R14
		pop		R15
		ret
    Callx64fii ENDP

    Callx64iff PROC func:PTR DWORD
		push R15
		push R14
		push R13

		sub   RSP, 68h
		lea   R13, 128[RSP]
		
		mov			func, rcx
		mov			rcx, rdx
		mov			rdx, r8
		mov			r8, r9
		movss	xmm0,xmm1
		movss	xmm1,xmm2
		movss	xmm2,xmm3
		call		func
		pop   R13
		pop   R14
		pop   R15
		ret
    Callx64iff ENDP

    Callx64fif PROC func:PTR DWORD
		push R15
		push R14
		push R13

		sub   RSP, 68h
		lea   R13, 128[RSP]
		
		mov		func, rcx
		mov		rcx, rdx
		mov		rdx, r8
		mov		r8, r9
		movss	xmm0,xmm1
		movss	xmm1,xmm2
		movss	xmm2,xmm3
		call	func
		pop   R13
		pop   R14
		pop   R15
		ret
    Callx64fif ENDP

    Callx64ffi PROC func:PTR DWORD
		push R15
		push R14
		push R13

		sub   RSP, 68h
		lea   R13, 128[RSP]
		
		mov			func, rcx
		mov			rcx, rdx
		mov			rdx, r8
		mov			r8, r9
		movss	xmm0,xmm1
		movss	xmm1,xmm2
		movss	xmm2,xmm3
		call		func
		pop   R13
		pop   R14
		pop   R15
		ret
    Callx64ffi ENDP

    Callx64fff PROC func:PTR DWORD
		push	R15
		push	R14
		push	R13

		sub		RSP, 68h
		lea		R13, 128[RSP]
		
		mov		func, rcx
		mov		rcx, rdx
		mov		rdx, r8
		mov		r8, r9
		movss	xmm0,xmm1
		movss	xmm1,xmm2
		movss	xmm2,xmm3
		call	func
		pop		R13
		pop		R14
		pop		R15
		ret
    Callx64fff ENDP

	ReturnFloatx64 PROC addrs:PTR DWORD
		mov			r8,rcx
		movss		dword ptr [r8],xmm0
		ret
    ReturnFloatx64 ENDP

ENDIF
	
END

