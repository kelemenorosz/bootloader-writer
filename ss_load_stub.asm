[ORG 0x7C00]

push word 0x000D ; carriage return
call print_4bits
add sp, 0x02

push word 0x000A ; new line
call print_4bits
add sp, 0x02


mov ah, 0x0e         ; Write teletype
mov al, 0x4d         ; Character
mov bl, 0x00         ; Page
int 0x10             ; BIOS interrupt video

mov ah, 0x0e         ; Write teletype
mov al, 0x43         ; Character
mov bl, 0x00         ; Page
int 0x10             ; BIOS interrupt video

mov ah, 0x0e         ; Write teletype
mov al, 0x4b         ; Character
mov bl, 0x00         ; Page
int 0x10             ; BIOS interrupt video

push word 0x000D ; carriage return
call print_4bits
add sp, 0x02

push word 0x000A ; new line
call print_4bits
add sp, 0x02

; memory check

xor ebx, ebx
memory_check:
stc
mov eax, 0xe820
mov ebx, [continuation_value]
mov cx, 0x0000
mov es, cx
mov di, address_range_descriptor
mov ecx, 0x18
mov edx, 0x0534D4150
int 0x15

jc memory_int15_error

cmp ebx, 0x0
je memory_int15_error

mov [continuation_value], ebx

mov eax, [address_range_descriptor] ;; Print low bits addr.
push word ax
shr eax, 0x10
push word ax
call print_dword
add sp, 0x02

push word 0x0020 ; space
call print_4bits
add sp, 0x02

mov eax, [address_range_descriptor + 0x04] ;; Print high bits addr.
push word ax
shr eax, 0x10
push word ax
call print_dword
add sp, 0x02

push word 0x0020 ; space
call print_4bits
add sp, 0x02

mov eax, [address_range_descriptor + 0x08] ;; Print low bits length in bytes
push word ax
shr eax, 0x10
push word ax
call print_dword
add sp, 0x02

push word 0x0020 ; space
call print_4bits
add sp, 0x02

mov eax, [address_range_descriptor + 0x0C] ;; Print high bits length in bytes
push word ax
shr eax, 0x10
push word ax
call print_dword
add sp, 0x02

push word 0x0020 ; space
call print_4bits
add sp, 0x02

mov eax, [address_range_descriptor + 0x10] ;; Print type
push word ax
shr eax, 0x10
push word ax
call print_dword
add sp, 0x02

push word 0x000D ; carriage return
call print_4bits
add sp, 0x02

push word 0x000A ; new line
call print_4bits
add sp, 0x02

; Save memory map at 0x6000:0x0002

cld ; Clear DF - Increment DS:SI and ES:DI

; Set destination ES:DI = 0x6000:[address_range_descriptor_counter]
mov ax, 0x6000 
mov es, ax
mov ax, [address_range_descriptor_counter]
mov di, ax

mov ecx, 0x18 ; Set length of Address Range Descriptor

; Set source DS:SI = 0x0000:address_range_descriptor
mov ax, address_range_descriptor
mov si, ax
mov ax, 0x0000
mov ds, ax

rep movsb ; Move ECX bytes from DS:SI to ES:DI

; Reset DS
mov ax, 0x0
mov ds, ax

; Increment [address_range_descriptor_counter]
mov ax, [address_range_descriptor_counter]
add ax, 0x18
mov [address_range_descriptor_counter], ax

jmp memory_check

memory_int15_error:
push word 0xFFFF
call print_byte
add sp, 0x02

; Save memory map length at 0x6000:0x0000
mov ax, 0x6000
mov es, ax
mov ax, 0x0
mov di, ax
mov ax, [address_range_descriptor_counter]
mov es:di, ax

; A20 Line Enable

;;cli ;; DEBUG BREAK
;;hlt ;;

push word 0x000D ; carriage return
call print_4bits
add sp, 0x02

push word 0x000A ; new line
call print_4bits
add sp, 0x02

call test_a20 ; Test if A20 line is enabled

push word ax 
call print_word
add sp, 0x02

push word 0x000D ; carriage return
call print_4bits
add sp, 0x02

push word 0x000A ; new line
call print_4bits
add sp, 0x02

call test_a20_bios_support ; Check for A20 BIOS support  

push word bx
push word ax 
call print_word
add sp, 0x02

call print_word
add sp, 0x02

push word 0x000D ; carriage return
call print_4bits
add sp, 0x02

push word 0x000A ; new line
call print_4bits
add sp, 0x02

call test_a20_bios_status ; Check for A20 status via BIOS

push word bx
push word ax
call print_word
add sp, 0x02

call print_word
add sp, 0x02

push word 0x000D ; carriage return
call print_4bits
add sp, 0x02

push word 0x000A ; new line
call print_4bits
add sp, 0x02

