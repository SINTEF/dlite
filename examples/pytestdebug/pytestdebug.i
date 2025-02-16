%module pytestdebug


%{
  #include <stdio.h>

  const char *hello(void)
  {
    return "Hello world!";
  }

%}


const char *hello(void);
