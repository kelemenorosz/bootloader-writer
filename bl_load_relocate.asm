; BPB + EBR (90 bytes) + 0x7C00
[ORG 0x7C5A]

; Set DS = 0
xor ax, ax
mov ds, ax

; Set SS = 0x8000 and SP = 0x0000. Top of the stack will be SS:0xFFFE (on first push instruction)
mov bx, 0x8000
mov ax, 0x0000
cli
mov ss, bx
mov sp, ax
sti

mov bx, something_to_print ; Address of thing to print
mov [print_address], bx

mov bx, [print_address]
mov cx, [bx]
push word cx
call print_word
add sp, 0x02

; Relocate

push word dx
call print_byte ; Print drive number
add sp, 0x02

mov ah, 0x41 ; Check for 13h extension support
mov bx, 0x55AA
int 0x13

jc no_extension
mov bx, 0x0000
jmp extension_continue
no_extension:
mov bx, 0x00FF
extension_continue:

push word bx
call print_byte ; Print extension available
add sp, 0x02

mov ah, 0x42 ; Read from disk
mov si, disk_address_packet ; Set DAP struct location. DS = 0
int 0x13

push ax ; Save return code

jc read_error
mov bx, 0x0000
jmp read_continue
read_error:
mov bx, 0x00FF
read_continue:

push word bx
call print_byte ; Print error
add sp, 0x02

pop ax ; Get return code
shr ax, 0x08
push word ax
call print_byte ; Print error code
add sp, 0x02

push word 0x0058
call print_4bits ; Print character X
add sp, 0x02

mov ax, print_word
push word ax
call ax ; Print CS
add sp, 0x02

push word 0x0058
call print_4bits ; Print character X
add sp, 0x02

mov bx, jump_label

mov ax, print_word
push word bx
call ax ; Print jump label here
add sp, 0x02

push word 0x0058
call print_4bits ; Print character X
add sp, 0x02

mov bx, jump_label
sub bx, 0x200

mov ax, print_word
push word bx
call ax ; Print jump label at copy
add sp, 0x02 

; Jump to other part
; Print Y
; Hang

mov ax, jump_label
sub ax, 0x200
jmp ax

; Operating in address space 0x7A00 - 0x7BFF
jump_label:

mov ax, print_4bits
sub ax, 0x200
push word 0x005A
call ax ; Print Y in moved address space
add sp, 0x02

mov ah, 0x42 ; Read from disk
mov si, disk_address_packet_second_stage ; Set DAP struct location. DS = 0
sub si, 0x200
int 0x13

mov ax, 0x7C00
jmp ax

mov ax, hang
jmp ax

; // Print word
print_word:
push bp
mov bp, sp

mov ax, [bp + 0x04]
shr ax, 0x08
push word ax
call print_byte
add sp, 0x02

mov ax, [bp + 0x04]
push word ax
call print_byte
add sp, 0x02

pop bp
ret

; // Print byte

print_byte:
push bp
mov bp, sp

mov ax, [bp + 0x4]
shr al, 0x4
and al, 0x0F
cmp al, 0x09
jg letter
add al, 0x30
jmp print
letter:
add al, 0x37
print:
push word ax
call print_4bits
add sp, 0x2

mov ax, [bp + 0x4]
and al, 0x0F
cmp al, 0x09
jg letter_1
add al, 0x30
jmp print_1
letter_1:
add al, 0x37
print_1:
push word ax
call print_4bits
add sp, 0x2

pop bp
ret

; // Print 4 bits

print_4bits:
push bp
mov bp, sp

mov cx, [bp + 0x4]
mov ah, 0x0e    ; Write teletype
mov al, cl      ; Character
mov bl, 0x00    ; Page
int 0x10        ; BIOS interrupt video

pop bp
ret

hang:
jmp hang

disk_address_packet_second_stage:
db 0x10     ; Set to 10h
db 0x00     ; Set to 0
dw 0x0008   ; Number of sectors to read
dw 0x7C00   ; Dest. offset
dw 0x0000   ; Dest. segment
dd 0x1      ; Low 4 bytes of LBA start
dd 0x0      ; High 4 bytes of LBA start

disk_address_packet:
db 0x10     ; Set to 10h
db 0x00     ; Set to 0
dw 0x0001   ; Number of sectors to read
dw 0x7A00   ; Dest. offset
dw 0x0000   ; Dest. segment
dd 0x0      ; Low 4 bytes of LBA start
dd 0x0      ; High 4 bytes of LBA start

print_address dw 0x0000
something_to_print dw 0x414E 
times 350 - ($ - $$) db 0