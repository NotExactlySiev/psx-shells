.set noreorder
.section .text

.macro sysc code, label
    .globl \label
    .type \label,@function
    \label:
        addiu      $a0, $zero, \code
        syscall    0
        jr         $ra
        nop
.endm

.macro kern table, code, label
    .globl \label
    .type \label,@function
    \label:
        addiu      $t2, $zero, \table
        jr         $t2
        addiu      $t1, $zero, \code
.endm

sysc 1, EnterCriticalSection
sysc 2, ExitCriticalSection
kern 0xA0, 0x3F, k_printf
kern 0xB0, 0x04, enable_timer_irq
kern 0xB0, 0x05, disable_timer_irq
kern 0xB0, 0x08, OpenEvent
kern 0xB0, 0x09, CloseEvent
kern 0xB0, 0x0C, EnableEvent
kern 0xB0, 0x0D, DisableEvent

.globl vec3_cross
.type vec3_cross,@function
vec3_cross:
    /* D x IR */
    /*170000Ch+sf*80000h*/
    /* activate gte */


    lw      $t0, 0x0($a1)
    lw      $t1, 0x4($a1)
    lw      $t2, 0x8($a1)
    /* load vector a */
    ctc2    $t0, $0
    ctc2    $t1, $2
    ctc2    $t2, $4

    lw      $t0, 0x0($a2)
    lw      $t1, 0x4($a2)
    lw      $t2, 0x8($a2)
    /* load vector b */
    mtc2    $t0, $9
    mtc2    $t1, $10
    mtc2    $t2, $11

    # .word 0x4BF0000C # invalid instruction
    c2      0x1F8000C
    mfc2    $t0, $9  
    mfc2    $t1, $10 
    mfc2    $t2, $11 
    sw      $t0, 0x0($a0)
    sw      $t1, 0x4($a0)
    jr      $ra
    sw      $t2, 0x8($a0)

.globl set_rotation_matrix
.type set_rotation_matrix,@function
set_rotation_matrix:
    mfc0    $t0, $12
    li      $t1, 0x40000000
    or      $t0, $t1
    mtc0    $t0, $12

    lw      $t0, 0x0($a0)   # 11 12
    lw      $t1, 0x4($a0)   # 13 21
    lw      $t2, 0x8($a0)   # 22 23
    lw      $t3, 0xc($a0)   # 31 32

    ctc2    $t0, $0
    ctc2    $t1, $1
    ctc2    $t2, $2
    ctc2    $t3, $3

    lw      $t0, 0x10($a0)
    nop
    ctc2    $t0, $4
    jr      $ra
    nop

.globl set_translation_vector
.type set_translation_vector,@function
set_translation_vector:
    lw      $t0, 0x0($a0)
    lw      $t1, 0x4($a0)
    lw      $t2, 0x8($a0)
    ctc2    $t0, $5
    ctc2    $t1, $6
    jr      $ra
    ctc2    $t2, $7


.globl transform_perspective
.type transform_perspective,@function
transform_perspective:
    lhu     $t0, 0x0($a1)
    lhu     $t1, 0x4($a1)
    lhu     $t2, 0x8($a1)
    sll     $t1, 16
    or      $t1, $t0
    mtc2    $t1, $0    # VXY0
    mtc2    $t2, $1    # VZ0

    ctc2    $0, $24    # OFX
    ctc2    $0, $25    # OFY
    sra     $a2, 4 
    ctc2    $a2, $26 # H
    nop

    c2      0x0180001
    nop
    mfc2    $t0, $14    # SXY2
    mfc2    $t2, $19    # SZ3
    nop
    add     $t1, $t0, $zero
    sll     $t0, 16     # sign extend X
    sra     $t0, 16
    sw      $t0, 0x0($a0)
    sra     $t1, 16     # sign extend Y
    sw      $t1, 0x4($a0)
    sw      $t2, 0x8($a0)

    jr  $ra
    nop


.globl transform_orthographic
.type transform_orthographic,@function
transform_orthographic:
    lhu     $t0, 0x0($a1)
    lhu     $t1, 0x4($a1)
    lhu     $t2, 0x8($a1)
    sll     $t1, 16
    or      $t1, $t0
    mtc2    $t1, $0
    mtc2    $t2, $1
    
    c2      0x0480012
    nop
    nop
    nop
    nop
    mfc2    $t0, $9
    mfc2    $t1, $10
    mfc2    $t2, $11
    nop
    sw      $t0, 0x0($a0)
    sw      $t1, 0x4($a0)
    sw      $t2, 0x8($a0)

    jr  $ra
    nop
