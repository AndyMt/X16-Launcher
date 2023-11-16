.include "cbm_kernal.inc"

.import popa

MACPTR := $FF44
SET_CHARSET := $FF62

.export _vsync
.export _screen_set_charset
.export _cbm_k_macptr
.export _check_dos_error
.export _dosmsg

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

; check disk command channel (15) for error; Z=no error.
.proc _check_dos_error: near
                lda #$0f
                ldx #$08
                ldy #$0f
                jsr SETLFS
                lda #$00
                sta _dosmsg
                jsr SETNAM
                jsr OPEN
                ldx #$0f
                jsr CHKIN
                jsr GETIN
                cmp #'0'
                bne error
                ldx #$0f
                jsr CLOSE
                lda #$00
                jmp ret
error:          sta _dosmsg
                ldy #$01
errloop:        jsr GETIN
                beq @done
                cmp #$0d
                beq @done

                sta _dosmsg,y
                iny
                bne errloop
@done:          lda #$00
                sta _dosmsg,y
                ldx #$0f
                jsr CLOSE
;                jsr CLALL
                lda #$01
ret:            rts
.endproc


.segment "DATA"
rc2: .byte 0

.bss
    _dosmsg:         .res 80

.code