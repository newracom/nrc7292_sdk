MEMORY
{
  FLASH (rx)    : ORIGIN = 0x00010000, LENGTH = FLASH_SIZE
  RAM (rwx)  	: ORIGIN = 0x10410000, LENGTH = 672K - STACK_SIZE
  RETENTION (rwx)      : ORIGIN = 0x200B8000, LENGTH = 15K
  RETENTION_INFO(rwx)  : ORIGIN = 0x200BBC00, LENGTH = 1008
}

FLASH_SIZE = 916K;
STACK_SIZE = 8K;

INCLUDE ././lib/modem/cmx_common_runram.ld