call test_a20_bios_enable ; Enable A20 status via BIOS

push word ax
call print_word
add sp, 0x02

push word 0x000D ; carriage return
call print_4bits
add sp, 0x02

push word 0x000A ; new line
call print_4bits
add sp, 0x02

call test_a20 ; Test if A20 line is enabled

push word ax 
call print_word
add sp, 0x02

push word 0x000D ; carriage return
call print_4bits
add sp, 0x02

push word 0x000A ; new line
call print_4bits
add sp, 0x02

call test_a20_bios_status ; Check for A20 status via BIOS

push word bx
push word ax
call print_word
add sp, 0x02

call print_word
add sp, 0x02

call print_cr_nl ; New line

mov ax, 0x0000
mov fs, ax
mov ax, [fs:0x7A00]
push word ax
call print_word
add sp, 0x2 

push dword 0x00000000 ; High 4 bytes lba start
push dword 0x00000000 ; Low 4 bytes lba start
push word 0x5000     ; Dest. segment
push word 0x0000     ; Dest. offset
push word 0x0001     ; Nr. of sectors to read
call read_sector
add sp, 0xE

push word ax
call print_word
add sp, 0x2

mov ax, 0x5000
mov fs, ax
mov ax, [fs:0x0000]
push word ax
call print_word
add sp, 0x2 

call print_cr_nl

push word 0x5000 ; MBR address segment
push word 0x0000 ; MBR address offset
call parse_mbr
add sp, 0x4

push dword eax
call print_cr_nl
pop dword eax
mov [partition_lba_start], eax

push dword 0x00000000 ; High 4 bytes lba start
push dword eax       ; Low 4 bytes lba start
push word 0x5000     ; Dest. segment
push word 0x0200     ; Dest. offset
push word 0x0001     ; Nr. of sectors to read
call read_sector
add sp, 0xE

mov ax, parse_boot_record_print_string
push word ds        ; Set string address segment
push word ax        ; Set string address offset
push word 0x12      ; Set string length
call print_string   ; Print string 'PARSE_BOOT_RECORD:'
add sp, 0x6
call print_cr_nl    ; Print carriage return and newline

mov ax, parse_boot_record_return_s
push word 0x0000 ; Return structure segment
push word ax     ; Retrun structure offset
push word 0x5000 ; Boot record address segment
push word 0x0200 ; Boot record address offset
call parse_boot_record
add sp, 0x4

call print_cr_nl

mov ax, [parse_boot_record_return_s]
push word ax
call print_word
add sp, 0x2
call print_space
mov ax, [parse_boot_record_return_s + 0x2]
push word ax
call print_byte
add sp, 0x2
call print_space
mov eax, [parse_boot_record_return_s + 0x3]
push ax
shr ax, 0x10
push ax
call print_dword
add sp, 0x4
call print_space
mov eax, [parse_boot_record_return_s + 0x7]
push ax
shr ax, 0x10
push ax
call print_dword
add sp, 0x4

call print_cr_nl

mov eax, [partition_lba_start]
xor ebx, ebx
mov bx, [parse_boot_record_return_s] ; Reserved sectors
add eax, ebx
mov ebx, eax
mov eax, [parse_boot_record_return_s + 0x3] ; FAT size
xor ecx, ecx
mov cl, [parse_boot_record_return_s + 0x2] ; Number of FATs
mul ecx
add ebx, eax

push ebx

push bx
shr bx, 0x10
push bx
call print_dword
add sp, 0x4

pop ebx

call print_cr_nl

push dword 0x00000000 ; High 4 bytes lba start
push dword ebx       ; Low 4 bytes lba start
push word 0x5000     ; Dest. segment
push word 0x0400     ; Dest. offset
push word 0x0020     ; Nr. of sectors to read
call read_sector
add sp, 0xE

push word 0x5000
push word 0x0400
call read_directory
add sp, 0x4

mov [kernel_first_cluster], eax

push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_cr_nl

mov eax, [partition_lba_start]
xor ebx, ebx
mov bx, [parse_boot_record_return_s] ; Reserved sectors
add eax, ebx
mov ebx, eax
mov [fat_first_sector], ebx
mov eax, [parse_boot_record_return_s + 0x3] ; FAT size
xor ecx, ecx
mov cl, [parse_boot_record_return_s + 0x2] ; Number of FATs
mul ecx
add ebx, eax
mov [data_first_sector], ebx

mov ax, read_kernel_elf_print_string
push word ds        ; Set string address segment
push word ax        ; Set string address offset
push word 0x10      ; Set string length
call print_string   ; Print string 'READ_KERNEL_ELF:'
add sp, 0x6
call print_cr_nl    ; Print carriage return and newline

;; Read kernel ELF file from disk

