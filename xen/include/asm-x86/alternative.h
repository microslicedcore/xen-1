#ifndef __X86_ALTERNATIVE_H__
#define __X86_ALTERNATIVE_H__

#include <asm/alternative-asm.h>
#include <asm/nops.h>

#ifndef __ASSEMBLY__
#include <xen/stringify.h>
#include <xen/types.h>

struct alt_instr {
    int32_t  orig_offset;   /* original instruction */
    int32_t  repl_offset;   /* offset to replacement instruction */
    uint16_t cpuid;         /* cpuid bit set for replacement */
    uint8_t  orig_len;      /* length of original instruction */
    uint8_t  repl_len;      /* length of new instruction, <= instrlen */
};

#define __ALT_PTR(a,f)      ((uint8_t *)((void *)&(a)->f + (a)->f))
#define ALT_ORIG_PTR(a)     __ALT_PTR(a, orig_offset)
#define ALT_REPL_PTR(a)     __ALT_PTR(a, repl_offset)

extern void add_nops(void *insns, unsigned int len);
/* Similar to alternative_instructions except it can be run with IRQs enabled. */
extern void apply_alternatives(const struct alt_instr *start,
                               const struct alt_instr *end);
extern void alternative_instructions(void);

#define OLDINSTR(oldinstr)      "661:\n\t" oldinstr "\n662:\n"

#define b_replacement(number)   "663"#number
#define e_replacement(number)   "664"#number

#define alt_slen "662b-661b"
#define alt_rlen(number) e_replacement(number)"f-"b_replacement(number)"f"

#define ALTINSTR_ENTRY(feature, number)                                       \
        " .long 661b - .\n"                             /* label           */ \
        " .long " b_replacement(number)"f - .\n"        /* new instruction */ \
        " .word " __stringify(feature) "\n"             /* feature bit     */ \
        " .byte " alt_slen "\n"                         /* source len      */ \
        " .byte " alt_rlen(number) "\n"                 /* replacement len */

#define DISCARD_ENTRY(number)                           /* rlen <= slen */    \
        " .byte 0xff + (" alt_rlen(number) ") - (" alt_slen ")\n"

#define ALTINSTR_REPLACEMENT(newinstr, feature, number) /* replacement */     \
        b_replacement(number)":\n\t" newinstr "\n" e_replacement(number) ":\n\t"

#define ALTERNATIVE_N(newinstr, feature, number)	\
	".pushsection .altinstructions,\"a\"\n"		\
	ALTINSTR_ENTRY(feature, number)			\
	".section .discard,\"a\",@progbits\n"		\
	DISCARD_ENTRY(number)				\
	".section .altinstr_replacement, \"ax\"\n"	\
	ALTINSTR_REPLACEMENT(newinstr, feature, number)	\
	".popsection\n"

/* alternative assembly primitive: */
#define ALTERNATIVE(oldinstr, newinstr, feature)			  \
	OLDINSTR(oldinstr)						  \
	ALTERNATIVE_N(newinstr, feature, 1)

#define ALTERNATIVE_2(oldinstr, newinstr1, feature1, newinstr2, feature2) \
	ALTERNATIVE(oldinstr, newinstr1, feature1)			  \
	ALTERNATIVE_N(newinstr2, feature2, 2)

/*
 * Alternative instructions for different CPU types or capabilities.
 *
 * This allows to use optimized instructions even on generic binary
 * kernels.
 *
 * length of oldinstr must be longer or equal the length of newinstr
 * It can be padded with nops as needed.
 *
 * For non barrier like inlines please define new variants
 * without volatile and memory clobber.
 */
#define alternative(oldinstr, newinstr, feature)                        \
        asm volatile (ALTERNATIVE(oldinstr, newinstr, feature) : : : "memory")

/*
 * Alternative inline assembly with input.
 *
 * Pecularities:
 * No memory clobber here.
 * Argument numbers start with 1.
 * Best is to use constraints that are fixed size (like (%1) ... "r")
 * If you use variable sized constraints like "m" or "g" in the
 * replacement make sure to pad to the worst case length.
 */
#define alternative_input(oldinstr, newinstr, feature, input...)	\
	asm volatile (ALTERNATIVE(oldinstr, newinstr, feature)		\
		      : : input)

/* Like alternative_input, but with a single output argument */
#define alternative_io(oldinstr, newinstr, feature, output, input...)	\
	asm volatile (ALTERNATIVE(oldinstr, newinstr, feature)		\
		      : output : input)

/*
 * This is similar to alternative_io. But it has two features and
 * respective instructions.
 *
 * If CPU has feature2, newinstr2 is used.
 * Otherwise, if CPU has feature1, newinstr1 is used.
 * Otherwise, oldinstr is used.
 */
#define alternative_io_2(oldinstr, newinstr1, feature1, newinstr2,	\
			 feature2, output, input...)			\
	asm volatile(ALTERNATIVE_2(oldinstr, newinstr1, feature1,	\
				   newinstr2, feature2)			\
		     : output : input)

/* Use this macro(s) if you need more than one output parameter. */
#define ASM_OUTPUT2(a...) a

#endif /*  !__ASSEMBLY__  */

#endif /* __X86_ALTERNATIVE_H__ */
