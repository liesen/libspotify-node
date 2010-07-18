// Lock-free (CAS aka spinlock style) thread-safe queue (singly linked list)

#if __ppc__

  .text
  .align          2
  .globl          _nt_atomic_dequeue32_mp
  .private_extern _nt_atomic_dequeue32_mp
_nt_atomic_dequeue32_mp:                // void * nt_atomic_dequeue(nt_atomic_queue *queue, size_t offset);
  mr      r5,r3
1:
  lwarx   r3,0,r5                 // get 1st item in queue
  cmpwi   r3,0                    // null?
  beqlr                           // yes, queue empty
  lwzx    r6,r3,r4                // get 2nd item
  stwcx.  r6,0,r5                 // make 2nd item first
  bne--   1b
  isync                           // cancel read-aheads (nop'd on UP)
  blr

  .text
  .align          2
  .globl          _nt_atomic_dequeue32_up
  .private_extern _nt_atomic_dequeue32_up
_nt_atomic_dequeue32_up:                // void * nt_atomic_dequeue(nt_atomic_queue *queue, size_t offset);
  mr      r5,r3
1:
  lwarx   r3,0,r5                 // get 1st item in queue
  cmpwi   r3,0                    // null?
  beqlr                           // yes, queue empty
  lwzx    r6,r3,r4                // get 2nd item
  stwcx.  r6,0,r5                 // make 2nd item first
  bne--   1b
  blr

  .text
  .align          2
  .globl          _nt_atomic_dequeue_ifnexteq32_mp
  .private_extern _nt_atomic_dequeue_ifnexteq32_mp
_nt_atomic_dequeue_ifnexteq32_mp:          // void * nt_atomic_dequeue_ifnexteq(nt_atomic_queue *queue, size_t offset, void *cmpptr);
  mr      r6,r3
1:
  lwsync                          // write barrier
  lwarx   r3,0,r6                 // get 1st item in queue
  cmpwi   r3,0                    // null?
  beqlr                           // yes, queue empty
  cmpw    r3,r5                   // 1st item still == cmpptr?
  bne--   2f                      // no, something changed
  lwzx    r7,r3,r4                // get 2nd item
  stwcx.  r7,0,r6                 // make 2nd item first
  bne--   1b
  isync                           // cancel read-aheads (nop'd on UP)
  blr
2:      li      r3,0
  blr

  .text
  .align          2
  .globl          _nt_atomic_dequeue_ifnexteq32_up
  .private_extern _nt_atomic_dequeue_ifnexteq32_up
_nt_atomic_dequeue_ifnexteq32_up:          // void * nt_atomic_dequeue_ifnexteq(nt_atomic_queue *queue, size_t offset, void *cmpptr);
  mr      r6,r3
1:
  lwarx   r3,0,r6                 // get 1st item in queue
  cmpwi   r3,0                    // null?
  beqlr                           // yes, queue empty
  cmpw    r3,r5                   // 1st item still == cmpptr?
  bne--   2f                      // no, something changed
  lwzx    r7,r3,r4                // get 2nd item
  stwcx.  r7,0,r6                 // make 2nd item first
  bne--   1b
  blr
2:      li      r3,0
  blr

  .text
  .align          2
  .globl          _nt_atomic_dequeue32_on64_mp
  .private_extern _nt_atomic_dequeue32_on64_mp
_nt_atomic_dequeue32_on64_mp:           // void * nt_atomic_dequeue(nt_atomic_queue *queue, size_t offset);
  mr      r5,r3
  li      r7,-8                   // use red zone to release reservation if necessary
1:
  lwarx   r3,0,r5                 // get 1st item in queue
  cmpwi   r3,0                    // null?
  beq     2f
  lwzx    r6,r3,r4                // get 2nd item
  stwcx.  r6,0,r5                 // make 2nd item first
  isync                           // cancel read-aheads (nop'd on UP)
  beqlr++                         // return next item in r2
  b       1b                      // retry (lost reservation)
2:
  stwcx.  r0,r7,r1                // on 970, release reservation using red zone
  blr                             // return null

  .text
  .align          2
  .globl          _nt_atomic_dequeue32_on64_up
  .private_extern _nt_atomic_dequeue32_on64_up