push dword [data_first_sector]
push dword [kernel_first_cluster]
push dword [fat_first_sector]
call read_kernel_elf
add sp, 0xC

;cli ;;
;hlt ;; DEBUG BREAKPOINT

mov ax, 0x1000
mov fs, ax
mov eax, [fs:0x0000]
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_cr_nl

mov ax, parse_kernel_elf_print_string
push word ds        ; Set string address segment
push word ax        ; Set string address offset
push word 0x11      ; Set string length
call print_string   ; Print string 'PARSE_KERNEL_ELF:'
add sp, 0x6
call print_cr_nl    ; Print carriage return and newline

;; Parse kernel ELF file 

mov ax, parse_kernel_elf_s
push word 0x0
push word ax
call parse_kernel_elf       ; Parse kernel ELF
add sp, 0x4

mov ax, parse_kernel_elf_s_print_string
push word ds        ; Set string address segment
push word ax        ; Set string address offset
push word 0x1B      ; Set string length
call print_string   ; Print string 'PARSE_KERNEL_ELF_STRUCTURE:'
add sp, 0x6
call print_cr_nl    ; Print carriage return and newline

mov eax, [parse_kernel_elf_s]
push ax
shr eax, 0x10
push ax
call print_dword    ; Print kernel location relative to ELF start
add sp, 0x4

call print_space

mov eax, [parse_kernel_elf_s + 0x4]
push ax
shr eax, 0x10
push ax
call print_dword    ; Print kernel size
add sp, 0x4

call print_cr_nl

mov ax, 0x1000
mov gs, ax

mov ax, [parse_kernel_elf_s]
mov si, ax
push ax
call print_word
add sp, 0x2

call print_space

mov eax, [gs:si]
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_cr_nl

mov ax, 0x1000
mov gs, ax

mov ax, [parse_kernel_elf_s]
add ax, 0x1008
mov si, ax
push ax
call print_word
add sp, 0x2

call print_space

mov eax, [gs:si]
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_space

call print_cr_nl

; Clear DF
cld
; Set destination ES:DI = 0x2000:0x0000
mov ax, 0x2000
mov es, ax
mov ax, 0x0
mov di, ax
; Set length [parse_kernel_elf_s + 0x4]
mov ecx, [parse_kernel_elf_s + 0x4]
; Set source DS:SI = 0x1000:[parse_kernel_elf_s]
mov ax, [parse_kernel_elf_s]
mov si, ax
mov ax, 0x1000
mov ds, ax
rep movsb

mov ax, 0x0
mov ds, ax

mov ax, [parse_kernel_elf_s + 0x4]
sub di, ax

mov eax, [es:di] 
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_cr_nl

;; Disable interrupts; Load GDTR

cli                         ; Disable interrupts

xor eax, eax                ; Calculate GDT
xor ebx, ebx
mov ax, gdt_table
mov [gdtr + 0x2], eax
mov eax, gdt_table_end
mov ebx, gdt_table
sub eax, ebx
mov [gdtr], ax
lgdt [gdtr]                 ; Load GDTR

;; Enable protected mode

mov eax, cr0        
or eax, 0x1
mov cr0, eax

;; Far jump to kernel code

jmp dword 0x8:0x20000

;; EBX -> root cluster
;; Read first sector of root cluster
;; Find kernel.elf length, first cluster
;; Read FAT
;; Read kernel.elf
;; Read section from kernel.elf
;; Set up GDTR
;; Go to protected mode
;; Jump to kernel.elf :)

jmp hang

; ======================================================================================================================================

; Parse kernel.elf. Params: return struct. address offset; return struct. address segment
parse_kernel_elf:
push bp
mov bp, sp
sub sp, 0x8

mov ax, 0x1000
mov fs, ax
mov ax, 0x0000
mov si, ax

mov al, [fs:si] ; Magic number 
push word ax
call print_byte
add sp, 0x2

call print_space

mov ax, si ; ELF string
inc ax
push word fs ; Set string address segment
push word ax ; Set string address offset
push word 0x3 ; Set string length
call print_string
add sp, 0x6

call print_space

mov al, [fs:si + 0x4] ; 32/64 bits
push word ax
call print_byte
add sp, 0x2

call print_space

mov al, [fs:si + 0x5] ; Endianness
push word ax
call print_byte
add sp, 0x2

call print_space

mov al, [fs:si + 0x6] ; Header version
push word ax
call print_byte
add sp, 0x2

call print_space

mov al, [fs:si + 0x7] ; OS ABI
push word ax
call print_byte
add sp, 0x2

call print_space

mov ax, [fs:si + 0x10] ; Type  
push word ax
call print_word
add sp, 0x2

call print_space

mov ax, [fs:si + 0x12] ; Instruction set
push word ax
call print_word
add sp, 0x2

call print_space

