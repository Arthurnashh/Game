TITLE	PIIIcode

.686
.XMM
.MODEL FLAT
.LIST

if 0

_TEXT	SEGMENT PARA PUBLIC USE32 'CODE'
	ALIGN 16

ftuName equ ?DoSetFastTransform@Vector3P3@@QAEXABVMatrix4P3@@ABV1@@Z
	PUBLIC ftuName
ftuName	PROC NEAR

	mov edx, DWORD PTR [esp+8] 
	mov eax, DWORD PTR [esp+4] 

	movss xmm0, DWORD PTR [edx]

	movups xmm4,OWORD PTR [eax]
	movups xmm5,OWORD PTR [eax+16] 
	movups xmm6,OWORD PTR [eax+32] 
	movups xmm7,OWORD PTR [eax+48] 

	shufps xmm0, xmm0, 0
	movss xmm1, DWORD PTR [edx+4]
	mulps xmm0, xmm4
	shufps xmm1, xmm1, 0
	movss xmm3, DWORD PTR [edx+8]
	mulps xmm1, xmm5
	shufps xmm3, xmm3, 0
	addps xmm0, xmm1 
	mulps xmm3, xmm6
	addps xmm0, xmm3
	addps xmm0, xmm7
	movups OWORD PTR [ecx], xmm0 
	ret 8 

	ALIGN 16 

ftuName ENDP

_TEXT	ENDS

endif

_TEXT	SEGMENT PARA PUBLIC USE32 'CODE'
	ALIGN 16

ftName equ ?DoSetFastTransformA@Vector3P3@@QAEXABVMatrix4P3@@ABV1@@Z
	PUBLIC ftName
ftName PROC NEAR

	int 3 ; not implemented

	ret 8 
	ALIGN 16 

ftName ENDP

_TEXT	ENDS

if 0

_TEXT	SEGMENT PARA PUBLIC USE32 'CODE'

	ALIGN 16

transLoopName equ _TransformPoints

;extern "C" void TransformPoints
;(
;	Vector3P3 *dst, const Vector3P3 *src, int n,
;	const Matrix4 &posView
;);

	PUBLIC C transLoopName
transLoopName PROC NEAR

	src equ ebx
	dst equ edx
	posView equ eax
	n equ ecx

	mov posView,[esp+16]
	mov n,[esp+12]
	mov src,[esp+8]
	mov dst,[esp+4]
	
	movups xmm4,[posView]
	movups xmm5,[posView+16]
	movups xmm6,[posView+32]
	movups xmm7,[posView+48]

	jmp loopWhile
	transLoop:
		; perform transformation
		prefetchnta [src+64]

		movss xmm0, DWORD PTR [src]
		shufps xmm0, xmm0, 0
		movss xmm1, DWORD PTR [src+4]
		mulps xmm0, xmm4
		shufps xmm1, xmm1, 0
		movss xmm3, DWORD PTR [src+8]
		mulps xmm1, xmm5
		shufps xmm3, xmm3, 0
		addps xmm0, xmm1 
		mulps xmm3, xmm6

		movss xmm2, DWORD PTR [src+16]

		addps xmm0, xmm3

		addps xmm0, xmm7
		movntps OWORD PTR [dst], xmm0 

		shufps xmm2, xmm2, 0
		movss xmm1, DWORD PTR [src+20]
		mulps xmm2, xmm4
		shufps xmm1, xmm1, 0
		movss xmm3, DWORD PTR [src+24]
		mulps xmm1, xmm5
		shufps xmm3, xmm3, 0
		addps xmm2, xmm1 
		mulps xmm3, xmm6
		add src,32
		addps xmm2, xmm3
		add dst,32
		addps xmm2, xmm7

		movntps OWORD PTR [dst-16], xmm2

		; update data pointers
		loopWhile:
		sub n,2
	jge transLoop

	test n,1
	je noRest

		movss xmm0, DWORD PTR [src]
		shufps xmm0, xmm0, 0
		movss xmm1, DWORD PTR [src+4]
		mulps xmm0, xmm4
		shufps xmm1, xmm1, 0
		movss xmm3, DWORD PTR [src+8]
		mulps xmm1, xmm5
		shufps xmm3, xmm3, 0
		addps xmm0, xmm1 
		mulps xmm3, xmm6
		addps xmm0, xmm3
		addps xmm0, xmm7
		movntps OWORD PTR [dst], xmm0 

	noRest:

	ret
	ALIGN 16

transLoopName ENDP

_TEXT	ENDS

endif

_TEXT	SEGMENT PARA PUBLIC USE32 'CODE'
 
_MM_FLUSH_ZERO_MASK equ 08000h

	ALIGN 16 ;
	PUBLIC C _SetFlushToZero

_SetFlushToZero PROC NEAR

	sub esp,4

	stmxcsr [esp]
	mov eax,[esp]
	and eax,-1-_MM_FLUSH_ZERO_MASK
	mov [esp],eax
	ldmxcsr [esp]

	add esp,4
	ret

_SetFlushToZero ENDP


END

