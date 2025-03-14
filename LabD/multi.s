section .bss
    buffer_storage: resd 1               ; Reserve space for struct pointer
    input_buffer: resb 600               ; Buffer to store input

section .data
    x_struct_val: db 5                   ; x structure value
    x_data: db 0xaa, 1, 2, 0x44, 0x4f    ; x data sequence
    y_struct_val: db 6                   ; y structure value
    y_data: db 0xaa, 1, 2, 3, 0x44, 0x4f ; y data sequence
    state_flag: dw 0xACE1                ; Initial state value
    small_value: db 0                    ; Variable for smallest value
    large_value: db 0                   ; Variable for largest value
    mask_value: dw 0x002D               ; Mask value for state manipulation
    new_line: db 10, 0                  ; New line character
    format_hex: db "%02hhx", 0          ; Hexadecimal format for printing

section .text
global main
extern malloc
extern strlen
extern fgets
extern stdin
extern printf
extern free
extern puts

main:
start_program:

    push ebp
    mov ebp, esp
    mov eax, [ebp+8]           ; argc - argument count
    mov ebx, [ebp+12]          ; argv - argument vector
process_arguments:
    cmp eax, 1
    je no_arguments_provided   ; If no arguments, jump to no_arguments_provided
    mov eax, [ebx + 4]         ; Load argument into eax
    cmp word [eax], "-R"       ; Check if argument is "-R"
    je run_R_function
    cmp word [eax], "-I"       ; Check if argument is "-I"
    je run_I_function

    jmp exit_program           ; Exit program if no valid arguments

exit_program: ; Exiting program
    pop ebp
    ret    

no_arguments_provided:
    push x_struct_val          
    push y_struct_val          
    call add_numbers          
    push eax                   
    call print_numbers        
    call free                 
    add esp, 12              
    pop ebp
    ret

run_I_function:
    call get_input_data
    push eax
    call get_input_data
    push eax
    call add_numbers
    push eax
    call print_numbers
    call free
    add esp, 4
    call free
    add esp, 4
    call free
    add esp, 4
    pop ebp
    ret

run_R_function:
    call generate_random_data
    push eax
    call generate_random_data
    push eax
    call add_numbers
    push eax
    call print_numbers
    call free
    add esp, 4
    call free
    add esp, 4
    call free
    add esp, 4
    pop ebp
    ret

print_numbers:
    push ebp                  
    mov ebp, esp               
    pushad                      
    mov edi, [ebp+8]            
    movzx ebx, byte[edi]        ; struct size

; Loop over struct elements and print them
print_loop:
    movzx ecx, byte[edi + ebx]  ; Retrieve byte from struct (little endian)
    push ecx                    
    push format_hex                 
    call printf        
    add esp, 8                  
    dec ebx                       ; Move to next byte
    cmp bl, 0                    
    jne print_loop               ; Continue if not the end

print_new_line:
    push new_line                ; Push newline character
    call printf                 
    add esp, 4                  
    popad                       
    pop ebp                    
    ret                         ; Return to main

get_input_data:
    push ebp                   
    mov ebp, esp       
    pushad         ; Save all registers

read_input:
    push dword[stdin]          
    push 600                  
    push input_buffer              
    call fgets                  ; Read input into buffer
    add esp, 12     

input_length:
    push input_buffer                
    call strlen                 ; Get length of the input
    add esp, 4                 
    mov edi, eax

adjust_size:
    dec edi
    dec edi                
    shr eax, 1                  
    inc eax 

allocate_memory_for_struct:             
    push eax                    
    call malloc                 ; Allocate memory
    mov dword[buffer_storage], eax      
    mov esi, eax                
    pop eax                     
    dec eax                    
    mov byte[esi], al         
    mov ecx, 1

parse_input:
    mov ebx, 0                  ; Reset
    mov bh, byte[input_buffer + edi]  ; Take first character
    dec edi                     
    
    cmp bh, 'a'                 ; Check if character is letter or number
    jge handle_character                  
    sub bh, '0'                
    jmp swap_bytes

handle_character:
    sub bh, 'a'                
    add bh, 0xa                 

swap_bytes:
    mov bl, bh                  
    mov bh, 0                  
    cmp edi, 0
    jl construct_value
    mov bh, byte[input_buffer + edi]  ; Take next character
    dec edi                     
    cmp bh, 'a'                 ; Check if second character is letter or number
    jge second_character                   
    add bh, 0xa       

