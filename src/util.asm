.import popa

RDTIM := $FFDE
MACPTR := $FF44
SET_CHARSET := $FF62

.export _vsync
.export _screen_set_charset
.export _cbm_k_macptr

.code

.proc _vsync: near
        jsr     RDTIM
        sta     rc2
b1:     jsr     RDTIM
        cmp     rc2
        beq     b1
        rts
.endproc

.proc _screen_set_charset: near
        jsr     SET_CHARSET
        rts
.endproc

; C prototype:
; extern int16_t __fastcall__ cbm_k_macptr((uint8_t*) numbyte, char* buffer);

.proc _cbm_k_macptr: near
    phx
    tax
    jsr popa
    ply
    jsr MACPTR
    bcs macptr_unsupported
macptr_success:
    ; convert .XY returned by MACPTR into .AX for C
    txa
    phy
    plx
    rts
macptr_unsupported:
    ; return -1 for unsupported device
    ldx #$FF
    lda #$FF
    rts
.endproc

.segment "DATA"
rc2: .byte 0