mov eax, [fs:si + 0x14] ; ELF version
push word ax
shr eax, 0x10
push word ax
call print_dword
add sp, 0x4

call print_cr_nl

mov eax, [fs:si + 0x18] ; Program entry offset
push word ax
shr eax, 0x10
push word ax
call print_dword
add sp, 0x4

call print_space

mov eax, [fs:si + 0x1C] ; Program header table offset
mov [bp - 0x6], eax
push word ax
shr eax, 0x10
push word ax
call print_dword
add sp, 0x4

call print_space

mov eax, [fs:si + 0x20] ; Section header table offset
push word ax
shr eax, 0x10
push word ax
call print_dword
add sp, 0x4

call print_space

mov eax, [fs:si + 0x24] ; Flags
push word ax
shr eax, 0x10
push word ax
call print_dword
add sp, 0x4

call print_space

mov ax, [fs:si + 0x28] ; Header size
push word ax
call print_word
add sp, 0x2

call print_space

mov ax, [fs:si + 0x2A] ; Program header table entry size
mov [bp - 0x8], ax
push word ax
call print_word
add sp, 0x2

call print_space

mov ax, [fs:si + 0x2C] ; Program header table entry count
mov [bp - 0x2], ax
push word ax
call print_word
add sp, 0x2

call print_space

mov ax, [fs:si + 0x2E] ; Section header table entry size
push word ax
call print_word
add sp, 0x2

call print_space

mov ax, [fs:si + 0x30] ; Section header table entry count
push word ax
call print_word
add sp, 0x2

call print_space

mov ax, [fs:si + 0x32] ; Section index to section header string table
push word ax
call print_word
add sp, 0x2

call print_cr_nl

mov eax, [bp - 0x6]
mov si, ax

parse_kernel_elf_prog_header_entry:
mov ax, [bp - 0x2]
cmp ax, 0x0
je parse_kernel_elf_exit

mov eax, [fs:si] ; Type
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_space

mov di, [bp + 0x4]
mov gs, [bp + 0x6]
mov eax, [fs:si + 0x4] ; Segment data
mov [gs:di], eax
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_space

mov eax, [fs:si + 0x8] ; Virtual address
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_space

mov eax, [fs:si + 0xC] ; Physical address
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_space

mov di, [bp + 0x4]
mov gs, [bp + 0x6]
mov eax, [fs:si + 0x10] ; Segment size
mov [gs:di + 0x4], eax
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_space

mov eax, [fs:si + 0x14] ; Segment memory size
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_space

mov eax, [fs:si + 0x18] ; Flags
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_space

mov eax, [fs:si + 0x1C] ; Alignment
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_cr_nl

mov ax, [bp - 0x2]
dec ax
mov [bp - 0x2], ax
mov ax, [bp - 0x8]
add si, ax
jmp parse_kernel_elf_prog_header_entry

parse_kernel_elf_exit:

mov sp, bp
pop bp
ret

; Read kernel.elf. Params: FAT start sector; File start cluster; Data section start sector
read_kernel_elf:
push bp
mov bp, sp
sub sp, 0x2

mov eax, [bp + 0x4] ; FAT start sector
mov ebx, [bp + 0x8] ; File start cluster
mov ecx, [bp + 0xC] ; Data section start sector
mov word [bp - 0x2], 0x0 ; Cluster counter

read_kernel_elf_next_cluster:
; Where is file start cluster in the fat?

mov eax, [bp + 0x4]
mov ebx, [bp + 0x8]
shr ebx, 0xC
shl ebx, 0x5
add ebx, eax

mov eax, ebx
push ax
shr eax, 0x10
push ax
call print_dword    ;; Print the number of the first sector of 32-sector long segment of FAT
add sp, 0x4
call print_cr_nl

mov eax, [bp + 0x4]
mov ebx, [bp + 0x8]
shr ebx, 0xC
shl ebx, 0x5
add ebx, eax

; Read 32 sectors starting at EBX + FAT start sector
push dword 0x00000000 ; High 4 bytes lba start
push dword ebx        ; Low 4 bytes lba start
push word 0x5000      ; Dest. segment
push word 0x0000      ; Dest. offset
push word 0x0020      ; Nr. of sectors to read
call read_sector
add sp, 0xE

; Print out some stuff
mov eax, [bp + 0xC]
push ax
shr eax, 0x10
push ax
call print_dword    ;; Print the number of the first sector of partition data
add sp, 0x4

call print_cr_nl

mov eax, [bp + 0x8]
sub eax, 0x2
shl eax, 0x4
push ax
shr eax, 0x10
push ax
call print_dword    ;; Print the number of the first sector of the data cluster to be read relative to partition data start
add sp, 0x4

call print_cr_nl

