"""Some utilities for testing."""
import atexit
import importlib
import os
import time
import socket
import subprocess
import sys
from pathlib import Path
from typing import Optional

import dlite
from dlite.paths import dlite_SOURCE_ROOT, DLITE_DATA_DIR


class UnexpectedSuccessError(Exception):
    """Code that was expected to raise an exeption didn't do so."""


class UnexpectedExceptionError(Exception):
    """Code did not raise the expected exception."""


class raises():
    """Assert that a code block raises one of the expected exceptions
    listed in `exceptions`.

    Arguments:
        *exceptions: Expected exception classes.
        silent: Whether to silent DLite error messages in the code block.
    """
    def __init__(self, *exceptions, silent=True):
        self.exceptions = exceptions
        self.silent = silent

    def __enter__(self):
        if self.silent:
            # Silence DLite error messages from expected exceptions and save the
            # current err_ignore state
            self.save_silenced = {}
            for exc in self.exceptions:
                code = dlite._dlite._err_getcode(exc.__name__)
                self.save_silenced[code] = dlite._dlite._err_ignore_get(code)
                dlite._dlite._err_ignore_set(code, 1)

    def __exit__(self, exc_type, exc_value, tb):
        if self.silent:
            # Restore the err_ignore state
            for code, value in self.save_silenced.items():
                dlite._dlite._err_ignore_set(code, value)

        excnames = ", ".join(repr(exc.__name__) for exc in self.exceptions)
        if exc_type is None:
            raise UnexpectedSuccessError(
                f"Expected one of the following exceptions: {excnames}, "
                "but no exception was raised."
            )

        # Leave context manager gracefully if one of the expected exceptions
        # was raised
        for exc in self.exceptions:
            if issubclass(exc_type, exc):
                dlite.errclr()
                return True

        raise UnexpectedExceptionError(
            f"Expected one of the following exceptions: {excnames}, "
            f"but got: '{exc_type.__name__}'"
        ) from exc_value


def importcheck(module_name, package=None):
    """Import and return the requested module or None if the module can't
    be imported."""
    try:
        return importlib.import_module(module_name, package=package)
    except ModuleNotFoundError as exc:
        return None


def importskip(module_name, package=None, exitcode=44,
               env_exitcode="DLITE_IMPORTSKIP_EXITCODE"):
    """Import and return the requested module.

    Calls `sys.exit()` with given exitcode if the module cannot be imported.

    Arguments:
        module_name: Name of module to try to import.
        package: Optional package name that the module might reside in.
        exitcode: The default exit code of `sys.exit()` if the module cannot
            be imported.
        env_exitcode: Name of environment variable containing an exitcode
            to call `sys.exit()` with if the module cannot be imported.
            This overrides `exitcode`.

    Notes:
        If you want to run the tests and get an error if a module
        cannot be imported, set environment variable
        `DLITE_IMPORTSKIP_EXITCODE=1` before running the tests (if you
        run with ctest, set `DLITE_IMPORTSKIP_EXITCODE` at configure
        time).

        For packages that depend on external services like postgresql,
        call `importskip()` with `env_exitcode=None` to skip the test
        regardless of whether `DLITE_IMPORTSKIP_EXITCODE` is set or not.

    """
    try:
        return importlib.import_module(module_name, package=package)
    except ModuleNotFoundError as exc:
        if env_exitcode and env_exitcode in os.environ:
            try:
                exitcode = int(os.environ[env_exitcode])
            except:
                pass
        else:
            print(f"{exc}: skipping test", file=sys.stderr)

        sys.exit(exitcode)


def servercheck(server, port, timeout=2):
    """Ping remote server on given port. Raises OSError on failure."""
    socket.setdefaulttimeout(timeout)
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((server, port))  # raises OSError on error
    s.close()


def serverskip(server, port, timeout=2, exitcode=44):
    """Ping a remote server and calls `sys.exit()` with given exitcode if it
    doesn't respond within the timeout."""
    try:
        servercheck(server, port, timeout=timeout)
    except OSError as exc:
        print(
            f"Server {server}:{port} seems to be down: {exc}", file=sys.stderr
        )
        sys.exit(exitcode)


class Service:
    """A class for starting services that are needed for testing.

    Arguments:
        name: Name of service.
        host: Host on which to ping for status.
        port: Port on which service is listening.
        timeout: Timeout in seconds for contacting the service.
        image: Docker image to use.
        compose: Docker compose file to use.
        autostop: Automatically stop the service when the script exists.
        sleep: Sleep time in seconds for service to get ready after start.
        stamp: Whether to create a stamp file if the service is already running.
             If a stamp file exists when stopping a service, the stamp file
             is removed instead of stopping the service.
    """
    # Possible configuration keys:
    #   - port: Port number that service is listening to.
    #   - image: Docker image that is used. If given, the service is started
    #         with `docker`. Otherwise with `docker compose`.
    #   - sleep: Number of sections to sleep after the service is started.
    conf = {
        "minio": {"port": 9000},
        "redis": {"port": 6379, "image": "redis:7-alpine"},
        "postgres": {"port": 5432, "sleep": 5},
        "mongo": {"port": 27017},
        #"mongo": {"port": 27017, "ping": 'exec mongodb mongosh --eval "db.runCommand({ping:1})"'},
    }

    def __init__(
        self,
        name: str,
        host: str = "localhost",
        port: Optional[int] = None,
        timeout: float = 3.05,
        image: Optional[str] = None,
        compose_file: Optional[str] = None,
        autostop: bool = True,
        sleep: Optional[float] = None,
        stamp: bool = False,
    ):
        rootdir = Path(dlite_SOURCE_ROOT)
        datadir = Path(DLITE_DATA_DIR)
        testdir = rootdir / "bindings" / "python" / "tests"
        if datadir.exists():
            outdir = Path("/tmp") / "dlite"
            outdir.mkdir(parents=True, exist_ok=True)
            composedir = datadir / "services"
        else:
            composedir = testdir / "services"
            outdir = testdir / "output"

        compfile = composedir / f"compose-{name}.yml"
        if compose_file is None and compfile.exists():
            compose_file = compfile

        conf_sleep = self.conf.get(name, {}).get("sleep", 0.5)

        self.name = name
        self.port = port if port else self.conf[name]["port"]
        self.host = host
        self.timeout = timeout
        self.image = image if image else self.conf.get(name, {}).get("image")
        self.compose_file = compose_file
        self.container_name = f"ctest-{name}-server"
        self.autostop = autostop
        self.sleep = sleep if sleep else conf_sleep
        self.stamp = stamp
        self.running = None  # Whether the service is already running
        # File created by start() if service is already running
        self.stampfile = outdir / f"{name}.stamp"

    def run(self, cmd, **kwargs):
        """Help function for executing a command capturing errors.

        Keyword arguments are passed to subprocess.run()."""
        command = [str(c) for c in cmd]
        r = subprocess.run(command, **kwargs)
        if r.returncode:
            raise OSError(
                f"Command `{' '.join(command)}` returned non-zero exit status: "
                f"{r.returncode}"
            )
        return r

    def start(self):
        """Start service."""
        # Don't start service if it is already running
        self.running = self.status()
        if self.running:
            if self.stamp:
                self.stampfile.touch()
            return

        # Make sure there is not stampfile
        self.stampfile.unlink(missing_ok=True)

        # Start container
        if self.compose_file:
            args = ["docker", "compose", "-f", self.compose_file, "up", "-d"]
        elif self.image:
            args = ["docker", "run", "-d", "--name", self.container_name,
                    "-p", f"{self.port}:{self.port}", self.image]
        else:
            raise TypeError(
                f"Cannot start {name}. Either `image` or `compose_file` must "
                "be given."
            )
        self.run(args)

        # Sleep to let the service getting started
        time.sleep(self.sleep)

        # Check that container is ready
        if not self.status():
            self.stop()
            raise ConnectionError(f"Cannot connect to {self.name}")

        if self.autostop and not self.running:
            atexit.register(self.stop)

    def stop(self):
        """Stop service."""
        # Do not stop service if a stampfile exists - just remove the stampfile
        if self.stamp and self.stampfile.exists():
            self.stampfile.unlink()
            return

        if self.compose_file:
            self.run(
                ["docker", "compose", "-f", self.compose_file, "down"],
                capture_output=True,
            )
        elif self.image:
            cmds = [
                ["docker", "stop", self.container_name],
                ["docker", "rm", self.container_name],
            ]
            for args in cmds:
                subprocess.run(args, capture_output=True)

        if self.autostop and self.running is False:
            atexit.unregister(self.stop)

    def status(self) -> bool:
        """Returns true if the service is running."""
        try:
            servercheck(self.host, self.port, self.timeout)
        except OSError:
            return False
        return True
