section .bss
    input_buffer resb 1        ; Reserve 1 byte for the input buffer
    stdin_fd resd 1           ; Reserve a variable for stdin file descriptor
    stdout_fd resd 1          ; Reserve a variable for stdout file descriptor

section .text
    global _start
    global system_call
    extern main

_start:
    pop    dword ecx    ; ecx = argc
    mov    esi, esp    ; esi = argv

    ; Initialize stdin_fd and stdout_fd to defaults (stdin and stdout)
    mov     dword [stdin_fd], 0 
    mov     dword [stdout_fd], 1 

    ; Check for arguments
    cmp     ecx, 1      ; If argc <= 1, no arguments provided
    jle     .main_start

    ; Loop through argv
    mov     ebx, 1      ; Start with argv[1]
.check_args:
    cmp     ebx, ecx    ; If argv index >= argc, stop checking
    jge     .main_start

    mov     edx, [esi + ebx * 4] ; Load argv[ebx]
    cmp     byte [edx], '-'      ; Check if argument starts with '-'
    jne     .next_arg

    cmp     byte [edx+1], 'i'    ; Check if it's "-i"
    je      .handle_input

    cmp     byte [edx+1], 'o'    ; Check if it's "-o"
    je      .handle_output

    jmp     .next_arg

.handle_input:
    ; Argument starts with "-i", open the file
    add     edx, 2               ; Skip "-i" to get the file name
    push    0                    ; Flags for read-only
    push    edx                  ; File name pointer
    push    5                    ; SYS_OPEN
    call    system_call
    add     esp, 12              ; Clean up stack
    test    eax, eax             ; Check if file was opened successfully
    js      .next_arg            ; If not, skip to next argument
    mov     dword [stdin_fd], eax ; Set stdin_fd to the file descriptor
    jmp     .next_arg

.handle_output:
    ; Argument starts with "-o", open the file
    add     edx, 2               ; Skip "-o" to get the file name
   ; push    0666                 ; Permissions (rw-rw-rw-)
    push    577                  ; Flags for O_CREAT | O_WRONLY
    push    edx                  ; File name pointer
    push    5                    ; SYS_OPEN
    call    system_call
    add     esp, 16              ; Clean up stack
    test    eax, eax             ; Check if file was opened successfully
    js      .next_arg            ; If not, skip to next argument
    mov     dword [stdout_fd], eax ; Set stdout_fd to the file descriptor
    jmp     .next_arg

.next_arg:
    inc     ebx                  ; Move to the next argument
    jmp     .check_args

.main_start:
    call    read_and_print_char  ; Call the function to read and print a character

    mov     ebx, eax
    mov     eax, 1      ; SYS_EXIT
    int     0x80        ; Make the system call to exit the program
    nop

system_call:
    push    ebp         ; Save caller state
    mov     ebp, esp
    sub     esp, 4      ; Leave space for local var on stack
    pushad              ; Save some more caller state

    mov     eax, [ebp+8]    ; Copy function args to registers: leftmost...        
    mov     ebx, [ebp+12]   ; Next argument...
    mov     ecx, [ebp+16]   ; Next argument...
    mov     edx, [ebp+20]   ; Next argument...
    int     0x80            ; Transfer control to operating system
    mov     [ebp-4], eax    ; Save returned value...
    popad                   ; Restore caller state (registers)
    mov     eax, [ebp-4]    ; place returned value where caller can see it
    add     esp, 4          ; Restore caller state
    pop     ebp             ; Restore caller state
    ret                     ; Back to caller

read_and_print_char:
    ; Loop to read a single character from stdin
.read_loop:
    mov eax, 3          ; SYS_READ (system call number 3)
    mov ebx, [stdin_fd] ; Use stdin_fd as the file descriptor for input
    mov ecx, input_buffer ; Address of buffer to store input
    mov edx, 1          ; Read 1 byte (1 character)
    int 0x80            ; Make the system call to read a character

    ; If no bytes are read (EOF), return 0 in eax
    test eax, eax
    jz .done

    ; Get the character from the buffer
    mov al, [input_buffer]

    ; Check if the character is in the range 'A' to 'Z'
    cmp al, 'A'
    jl .check_lowercase
    cmp al, 'Z'
    jg .check_lowercase
    ; It's an uppercase letter, add 1
    inc al
    ; If it's 'Z', wrap around to 'A'
    cmp al, 'Z' + 1
    jg .wrap_upper
    jmp .store_and_print

.wrap_upper:
    mov al, 'A'         ; Wrap around to 'A'

.check_lowercase:
    ; Check if the character is in the range 'a' to 'z'
    cmp al, 'a'
    jl .store_and_print
    cmp al, 'z'
    jg .store_and_print
    ; It's a lowercase letter, add 1
    inc al
    ; If it's 'z', wrap around to 'a'
    cmp al, 'z' + 1
    jg .wrap_lower
    jmp .store_and_print

.wrap_lower:
    mov al, 'a'         ; Wrap around to 'a'

.store_and_print:
    ; Store the encoded character back in the buffer
    mov [input_buffer], al

    ; Print the encoded character to stdout or file
    mov eax, 4          ; SYS_WRITE (system call number 4)
    mov ebx, [stdout_fd] ; Use stdout_fd as the file descriptor for output
    mov ecx, input_buffer ; Address of buffer containing the character
    mov edx, 1          ; Write 1 byte (1 character)
    int 0x80            ; Make the system call to print the character

    ; Continue looping to read the next character
    jmp .read_loop

.done:
    ret
