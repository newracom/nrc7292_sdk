/* below parameters defined to get around build error */
/* must check if there's any side effect */
void *__dso_handle = 0;

extern "C" int __aeabi_atexit(void *object,
		void (*destructor)(void *),
		void *dso_handle)
{
	return 0;
}

namespace __gnu_cxx {
void __verbose_terminate_handler()
{
  while(1);
}
}
