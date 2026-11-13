:py:mod:`dlite.testutils`
=========================

.. py:module:: dlite.testutils

.. autoapi-nested-parse::

   Some utilities for testing.



Module Contents
---------------

Classes
~~~~~~~

.. autoapisummary::

   dlite.testutils.raises
   dlite.testutils.skip_on_service_mismatch



Functions
~~~~~~~~~

.. autoapisummary::

   dlite.testutils.importcheck
   dlite.testutils.importskip
   dlite.testutils.serverskip
   dlite.testutils.service_config
   dlite.testutils.service_available
   dlite.testutils.serviceskip



Attributes
~~~~~~~~~~

.. autoapisummary::

   dlite.testutils.TEST_SERVICES
   dlite.testutils.SERVICE_MISMATCH_PATTERNS


.. py:exception:: UnexpectedSuccessError

   Bases: :py:obj:`Exception`

   Code that was expected to raise an exeption didn't do so.


.. py:exception:: UnexpectedExceptionError

   Bases: :py:obj:`Exception`

   Code did not raise the expected exception.


.. py:class:: raises(*exceptions, silent=True)

   Assert that a code block raises one of the expected exceptions
   listed in `exceptions`.

   :param \*exceptions: Expected exception classes.
   :param silent: Whether to silent DLite error messages in the code block.

   .. py:attribute:: exceptions
      :annotation: = ()

      

   .. py:attribute:: silent
      :annotation: = True

      

   .. py:method:: __enter__()


   .. py:method:: __exit__(exc_type, exc_value, tb)



.. py:function:: importcheck(module_name, package=None)

   Import and return the requested module or None if the module can't
   be imported.


.. py:function:: importskip(module_name, package=None, exitcode=44, env_exitcode='DLITE_IMPORTSKIP_EXITCODE')

   Import and return the requested module.

   Calls `sys.exit()` with given exitcode if the module cannot be imported.

   :param module_name: Name of module to try to import.
   :param package: Optional package name that the module might reside in.
   :param exitcode: The default exit code of `sys.exit()` if the module cannot
                    be imported.
   :param env_exitcode: Name of environment variable containing an exitcode
                        to call `sys.exit()` with if the module cannot be imported.
                        This overrides `exitcode`.

   .. admonition:: Notes

      If you want to run the tests and get an error if a module
      cannot be imported, set environment variable
      `DLITE_IMPORTSKIP_EXITCODE=1` before running the tests (if you
      run with ctest, set `DLITE_IMPORTSKIP_EXITCODE` at configure
      time).
      
      For packages that depend on external services like postgresql,
      call `importskip()` with `env_exitcode=None` to skip the test
      regardless of whether `DLITE_IMPORTSKIP_EXITCODE` is set or not.


.. py:function:: serverskip(server, port, timeout=2, exitcode=44)

   Ping a remote server and calls `sys.exit()` with given exitcode if it
   doesn't respond within the timeout.


.. py:data:: TEST_SERVICES
   

   

.. py:function:: service_config(service)

   Return a dict with the configuration of the given test service.

   :param service: Name of the service.  Should be a key in `TEST_SERVICES`.

   :returns: Dict with the default configuration in `TEST_SERVICES`, where
             each value may be overridden by the environment variable
             DLITE_TEST_<SERVICE>_<KEY>.


.. py:function:: service_available(service, timeout=2)

   Return true if the given test service can be contacted, otherwise
   false.


.. py:function:: serviceskip(service, timeout=2, exitcode=44)

   Call `sys.exit()` with the given exit code if the given test
   service cannot be contacted.  Otherwise return its configuration
   (see `service_config()`).


.. py:data:: SERVICE_MISMATCH_PATTERNS
   

   

.. py:class:: skip_on_service_mismatch(service, exitcode=44)

   Context manager that calls `sys.exit()` with the given exit code
   if the code block raises an exception indicating that the server is
   not configured as expected (e.g. wrong credentials).

   .. admonition:: Example

      with skip_on_service_mismatch("postgresql"):
          inst.save(f"postgresql://{location}?{options}")

   .. py:attribute:: service
      

      

   .. py:attribute:: exitcode
      :annotation: = 44

      

   .. py:method:: __enter__()


   .. py:method:: __exit__(exc_type, exc_value, tb)