second_character:
    sub bh, 'a'                
    add bh, 0xa                

construct_value:
    shl bh, 4                 
    or bl, bh                 

store_in_struct:
    mov byte[esi + ecx], bl    
    inc ecx                    
    cmp edi, 0                  
    jge parse_input           ; Continue parsing if not finished

end_parsing:
    jmp save_buffer_value                      
    ret                                               

MaxMinComparison:
    movzx edx, byte[ebx]      
    movzx ecx, byte[eax]                
    cmp ecx, edx                 
    jae no_swap                     ; If eax >= ebx, no swap
    xchg eax, ebx             
no_swap:            
    ret                           

add_numbers:
    push ebp              
    mov ebp, esp              
    pushad                     
    mov eax, [ebp+8]            
    mov ebx, [ebp+12]          

print_inputs:
    push ebx                    
    call print_numbers            ; Print second input 
    add esp, 4   
    push eax                  
    call print_numbers            ; Print first input 
    add esp, 4              
    call MaxMinComparison        ; Swap if needed (eax > ebx)

save_input_for_looping:
    mov edi, ebx    
    mov esi, eax                 
    movzx eax, byte[edi]        
    mov byte[small_value], al 
    movzx eax, byte[esi]        
    mov byte[large_value], al   
    inc eax
    inc eax              
    push eax                  
    call malloc                
    mov dword[buffer_storage], eax    
    pop ecx                    
    sub ecx,1                     
    mov byte[eax], cl   

update_numbers:        
    mov ecx, 0                  
    mov edx, 0                 
    inc esi                     ; Move to next byte in first input
    inc edi                     ; Move to next byte in second input
    inc eax                     ; Move to next byte in result  

loop_addition:
    movzx ebx, byte[esi]        ; First byte of first input
    add ebx, ecx                
    movzx ecx, byte[edi]        ; First byte of second input
    add ebx, ecx               
    mov cl, bh                  
    mov byte[eax], bl           ; Store the result

update_addition:
    add edx, 1                    
    add esi, 1                    ; Move to next byte in first input
    add edi, 1                    ; Move to next byte in second input
    add eax, 1                    ; Move to next byte in result
    cmp dl, byte[small_value]  
    jne loop_addition            

    cmp dl, byte[large_value]  
    je skip_addition

continue_addition_for_second_byte:
    movzx ebx, byte[esi]        
    add ebx, ecx             
    mov cl, bh                  
    mov byte[eax], bl

update_addition2:         
    add edx, 1                    
    add esi, 1                   
    add eax, 1                     
    cmp dl, byte[large_value]         
    jne continue_addition_for_second_byte

skip_addition:
    mov byte[eax], cl         
    jmp save_buffer_value       
    ret                                       

generate_random_data:
    push ebp                   
    mov ebp, esp                
    pushad                      

generate_random_size:
    call rand_value                            ; Get random number    
    cmp al, 0                 
    je generate_random_size                               ; Repeat until size is non-zero

    movzx ebx, al               
    add ebx, 1                  
    push ebx                   
    call malloc                              ; Allocate memory (size = bl)
    mov dword[buffer_storage], eax      
    pop ebx                     
    sub ebx, 1                    
    mov byte[eax], bl           
    mov esi, eax              
    mov edx, 0                  

generate_random_array:
    call rand_value                        ; Generate random byte
    mov byte[esi + edx + 1], al 
    inc edx                
    sub ebx, 1                 
    jnz generate_random_array   ; Continue until done
    
    jmp save_buffer_value       
    ret                         

rand_value:
    push ebp                    
    mov ebp, esp               
    pushad                     
random_value_gen:
    mov bx, 0              
    movzx eax, word[state_flag] 
    and ax, [mask_value]          
    jnp parity_check            
    mov bx, 0x8000          
parity_check:
    movzx eax, word[state_flag]  
    shr ax, 1               
    or ax, bx              
    mov word[state_flag], ax    
saving_rand_value:
    popad                     
    pop ebp                     
    movzx eax, word[state_flag]      
    ret        

save_buffer_value:
    popad                      
    pop ebp                    
    mov eax, [buffer_storage]           
    ret   

_exit:
end_program:
    pop ebp
    ret    