_nt_atomic_dequeue32_on64_up:           // void * nt_atomic_dequeue(nt_atomic_queue *queue, size_t offset);
  mr      r5,r3
  li      r7,-8                   // use red zone to release reservation if necessary
1:
  lwarx   r3,0,r5                 // get 1st item in queue
  cmpwi   r3,0                    // null?
  beq     2f
  lwzx    r6,r3,r4                // get 2nd item
  stwcx.  r6,0,r5                 // make 2nd item first
  beqlr++                         // return next item in r2
  b       1b                      // retry (lost reservation)
2:
  stwcx.  r0,r7,r1                // on 970, release reservation using red zone
  blr                             // return null

  .text
  .align          2
  .globl          _nt_atomic_dequeue_ifnexteq32_on64_mp
  .private_extern _nt_atomic_dequeue_ifnexteq32_on64_mp
_nt_atomic_dequeue_ifnexteq32_on64_mp:     // void * nt_atomic_dequeue(nt_atomic_queue *queue, size_t offset);
  mr      r6,r3
  li      r8,-8                   // use red zone to release reservation if necessary
1:
  lwarx   r3,0,r6                 // get 1st item in queue
  cmpwi   r3,0                    // null?
  beq     2f
  cmpw    r3,r5                   // 1st item still == cmpptr?
  bne--   2f                      // no, something changed
  lwzx    r7,r3,r4                // get 2nd item
  stwcx.  r7,0,r6                 // make 2nd item first
  isync                           // cancel read-aheads (nop'd on UP)
  beqlr++                         // return next item in r2
  b       1b                      // retry (lost reservation)
2:
  stwcx.  r0,r8,r1                // on 970, release reservation using red zone
  blr                             // return null


  .text
  .align          2
  .globl          _nt_atomic_dequeue_ifnexteq32_on64_up
  .private_extern _nt_atomic_dequeue_ifnexteq32_on64_up
_nt_atomic_dequeue_ifnexteq32_on64_up:     // void * nt_atomic_dequeue(nt_atomic_queue *queue, size_t offset);
  mr      r6,r3
  li      r8,-8                   // use red zone to release reservation if necessary
1:
  lwarx   r3,0,r6                 // get 1st item in queue
  cmpwi   r3,0                    // null?
  beq     2f
  cmpw    r3,r5                   // 1st item still == cmpptr?
  bne--   2f                      // no, something changed
  lwzx    r7,r3,r4                // get 2nd item
  stwcx.  r7,0,r6                 // make 2nd item first
  beqlr++                         // return next item in r2
  b       1b                      // retry (lost reservation)
2:
  stwcx.  r0,r8,r1                // on 970, release reservation using red zone
  blr                             // return null



  .text
  .align          2
  .globl          _nt_atomic_enqueue32_mp
  .private_extern _nt_atomic_enqueue32_mp
_nt_atomic_enqueue32_mp:                // void nt_atomic_enqueue(nt_atomic_queue *queue, void *elem, size_t offset);
1:
  lwarx   r6,0,r3                 // get ptr to 1st item in queue
  stwx    r6,r4,r5                // hang queue off new item
  eieio                           // make sure the "stwx" comes before "stwcx."
  stwcx.  r4,0,r3                 // make new 1st item in queue
  beqlr++
  b       1b

  .text
  .align          2
  .globl          _nt_atomic_enqueue32_up
  .private_extern _nt_atomic_enqueue32_up
_nt_atomic_enqueue32_up:                // void nt_atomic_enqueue(nt_atomic_queue *queue, void *elem, size_t offset);
1:
  lwarx   r6,0,r3                 // get ptr to 1st item in queue
  stwx    r6,r4,r5                // hang queue off new item
  stwcx.  r4,0,r3                 // make new 1st item in queue
  beqlr++
  b       1b


#endif // __ppc__ 


#ifdef __ppc64__

  .text
  .align          2
  .globl          _nt_atomic_dequeue64_mp
  .private_extern _nt_atomic_dequeue64_mp
_nt_atomic_dequeue64_mp:                // void * nt_atomic_dequeue(nt_atomic_queue *queue, size_t offset);
  mr      r5,r3
  li      r7,-8                   // use red zone to release reservation if necessary
