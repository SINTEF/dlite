#
# Build this docker with
#
#     docker build -t dlite .
#
# To run this docker in an interactive bash shell, do
#
#     docker run -i -t dlite
#
# A more realistic way to use this docker is to put the following
# into a shell script (called dlite)
#
#     #!/bin/sh
#     docker run --rm -it --user="$(id -u):$(id -g)" --net=none \
#         -v "$PWD":/data dlite "$@"
#
# To run the getuuid tool, you could then do
#
#     dlite dlite-getuuid <string>
#
# To run a python script in current directory
#
#     dlite python /data/script.py
#


##########################################
# Stage: install dependencies
##########################################
FROM ubuntu:20.04 AS dependencies
RUN apt-get update --fix-missing

# Default cmake is 3.10.2. We need at least 3.11...
# Install tools for adding cmake
RUN apt-get install -y \
    apt-transport-https \
    ca-certificates \
    gnupg \
    software-properties-common \
    wget

# Obtain signing key
RUN wget -O - \
     https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \
     apt-key add -

# Add Kitware repo
#RUN apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
RUN apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
RUN apt update

# Ensure that our keyring stays up to date
RUN apt-get install kitware-archive-keyring

# Install dependencies
#RUN apt-get install -y --fix-missing
RUN apt-get install -y \
    cmake \
    cmake-curses-gui \
    cppcheck \
    doxygen \
    gcc \
    gdb \
    gfortran \
    git \
    graphviz \
    g++ \
    libhdf5-dev \
    libjansson-dev \
    make \
    python3 \
    python3-dev \
    python3-numpy \
    python3-psycopg2 \
    python3-yaml \
    python3-pip \
    swig3.0

# Install Python packages
RUN pip3 install --trusted-host files.pythonhosted.org \
    --upgrade pip
RUN pip3 install --trusted-host files.pythonhosted.org \
    fortran-language-server


##########################################
# Stage: build
##########################################
FROM dependencies AS build

# Create and become a normal user
RUN useradd -ms /bin/bash user
USER user
ENV PYTHONPATH "/home/user/EMMO-python/:${PYTHONPATH}"


# Setup dlite
RUN mkdir /home/user/sw
COPY --chown=user:user . /home/user/sw/dlite
WORKDIR /home/user/sw/dlite
RUN rm -rf build

# Perform static code checking
# FIXME - test_tgen.c produce a lot of false positives
RUN cppcheck . \
    --language=c -q --force --error-exitcode=2 --inline-suppr -i build \
    -i src/utils/tests/test_tgen.c

# Build dlite
RUN mkdir build
WORKDIR /home/user/sw/dlite/build
RUN cmake .. -DFORCE_EXAMPLES=ON -DALLOW_WARNINGS=ON -DWITH_FORTRAN=ON \
        -DCMAKE_INSTALL_PREFIX=/tmp/dlite-install
RUN make
USER root
RUN make install

# Skip postgresql tests since we haven't set up the server and
# static-code-analysis since it is already done.
# TODO - set up postgresql server and run the postgresql tests...
USER user
RUN ctest -E "(postgresql|static-code-analysis)" || \
    ctest -E "(postgresql|static-code-analysis)" \
        --rerun-failed --output-on-failure -VV

# Remove unneeded installed files
USER root
RUN rm -r /tmp/dlite-install/lib/lib*.a
RUN rm -r /tmp/dlite-install/include
RUN rm -r /tmp/dlite-install/share/dlite/html
RUN rm -r /tmp/dlite-install/share/dlite/examples
RUN rm -r /tmp/dlite-install/share/dlite/cmake


##########################################
# Stage: final slim image
##########################################
FROM python:3.8.3-slim-buster
COPY --from=build /tmp/dlite-install /usr/local
COPY --from=build /usr/lib/x86_64-linux-gnu/libjansson.so* /usr/local/lib/
COPY --from=build /usr/lib/x86_64-linux-gnu/libhdf5*.so* /usr/local/lib/
COPY --from=build /usr/lib/x86_64-linux-gnu/libsz.so* /usr/local/lib/
COPY --from=build /usr/lib/x86_64-linux-gnu/libaec.so* /usr/local/lib/
COPY --from=build /usr/lib/x86_64-linux-gnu/libm.so* /usr/local/lib/
RUN pip install --upgrade pip
RUN pip install numpy PyYAML psycopg2-binary
RUN useradd -ms /bin/bash user
USER user
WORKDIR /home/user
ENV LD_LIBRARY_PATH=/usr/local/lib
ENV DLITE_ROOT=/usr/local

# Default command
CMD ["/bin/bash"]
