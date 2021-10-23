MEMORY
{
  FLASH (rx) 	: ORIGIN = 0x00010000, LENGTH = 1M
  RAM (rwx)  	: ORIGIN = 0x10400000, LENGTH = 736K

  /* mailbox test */
  /*RAM (rwx)  	: ORIGIN = 0x10420000, LENGTH = 384K */
  /*RETENTION (rwx)  : ORIGIN = 0x104B8000, LENGTH = 16K */
  RETENTION (r)  : ORIGIN = 0x200B8000, LENGTH = 16K 
}

STACK_SIZE = 8K;	

INCLUDE ././lib/modem/cmx_common.ld