1:
  ldarx   r3,0,r5                 // get 1st item in queue
  cmpdi   r3,0                    // null?
  beq     2f
  ldx     r6,r3,r4                // get 2nd item
  stdcx.  r6,0,r5                 // make 2nd item first
  isync                           // cancel read-aheads (nop'd on UP)
  beqlr++                         // return next item in r2
  b       1b                      // retry (lost reservation)
2:
  stdcx.  r0,r7,r1                // on 970, release reservation using red zone
  blr                             // return null

  .text
  .align          2
  .globl          _nt_atomic_dequeue64_up
  .private_extern _nt_atomic_dequeue64_up
_nt_atomic_dequeue64_up:                // void * nt_atomic_dequeue(nt_atomic_queue *queue, size_t offset);
  mr      r5,r3
  li      r7,-8                   // use red zone to release reservation if necessary
1:
  ldarx   r3,0,r5                 // get 1st item in queue
  cmpdi   r3,0                    // null?
  beq     2f
  ldx     r6,r3,r4                // get 2nd item
  stdcx.  r6,0,r5                 // make 2nd item first
  beqlr++                         // return next item in r2
  b       1b                      // retry (lost reservation)
2:
  stdcx.  r0,r7,r1                // on 970, release reservation using red zone
  blr                             // return null


  .text
  .align          2
  .globl          _nt_atomic_dequeue_ifnexteq64_mp
  .private_extern _nt_atomic_dequeue_ifnexteq64_mp
_nt_atomic_dequeue_ifnexteq64_mp:          // void * nt_atomic_dequeue(nt_atomic_queue *queue, size_t offset);
  mr      r6,r3
  li      r8,-8                   // use red zone to release reservation if necessary
1:
  ldarx   r3,0,r6                 // get 1st item in queue
  cmpdi   r3,0                    // null?
  beq     2f                      // yes, queue empty
  cmpw    r3,r5                   // 1st item still == cmpptr?
  bne--   2f                      // no, something changed
  ldx     r7,r3,r4                // get 2nd item
  stdcx.  r7,0,r6                 // make 2nd item first
  isync                           // cancel read-aheads (nop'd on UP)
  beqlr++                         // return next item in r2
  b       1b                      // retry (lost reservation)
2:
  stdcx.  r0,r8,r1                // on 970, release reservation using red zone
  blr                             // return null

  .text
  .align          2
  .globl          _nt_atomic_dequeue_ifnexteq64_up
  .private_extern _nt_atomic_dequeue_ifnexteq64_up
_nt_atomic_dequeue_ifnexteq64_up:          // void * nt_atomic_dequeue(nt_atomic_queue *queue, size_t offset);
  mr      r6,r3
  li      r8,-8                   // use red zone to release reservation if necessary
1:
  ldarx   r3,0,r6                 // get 1st item in queue
  cmpdi   r3,0                    // null?
  beq     2f                      // yes, queue empty
  cmpw    r3,r5                   // 1st item still == cmpptr?
  bne--   2f                      // no, something changed
  ldx     r7,r3,r4                // get 2nd item
  stdcx.  r7,0,r6                 // make 2nd item first
  beqlr++                         // return next item in r2
  b       1b                      // retry (lost reservation)
2:
  stdcx.  r0,r8,r1                // on 970, release reservation using red zone
  blr                             // return null


  .text
  .align          2
  .globl          _nt_atomic_enqueue64_mp
  .private_extern _nt_atomic_enqueue64_mp
_nt_atomic_enqueue64_mp:                // void nt_atomic_enqueue(nt_atomic_queue *queue, void *elem, size_t offset);
1:
  ldarx   r6,0,r3                 // get ptr to 1st item in queue
  stdx    r6,r4,r5                // hang queue off new item
  lwsync                          // make sure the "stdx" comes before the "stdcx." (nop'd on UP)
  stdcx.  r4,0,r3                 // make new 1st item in queue
  bne--   1b
  li      r3,0
  blr

  .text
  .align          2
  .globl          _nt_atomic_enqueue64_up
  .private_extern _nt_atomic_enqueue64_up
_nt_atomic_enqueue64_up:                // void nt_atomic_enqueue(nt_atomic_queue *queue, void *elem, size_t offset);
1:
  ldarx   r6,0,r3                 // get ptr to 1st item in queue
  stdx    r6,r4,r5                // hang queue off new item
  stdcx.  r4,0,r3                 // make new 1st item in queue
  beqlr++
  b       1b


#endif

#ifdef __i386__

  .text
  .align          2
  .globl          _nt_atomic_enqueue32_up
  .private_extern _nt_atomic_enqueue32_up
_nt_atomic_enqueue32_up:
  .globl          _nt_atomic_enqueue32_mp
  .private_extern _nt_atomic_enqueue32_mp
_nt_atomic_enqueue32_mp:                // void nt_atomic_enqueue(nt_atomic_queue *queue, void *elem, size_t offset);
  pushl   %edi
  pushl   %esi
  pushl   %ebx
  movl    16(%esp),%edi           // %edi == ptr to queue head
  movl    20(%esp),%ebx           // %ebx == item
  movl    24(%esp),%esi           // %esi == offset
  movl    (%edi),%eax             // %eax == ptr to 1st item in queue
  movl    4(%edi),%edx            // %edx == current generation count
1:
  movl    %eax,(%ebx,%esi)        // link to old queue head from new item
  movl    %edx,%ecx
  incl    %ecx                    // increment generation count
  lock                            // always lock for now...
  cmpxchg8b (%edi)                // ...push on new item
  jnz     1b
  popl    %ebx
  popl    %esi
  popl    %edi
  ret

  .text
  .align          2
  .globl          _nt_atomic_dequeue32_up
  .private_extern _nt_atomic_dequeue32_up
_nt_atomic_dequeue32_up:
  .globl          _nt_atomic_dequeue32_mp
  .private_extern _nt_atomic_dequeue32_mp
_nt_atomic_dequeue32_mp:                // void * nt_atomic_dequeue(nt_atomic_queue *queue, size_t offset);
  pushl   %edi
  pushl   %esi
  pushl   %ebx
  movl    16(%esp),%edi           // %edi == ptr to queue head
  movl    20(%esp),%esi           // %esi == offset
  movl    (%edi),%eax             // %eax == ptr to 1st item in queue
  movl    4(%edi),%edx            // %edx == current generation count
1:
  testl   %eax,%eax               // queue empty?
  jz      2f                      // yes
  movl    (%eax,%esi),%ebx        // point to 2nd item in queue
  movl    %edx,%ecx
  incl    %ecx                    // increment generation count
  lock                            // always lock for now...
  cmpxchg8b (%edi)                // ...pop off 1st item
  jnz     1b
2:
  popl    %ebx
  popl    %esi
  popl    %edi
  ret                             // ptr to 1st item in queue still in %eax


  .text
  .align          2
  .globl          _nt_atomic_dequeue_ifnexteq32_up
  .private_extern _nt_atomic_dequeue_ifnexteq32_up
_nt_atomic_dequeue_ifnexteq32_up:
  .globl          _nt_atomic_dequeue_ifnexteq32_mp
  .private_extern _nt_atomic_dequeue_ifnexteq32_mp
_nt_atomic_dequeue_ifnexteq32_mp:          // void * nt_atomic_dequeue_ifnexteq(nt_atomic_queue *queue, size_t offset, void *cmpptr);
  pushl   %edi
  pushl   %esi
  pushl   %ebx
  movl    16(%esp),%edi           // %edi == ptr to queue head
  movl    20(%esp),%esi           // %esi == offset
  movl    (%edi),%eax             // %eax == ptr to 1st item in queue
  movl    4(%edi),%edx            // %edx == current generation count
1:
  testl   %eax,%eax               // queue empty?
  jz      2f                      // yes
  cmp     24(%esp),%eax           // 1st item still == cmpptr?
  jne     2f                      // no, something changed
  movl    (%eax,%esi),%ebx        // point to 2nd item in queue
  movl    %edx,%ecx
  incl    %ecx                    // increment generation count
  lock                            // always lock for now...
  cmpxchg8b (%edi)                // ...pop off 1st item
  jnz     1b
  popl    %ebx
  popl    %esi
  popl    %edi
  ret                             // ptr to 1st item in queue still in %eax