; Read current cluster into memory
mov ebx, [bp + 0x8]
mov ecx, [bp + 0xC]
sub ebx, 0x2
shl ebx, 0x4
add ebx, ecx
push dword 0x00000000       ; High 4 bytes lba start
push dword ebx              ; Low 4 bytes lba start
push word 0x1000            ; Dest. segment
push word [bp - 0x2]        ; Dest. offset
push word 0x0010            ; Nr. of sectors to read
call read_sector
add sp, 0xE
add word [bp - 0x2], 0x2000

; Where is the first file cluster in the fat?
mov ebx, [bp + 0x8]
and ebx, 0xFFF
shl ebx, 0x2

mov eax, ebx
push ax
shr eax, 0x10
push ax
call print_dword    ;; Print the memory location of the cluster in the FAT relative to the 32 sector long section
add sp, 0x4
call print_cr_nl

mov ebx, [bp + 0x8]
and ebx, 0xFFF
shl ebx, 0x2

; Move cluster in fat to EAX
mov ax, 0x5000
mov fs, ax
mov eax, [fs:ebx]

; If bigger than 0x0FFFFFF8 no more clusters in the chain
; and eax, 0x0FFFFFFF
mov [bp + 0x8], eax

push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_cr_nl

mov eax, [bp + 0x8]
cmp eax, 0x0FFFFFF8
jl read_kernel_elf_next_cluster

read_kernel_elf_exit:

mov sp, bp
pop bp
ret

; String compare. Params:   first string address offset; first string address segment; 
;                           second string address offset; seconds string address segment; compare length
string_compare:
push bp
mov bp, sp
push si
push fs
push es
push di
push bx
push cx

mov si, [bp + 0x4]
mov fs, [bp + 0x6]
mov di, [bp + 0x8]
mov es, [bp + 0xA]
mov bx, [bp + 0xC]

string_compare_char:
cmp bx, 0x0
je string_compare_equal
mov byte cl, [fs:si]
mov byte ch, [es:di]
cmp cl, ch
jne string_compare_different
dec bx
inc si
inc di
jmp string_compare_char
string_compare_equal:
mov ax, 0x0
jmp string_compare_exit
string_compare_different:
mov ax, 0x1
string_compare_exit:

pop cx
pop bx
pop di
pop es
pop fs
pop si
mov sp, bp
pop bp
ret

; Read directory. Params: sector address offset; sector address segment
read_directory:
push bp
mov bp, sp

mov si, [bp + 0x4] ; Set sector offset
mov fs, [bp + 0x6] ; Set sector segment

mov dx, [bp + 0x4]
add dx, 0x4000

mov bx, 0x00

read_directory_entry:
mov ax, [fs:si + 0x0B]
cmp ax, 0x0
je read_directory_kernel_not_found ; Jump if dir entry offset 0x0B is 0x0 - no more entries

cmp ax, 0xF
je read_directory_entry_ext ; Jump if dir entry is extension

; Regular entry
mov cx, bx
mov bx, 0x0
cmp cx, 0x0
jne read_directory_entry_next ; Jump if entry has extension

mov byte cl, [fs:si]
cmp cl, 0xE5
je read_directory_entry_next

; Print filename
push word fs ; Set string address segment
push word si ; Set string address offset
push word 0x8 ; Set string length
call print_string
add sp, 0x6

call print_space

; Print file extension
mov cx, si
add cx, 0x8
push word fs ; Set string address segment
push word cx ; Set string address offset
push word 0x3 ; Set string length
call print_string
add sp, 0x6

call print_space

; First cluster
mov cx, [fs:si + 0x14]
shl ecx, 0x10
mov cx, [fs:si + 0x1A]
push cx
shr ecx, 0x10
push cx
call print_dword
add sp, 0x4

call print_space

; File size
mov ecx, [fs:si + 0x1C]
push cx
shr ecx, 0x10
push cx
call print_dword
add sp, 0x4

call print_space

push word 0x4 ; Set length
push word fs ; Set string address segment
push word si ; Set string address offset
push word ds ; Set cmp string address segment
push word kernel_name ; Set cmp string address offset
call string_compare
add sp, 0xA

push word ax
push word ax
call print_byte
add sp, 0x2

call print_space

mov cx, si
add cx, 0x8
push word 0x3 ; Set length
push word fs ; Set string address segment
push word cx ; Set string address offset
push word ds ; Set cmp string address segment
push word kernel_extension ; Set cmp string address offset
call string_compare
add sp, 0xA

push word ax
push word ax
call print_byte
add sp, 0x2

call print_cr_nl

pop dword ebx
mov ax, [fs:si + 0x14]
shl eax, 0x10
mov ax, [fs:si + 0x1A]
cmp ebx, 0x0
je read_directory_kernel_found

mov bx, 0x0
jmp read_directory_entry_next

; Extension entry
read_directory_entry_ext:
mov bx, 0x1

