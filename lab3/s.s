          .globl  running,scheduler,tswitch
tswitch:
SAVE:                            # on entry: stack top = retPC
	  pushl %eax             # save CPU registers on stack
	  pushl %ebx
	  pushl %ecx
	  pushl %edx
	  pushl %ebp
	  pushl %esi
	  pushl %edi
          pushfl

          movl   running,%ebx    # ebx -> running PROC
          movl   %esp,4(%ebx)    # running PROC.saved_sp = esp

FIND:	  call   scheduler       # pick a new running PROC

RESUME:	  movl   running,%ebx    # ebx -> (new) running PROC
          movl   4(%ebx),%esp    # esp =  (new) running PROC.saved_sp

          popfl                  # restore saved registers from stack
	  popl  %edi
          popl  %esi
          popl  %ebp
          popl  %edx
          popl  %ecx
          popl  %ebx
          popl  %eax
	
          ret                    # return by retPC on top of stack 

# stack contents = |retPC|eax|ebx|ecx|edx|ebp|esi|edi|eflag|
#                    -1   -2  -3  -4  -5  -6  -7  -8   -9
	