2:
  xorl    %eax,%eax               // Unconditionally return NULL
  popl    %ebx
  popl    %esi
  popl    %edi
  ret


#endif // __i386__

#ifdef __x86_64__

  .code64
  .text
  .align          2
  .globl          _nt_atomic_enqueue64_up
  .private_extern _nt_atomic_enqueue64_up
_nt_atomic_enqueue64_up:
  .globl          _nt_atomic_enqueue64_mp
  .private_extern _nt_atomic_enqueue64_mp
                                  // void nt_atomic_enqueue(nt_atomic_queue *queue, void *elem, size_t offset);
_nt_atomic_enqueue64_mp:                // %rdi == nt_atomic_queue *, %rsi == item, %rdx == offset
  pushq   %rbx
  movq    %rsi,%rbx               // %rbx == item
  movq    %rdx,%rsi               // %rsi == offset
  movq    (%rdi),%rax             // %rax == ptr to 1st item in queue
  movq    8(%rdi),%rdx            // %rdx == current generation count
1:
  movq    %rax,(%rbx,%rsi)        // link to old queue head from new item
  movq    %rdx,%rcx
  incq    %rcx                    // increment generation count
  lock                            // always lock for now...
  cmpxchg16b (%rdi)               // ...push on new item
  jnz     1b
  popq    %rbx
  ret

  .text
  .align          2
  .code64
  .globl          _nt_atomic_dequeue64_up
  .private_extern _nt_atomic_dequeue64_up
_nt_atomic_dequeue64_up:
  .globl          _nt_atomic_dequeue64_mp
  .private_extern _nt_atomic_dequeue64_mp
                                  // void * nt_atomic_dequeue(nt_atomic_queue *queue, size_t offset);
_nt_atomic_dequeue64_mp:                // %rdi == queue head, %rsi == offset
  pushq   %rbx
  movq    (%rdi),%rax             // %rax == ptr to 1st element in queue
  movq    8(%rdi),%rdx            // %rdx == current generation count
1:
  testq   %rax,%rax               // queue empty?
  jz      2f                      // yes
  movq    (%rax,%rsi),%rbx        // point to 2nd in queue
  movq    %rdx,%rcx
  incq    %rcx                    // increment generation count
  lock                            // always lock for now...
  cmpxchg16b (%rdi)               // ...pop off 1st item
  jnz     1b
2:
  popq    %rbx
  ret                             // ptr to 1st item in queue still in %rax

  .text
  .align          2
  .code64
  .globl          _nt_atomic_dequeue_ifnexteq64_up
  .private_extern _nt_atomic_dequeue_ifnexteq64_up
_nt_atomic_dequeue_ifnexteq64_up:
  .globl          _nt_atomic_dequeue_ifnexteq64_mp
  .private_extern _nt_atomic_dequeue_ifnexteq64_mp
                                  // void * nt_atomic_dequeue_ifnexteq(nt_atomic_queue *queue, size_t offset, void *cmpptr);
_nt_atomic_dequeue_ifnexteq64_mp:          // %rdi == nt_atomic_queue *, %rsi == offset, %rdx == cmpptr
  pushq   %rbx
  movq    %rdx,%r8                // %r8 == cmpptr
  movq    (%rdi),%rax             // %rax == ptr to 1st item in queue
  movq    8(%rdi),%rdx            // %rdx == current generation count
1:
  testq   %rax,%rax               // queue empty?
  jz      2f                      // yes
  cmp     %r8,%rax                // 1st item still == cmpptr?
  jne     2f                      // no, something changed
  movq    (%rax,%rsi),%rbx        // point to 2nd item in queue
  movq    %rdx,%rcx
  incq    %rcx                    // increment generation count
  lock                            // always lock for now...
  cmpxchg16b (%rdi)               // ...pop off 1st item
  jnz     1b
  popq    %rbx
  ret                             // ptr to 1st item in queue still in %rax
2:
  xor     %rax,%rax               // Unconditionally return NULL
  popq    %rbx
  ret


#endif