read_directory_entry_next:
add si, 0x20
cmp si, dx
jl read_directory_entry

read_directory_kernel_found:
jmp read_directory_exit

read_directory_kernel_not_found:
mov eax, 0x0

read_directory_exit:

mov sp, bp
pop bp
ret

; Parse boot record. Params: br address offset; br address segment; return struct offset; return struct segment
parse_boot_record:
push bp
mov bp, sp

mov si, [bp + 0x4] ; Set BR offset
mov fs, [bp + 0x6] ; Set BR segment

mov eax, [fs:si] ; Jump opcode
and eax, 0x00FFFFFF
push word ax
shr eax, 0x10
push word ax
call print_dword
add sp, 0x4

call print_space

mov bx, si
add bx, 0x3  ; OEM identifier
push word fs ; Set string address segment
push word bx ; Set string address offset
push word 0x8 ; Set string length
call print_string
add sp, 0x4
pop fs

call print_space

mov ax, [fs:si + 0xB] ; Bytes per sector
push word ax
call print_word
add sp, 0x2

call print_space

mov ax, [fs:si + 0xD] ; Sectors per cluster
push word ax
call print_byte
add sp, 0x2

call print_space

mov ax, [fs:si + 0xE] ; Reserved sectors
push word ax
call print_word
add sp, 0x2

call print_space

mov ax, [fs:si + 0x10] ; Number of File Allocation Tables (FAT)s
push word ax
call print_byte
add sp, 0x2 

call print_space

mov ax, [fs:si + 0x11] ; Root directory entries
push word ax
call print_word
add sp, 0x2

call print_space

mov ax, [fs:si + 0x13] ; Total sectors (small)
push word ax
call print_word
add sp, 0x2

call print_space

mov ax, [fs:si + 0x15] ; Media descriptor type
push word ax
call print_byte
add sp, 0x2

call print_space

mov ax, [fs:si + 0x16] ; Sectors per FAT (FAT12/FAT16 only)
push word ax
call print_word
add sp, 0x2

call print_space

mov ax, [fs:si + 0x18] ; Sectors per track
push word ax
call print_word
add sp, 0x2

call print_space

mov ax, [fs:si + 0x1A] ; Heads on stroage media
push word ax
call print_word
add sp, 0x2

call print_space

mov eax, [fs:si + 0x1C] ; Hidden sectors
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_space

mov eax, [fs:si + 0x20] ; Total sectors (large)
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_cr_nl

mov eax, [fs:si + 0x24] ; Sectors per FAT
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_space

mov ax, [fs:si + 0x28] ; Flags
push word ax
call print_word
add sp, 0x2

call print_space

mov ax, [fs:si + 0x2A] ; FAT version nr.
push word ax
call print_word
add sp, 0x2

call print_space

mov eax, [fs:si + 0x2C] ; Root directory cluster number
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_space

mov ax, [fs:si + 0x30] ; FSInfo sector number
push word ax
call print_word
add sp, 0x2

call print_space

mov ax, [fs:si + 0x32] ; Backup boot sector sector number
push word ax
call print_word
add sp, 0x2

call print_space

mov ax, [fs:si + 0x40] ; Drive number 
push word ax
call print_byte
add sp, 0x2

call print_space

mov ax, [fs:si + 0x41] ; Windows NT flags
push word ax
call print_byte
add sp, 0x2

call print_space

mov ax, [fs:si + 0x42] ; Signature
push word ax
call print_byte
add sp, 0x2

call print_space

mov eax, [fs:si + 0x43] ; Volume ID serial number
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_space

mov bx, si
add bx, 0x47  ; Volume label string
push word fs ; Set string address segment
push word bx ; Set string address offset
push word 0xB ; Set string length
call print_string
add sp, 0x4
pop fs

call print_space

mov bx, si
add bx, 0x52  ; System identifier string
push word fs ; Set string address segment
push word bx ; Set string address offset
push word 0x8 ; Set string length
call print_string
add sp, 0x4
pop fs

mov bx, [bp + 0x8] ; Return struct offset
mov ax, [bp + 0xA] ; Return struct segment
mov gs, ax
mov di, bx

mov ax, [fs:si + 0xE] ; Reserved sectors
mov [gs:di], ax
mov al, [fs:si + 0x10] ; Number of FATs
mov [gs:di + 0x2], al
mov eax, [fs:si + 0x24] ; Sectors per FAT
mov [gs:di + 0x3], eax
mov eax, [fs:si + 0x2C] ; Root dir. cluster
mov [gs:di + 0x7], eax

mov sp, bp
pop bp
ret

; Print string. Params: string len; string address offset; string address segment
print_string:
push bp
mov bp, sp
push ds
push si
push bx
push ax

