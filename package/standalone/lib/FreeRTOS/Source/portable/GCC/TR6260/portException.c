#ifdef MPU_SUPPORT

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
 * all the API functions to use the MPU wrappers.  That should only be done when
 * task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* FreeRTOS includes. */
#include "FreeRTOS.h"

/* Lint e961 and e750 are suppressed as a MISRA exception justified because the
 * MPU ports require MPU_WRAPPERS_INCLUDED_FROM_API_FILE to be defined for the
 * header files above, but not in this file, in order to generate the correct
 * privileged Vs unprivileged linkage and placement. */
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE /*lint !e961 !e750. */

#include "nds32_intrinsic.h"
#include "nds32.h"
#include <stdio.h>

#define ALIGNMENT		0x0
#define RESERVERD_INST		0x1
#define TRAP			0x2
#define ARITHMETIC		0x3
#define PRECISE_BUS_ERR		0x4
#define IMPRECISE_BUS_ERR	0x5
#define COPROCESSOR		0x6
#define PRIVILEGED_INST		0x7
#define RESERVED_VALUE		0x8
#define NONEXISTENT_MEM_ADDR	0x9
#define MPZIU_CONTROL		0xA

/* Exception entry functions */
void OS_Trap_TLB_Fill(void) PRIVILEGED_FUNCTION __attribute__((naked));
void OS_Trap_TLB_Misc(void) PRIVILEGED_FUNCTION __attribute__((naked));
void OS_Trap_General_Exception() PRIVILEGED_FUNCTION __attribute__((naked));
void OS_Trap_Syscall(void) PRIVILEGED_FUNCTION __attribute__((naked));

/* Valid access point. It should be point to prvRaisePrivilege() address. */
extern void * const valid_ap; //valid access point

/*
 * The MPU wrapper function will call prvRaisePrivilege() function to access
 * PSW to privileged mode. If it is called in user mode, the 'Privileged instruction'
 * general exception will occur. The general exception handler must take it
 * and return IPSW value back.
 */
static unsigned long General_Exception_Handler(unsigned long * topOfStack)
{
	unsigned int etype = __nds32__mfsr(NDS32_SR_ITYPE) & 0xf;

	switch (etype)
	{
		case PRIVILEGED_INST:
		{
			unsigned long ipc = __nds32__mfsr(NDS32_SR_IPC);
			/* Is it from prvRaisePrivileg() by accessing PSW? */
			if ( ipc == (unsigned long)valid_ap )
			{
				__nds32__mtsr(ipc+4, NDS32_SR_IPC);

				/* return IPSW by $r0 */
				return __nds32__mfsr(NDS32_SR_IPSW);
			}
		}
		default:
		{
			printf("General Exception : EType is %d\n", etype);

			/* Dump GPRs contents */
			int i;
			for ( i = 0; i < 30; ++i)
			{
				printf(" $R%d 0x%x ",i,topOfStack[i]);
				if (!(i%4))
					printf("\n");
			}
			printf(" $SP 0x%x\n", &topOfStack[31]);
			while(1);
		}
	};
}

/*
 * Dump all the MPU TLB entry information
 */
static void Dump_TLB_Entry(void)
{
	unsigned int tlb_data, tlb_vpn;
	int i;

	/* Dump MPU Entry Information */
	for ( i = 0; i < 8; i++ ) {
		__nds32__tlbop_trd(i << 29);
		__nds32__dsb();
		tlb_vpn = __nds32__mfsr(NDS32_SR_TLB_VPN);
		tlb_data = __nds32__mfsr(NDS32_SR_TLB_DATA);

		printf("Entry %d\r\n", i);
		printf("TLB_VPN is 0x%x\r\n", tlb_vpn);
		printf("TLB_DATA is 0x%x\r\n", tlb_data);
		printf("\n");
	}
}

void OS_Trap_TLB_Fill(void)
{
	/* enable IT/DT */
	__asm__ volatile
	(
		"mfsr	$p0, $PSW\n\t"
		"ori	$p0, $p0, %0\n\t"
		"mtsr	$p0, $PSW\n\t"
		"dsb\n\t"
		:: "i" (PSW_mskIT | PSW_mskDT)
	);

	printf("OS_Trap_TLB_Fill\r\n");

	Dump_TLB_Entry();

	while(1);
}

void OS_Trap_TLB_Misc(void)
{
	/* enable IT/DT */
	__asm__ volatile
	(
		"mfsr	$p0, $PSW\n\t"
		"ori	$p0, $p0, %0\n\t"
		"mtsr	$p0, $PSW\n\t"
		"dsb\n\t"
		:: "i" (PSW_mskIT | PSW_mskDT)
	);

	printf("OS_Trap_TLB_Misc\r\n");

	Dump_TLB_Entry();

	while(1);
}

void OS_Trap_General_Exception(void)
{
	/* enable IT/DT and save registers */
	__asm__ volatile
	(
		"mfsr	$p0, $PSW\n\t"
		"ori	$p0, $p0, %0\n\t"
		"mtsr	$p0, $PSW\n\t"
		"dsb\n\t"
		"pushm	$r0, $r30\n\t"
		:: "i" (PSW_mskIT | PSW_mskDT)
	);

	General_Exception_Handler((unsigned long *)__nds32__get_current_sp());

	__asm__ volatile
	(
		"addi	$sp, $sp ,0x4\n\t"		// skip original $r0 since $r0 is $IPSW
							// would be returned to $PSW by iret.
		"popm	$r1, $r30\n\t"
		"iret\n\t"
	);
}

void OS_Trap_Syscall(void)
{
	/*
	 * Called from prvRaisePrivilege().
	 * Do nothing just set to privileged mode.
	 */
	__asm__ volatile
	(
		"mfsr	$p0, $IPSW\n\t"
		"ori	$p0, $p0, %0\n\t"		// Set privilege bit
		"mtsr	$p0, $IPSW\n\t"

		"mfsr	$p0, $IPC\n\t"
		"addi	$p0, $p0, 0x4\n\t"
		"mtsr   $p0, $IPC\n\t"

		"iret\n\t"
		:: "i" (1 << PSW_offPOM)
	);
}

#endif	// MPU_SUPPORT
