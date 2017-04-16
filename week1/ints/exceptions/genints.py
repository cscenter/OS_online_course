#!/usr/bin/env python

def main():
    def generate_entry(intno, witherr, common):
        err = '\tpushq ${}\n\tjmp {}'
        noerr = '\tpushq $0\n\tpushq ${}\n\tjmp {}'

        return [noerr, err][bool(witherr)].format(intno, common)

    def generate_common(regs, handler):
        prologue = '\tsubq ${}, %rsp\n'.format(len(regs) * 8)
        epilogue = '\taddq ${}, %rsp\n\tiretq\n'.format((len(regs) + 2) * 8)

        save_regs = ''.join('\tmovq %{}, {}(%rsp)\n'.format(reg, i * 8)
                            for i, reg in enumerate(regs))
        restore_regs = ''.join('\tmovq {}(%rsp), %{}\n'.format(i * 8, reg)
                               for i, reg in enumerate(regs))

        call = '\tcld\n\tmovq %rsp, %rdi\n\tcall {}\n'.format(handler)

        return ''.join([prologue, save_regs, call, restore_regs, epilogue])

    # For these interrupts CPU puts 'error code' on stack in
    # addition to everyting else, so they require special care
    witherr = [8, 10, 11, 12, 13, 14, 17]

    # Order is important here, never change order here without
    # changing struct frame accordingly (see inc/ints.h)
    regs = [
        'rbp', 'rbx', 'r15', 'r14', 'r13', 'r12', 'r11',
        'r10', 'r9',  'r8',  'rax', 'rcx', 'rdx', 'rsi', 'rdi'
    ]

    # C code depends on these names, never change names without
    # changing references to them (see src/ints.c)
    table_name = '__raw_handler'
    handler_name = 'isr_handler'

    common = 'common'


    print '\t.text'
    print '\t.code64'
    print '\t.global ' + table_name
    print '\t.extern ' + handler_name

    print

    print common + ':'
    print generate_common(regs, handler_name)

    print

    for i in xrange(256):
        print '\t.align 16'
        print 'entry{}:'.format(i)
        print generate_entry(i, i in witherr, common)
        print

    print '\t.align 16'
    print '{}:'.format(table_name)
    for i in xrange(256):
        print '\t.quad entry{}'.format(i)


if __name__ == '__main__':
    main()