cld                ; Set lodsb to increment DS:SI
mov bx, [bp + 0x4] ; String len
mov si, [bp + 0x6] ; String offset
mov ds, [bp + 0x8] ; String segment
print_string_char:
lodsb           ; Load byte into AL
push bx         ; Save BX

mov ah, 0x0e    ; Set write teletype
mov bl, 0x00    ; Set page nr.
int 0x10        ; Call BIOS interrupt video

pop bx          ; Get BX
dec bx          ; Decrement BX
cmp bx, 0x0
jne print_string_char

pop ax
pop bx
pop si
pop ds
mov sp, bp
pop bp
ret

; Parse mbr. Params: mbr address offset; mbr address segment
parse_mbr:
push bp
mov bp, sp
sub sp, 0x4

mov si, [bp + 0x4] ; Set MBR offset
mov fs, [bp + 0x6] ; Set MBR segment

mov ax, [fs:si + 0x1BE] ; Drive attribute
push word ax
call print_byte
add sp, 0x2

call print_space

mov ax, [fs:si + 0x1BE + 0x1] ; Starting head
push word ax
call print_byte
add sp, 0x2

call print_space

mov ah, [fs:si + 0x1BE + 0x2] ; Starting cylinder high 2 bits
shr ax, 0x6
mov al, [fs:si + 0x1BE + 0x3] ; Starting cylinder low 8 bits
push word ax
call print_word
add sp, 0x2

call print_space

mov al, [fs:si + 0x1BE + 0x2] ; Starting sector
and ax, 0x3F
push word ax
call print_byte
add sp, 0x2

call print_space

mov ax, [fs:si + 0x1BE + 0x4] ; System ID
push word ax
call print_byte
add sp, 0x2

call print_space

mov ax, [fs:si + 0x1BE + 0x5] ; Ending head
push word ax
call print_byte
add sp, 0x2

call print_space

mov ah, [fs:si + 0x1BE + 0x6] ; Ending cylinder high 2 bits
shr ax, 0x6
mov al, [fs:si + 0x1BE + 0x7] ; Ending cylinder low 8 bits
push word ax
call print_word
add sp, 0x2

call print_space

mov al, [fs:si + 0x1BE + 0x6] ; Ending sector
and ax, 0x3F
push word ax
call print_byte
add sp, 0x2

call print_space

mov eax, [fs:si + 0x1BE + 0x8] ; Relative sector
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

call print_space

mov eax, [fs:si + 0x1BE + 0xC] ; Total sectors
push ax
shr eax, 0x10
push ax
call print_dword
add sp, 0x4

mov eax, [fs:si + 0x1BE + 0x8]

mov sp, bp
pop bp
ret

; Read sector. Params: nr. of sectors; dest. offset; dest. segment; low 4 bytes lba start; high 4 bytes lba start
read_sector:
push bp
mov bp, sp

mov ax, [bp + 0x4]
mov [disk_address_packet + 0x2], ax ; Set nr. of sectors
mov ax, [bp + 0x6]
mov [disk_address_packet + 0x4], ax ; Set dest. offest
mov ax, [bp + 0x8]
mov [disk_address_packet + 0x6], ax ; Set dest. segment
mov eax, [bp + 0xA]
mov [disk_address_packet + 0x8], eax ; Set low 4 bytes lba start
mov eax, [bp + 0xE]
mov [disk_address_packet + 0xC], eax ; Set high 4 bytes lba start

mov ax, 0x0000
mov ds, ax
stc                             ; Set carry
mov ah, 0x42                    ; INT 13h. AH = 42h
mov dl, 0x80                    ; Drive number. Presume 80h
mov si, disk_address_packet     ; DS:SI pointer to disk address packet structure. DS is 0
int 0x13
jc read_sector_error            ; Goto error if CF = 1 
mov ax, 0x00                    ; AX = 0 on success
jmp read_sector_exit            ; Goto exit if CF = 0

read_sector_error:
mov al, 0x01                    ; AH = error code. Set AL = 0

read_sector_exit:

pop bp
ret

; Print carriage return and new line
print_cr_nl:
push bp
mov bp, sp

push word 0x000D ; carriage return
call print_4bits
add sp, 0x02

push word 0x000A ; new line
call print_4bits
add sp, 0x02

pop bp
ret

; Print space
print_space:
push bp
mov bp, sp

push word 0x0020 ; new line
call print_4bits
add sp, 0x02

pop bp
ret

; Enable A20 via BIOS
test_a20_bios_enable:
push bp
mov bp, sp

stc
mov ax, 0x2401 ; BIOS enable A20. CF 0 if successful. CF set if error and AH status
int 0x15
jc test_a20_bios_enable_error
mov ax, 0x0
jmp test_a20_bios_enable_exit

test_a20_bios_enable_error:
mov ax, 0x1

