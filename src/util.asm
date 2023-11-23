.include "cbm_kernal.inc"
.include "cx16.inc"

.import popa

MACPTR := $FF44
SET_CHARSET := $FF62
THUMBNAIL_BUFFER_ADDR := $4000
THUMBNAIL_BASE_ADDR   := $8000

VERA_addr_low     = $9F20
VERA_addr_high    = $9F21
VERA_addr_bank    = $9F22
VERA_data0        = $9F23
VERA_data1        = $9F24
VERA_ctrl         = $9F25

.export _vsync
.export _screen_set_charset
.export _cbm_k_macptr
.export _check_dos_error
.export _check_file_exists
.export _split_thumbnail
.export _screen_put_char

.export _dosmsg
.export _s_addr
.export _s_offset

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

.proc _screen_put_char: near
        jsr     CHROUT
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

.proc _check_file_exists: near
                lda #$02
                ldx #$08
                ldy #$02
                jsr SETLFS


                jsr SETNAM

                jsr OPEN
                ldx #$0f
                jsr CLOSE
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
                ;jsr CLALL
                lda #$01
ret:            rts
.endproc

; split thumbnail bitmap into sprite data
.proc _split_thumbnail
    ldx #0
    stx row

    lda #<THUMBNAIL_BASE_ADDR ; base address for sprites
    sta addr
    lda #>THUMBNAIL_BASE_ADDR 
    sta addr+1

    lda #<THUMBNAIL_BUFFER_ADDR ; base address for bitmap data
    sta start
    lda #>THUMBNAIL_BUFFER_ADDR
    sta start+1

row_loop:       

    ; copy start offset 
    lda start
    sta offset
    lda start+1
    sta offset+1

    ; select ADDRESEL 1 = offset
    lda VERA_ctrl
    ora #$01
    sta VERA_ctrl
    ; store vram address (1=offset)
    lda #$10
    sta VERA_addr_bank      ; always 0 + inc 1
    lda offset+1
    sta VERA_addr_high
    lda offset
    sta VERA_addr_low

    ; select ADDRESEL 0 = addr in sprite data
    lda VERA_ctrl
    and #$fe
    sta VERA_ctrl
    ; store addr (0=addr)
    lda #$10
    sta VERA_addr_bank      ; always 0 + inc 1
    lda addr+1
    sta VERA_addr_high
    lda addr
    sta VERA_addr_low

    ldy #0
y_loop:                     ; using y register for counting exclusively

    ; select ADDRESEL 1 = offset
    lda VERA_ctrl
    ora #$01
    sta VERA_ctrl
    ; store addr (1=offset)
    lda offset+1
    sta VERA_addr_high
    lda offset
    sta VERA_addr_low

    ; select ADDRESEL 0 = addr
    lda VERA_ctrl
    and #$fe
    sta VERA_ctrl
    ; store addr (0=addr)
    lda addr+1
    sta VERA_addr_high
    lda addr
    sta VERA_addr_low

    .repeat 32
    lda VERA_data1
    sta VERA_data0
    sta VERA_data0
    .endrepeat

    ;increment addr by 64
    clc
    lda addr
    adc #64
    sta addr
    lda addr+1              ; carry over to high byte
    adc #0
    sta addr+1

    ;increment offset by 128
    clc
    lda offset
    adc #128
    sta offset
    lda offset+1            ; carry over to high byte
    adc #0
    sta offset+1

    iny
    cpy #96
    bcs exit_y_loop
    jmp y_loop
exit_y_loop:

    clc
    lda start
    adc #32
    sta start

    ldx row
    inx
    stx row
    cpx #4;
    bcs exit_row_loop
    jmp row_loop
exit_row_loop:
    rts
.endproc


.zeropage
_s_addr:
addr:       .word 0
start:      .word 0
_s_offset:
offset:     .word 0

.segment "DATA"
rc2:        .byte 0
row:        .byte 0
; _s_addr:
; addr:       .word 0
; _s_offset:
; start:      .word 0     ; start of bitmap data offsets
; offset:     .word 0

.bss
    _dosmsg:         .res 80

.code