test_a20_bios_enable_exit:

pop bp
ret

; Check for A20 status via BIOS
test_a20_bios_status:
push bp
mov bp, sp

stc
mov ax, 0x2402 ; BIOS query A20 status. CF 0 if successful and AL status. CF set if error and AH status
int 0x15
jc test_a20_bios_status_error
mov bx, 0x0
mov bl, al
mov ax, 0x0
jmp test_a20_bios_status_exit

test_a20_bios_status_error:
mov bx, 0x0
mov ax, 0x1

test_a20_bios_status_exit:

pop bp
ret

; Test A20 BIOS support
test_a20_bios_support:
push bp
mov bp, sp

stc
mov ax, 0x2403 ; BIOS query a20 support. CF 0 if supported. CF set and AH status on error 
int 0x15
jnc a20_bios_support_enabled
jmp a20_bios_support_exit

a20_bios_support_enabled:
mov ax, 0x0

a20_bios_support_exit:

pop bp
ret

; Test a20 line, if enabled ax = 0, if disabled ax = 1
test_a20:
push bp
mov bp, sp

pushf
push es
push ds
push si
push di

xor ax, ax
mov ds, ax
mov ax, 0x7BFE ; Set DS:SI 0x0000:0x7BFE, last word of bootsector 
mov si, ax

xor ax, ax
not ax
mov es, ax
mov ax, 0x7C0E ; Set ES:DI 0xFFFF:0x7C0E
mov di, ax

push word es
call print_word
pop es

push word di
call print_word
pop di

mov ax, [ds:si]
mov bx, [es:di]
cmp ax, bx
jne a20_enabled

mov ax, [ds:si]
mov bh, al
mov bl, ah
mov [ds:si], bx
mov ax, [es:di]
cmp ax, bx
jne a20_enabled

mov ax, 0x1
jmp a20_exit

a20_enabled:
mov ax, 0x0

a20_exit:

pop di
pop si
pop ds
pop es
popf

pop bp
ret

; Print doubleword
print_dword:
push bp
mov bp, sp

mov ax, [bp + 0x04]
push word ax
call print_word
add sp, 0x02

mov ax, [bp + 0x06]
push word ax
call print_word
add sp, 0x02

pop bp
ret

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

continuation_value:
dd 0x0000   ; Continuation value

address_range_descriptor:                   
dd 0x0000   ; Base addr. low bits
dd 0x0000   ; Base addr. high bits
dd 0x0000   ; Length in bytes low bits
dd 0x0000   ; Length in bytes high bits
dd 0x0000   ; Type
dd 0x0000   ; Extended attributes

address_range_descriptor_counter:
dw 0x2

disk_address_packet:
db 0x10     ; Set to 10h
db 0x00     ; Set to 0
dw 0x0000   ; Number of sectors to read
dw 0x0000   ; Dest. offset
dw 0x0000   ; Dest. segment
dd 0x0000   ; Low 4 bytes of LBA start
dd 0x0000   ; High 4 bytes of LBA start

parse_boot_record_return_s:
dw 0x0000   ; Reserved sectors
db 0x00     ; Number of FATs
dd 0x0000   ; Sectors per FAT
dd 0x0000   ; Root directory cluster

parse_kernel_elf_s:
dd 0x0 ; Kernel location
dd 0x0 ; Kernel size

partition_lba_start:
dd 0x0000 ; partition lba start

parse_boot_record_print_string:
db 'PARSE_BOOT_RECORD:', 0

read_kernel_elf_print_string:
db 'READ_KERNEL_ELF:', 0

parse_kernel_elf_print_string:
db 'PARSE_KERNEL_ELF:', 0  

parse_kernel_elf_s_print_string:
db 'PARSE_KERNEL_ELF_STRUCTURE:', 0

kernel_name:
db 'STUB', 0

kernel_extension:
db 'ELF', 0

kernel_first_cluster:
dd 0x0

fat_first_sector:
dd 0x0

data_first_sector:
dd 0x0

gdtr:
dw 0x0 ; Limit GDT
dd 0x0 ; Base GDT

gdt_table:
; Null descriptor
dd 0x0
dd 0x0
; Code segment
dw 0xFFFF  ; Limit 16 bits
dw 0x0     ; Base 16 bits
db 0x0     ; Base 8 bits
db 0x9A    ; Access 8 bits 
db 0xCF    ; Flags 4 bits = C; Limit 4 bits = F
db 0x0     ; Base 8 bits
; Data segment
dw 0xFFFF  ; Limit 16 bits
dw 0x0     ; Base 16 bits
db 0x0     ; Base 8 bits
db 0x92    ; Access 8 bits 
db 0xCF    ; Flags 4 bits = C; Limit 4 bits = F
db 0x0     ; Base 8 bits
gdt_table_end: