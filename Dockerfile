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
# To run the getuuid tool inside the docker image, you could then do
#
#     dlite dlite-getuuid <string>
#
# To run a python script in current directory inside the docker image
#
#     dlite python /data/script.py
#


##########################################
# Stage: install dependencies
##########################################
FROM ubuntu:21.04 AS dependencies
RUN apt-get -qq update --fix-missing

# Install dependencies
RUN DEBIAN_FRONTEND="noninteractive" apt-get install -qq -y --fix-missing \
        cmake \
        cmake-curses-gui \
        cppcheck \
        doxygen \
        gcc \
        gdb \
        gfortran \
        git \
        g++ \
        libhdf5-dev \
        librdf0-dev \
        librasqal3-dev \
        libraptor2-dev \
        make \
        python3-dev \
        python3-pip \
        swig4.0 \
        rpm \
        librpmbuild9 \
        dpkg \
    && rm -rf /var/lib/apt/lists/*


# Install Python packages
COPY requirements.txt .
RUN pip3 install --trusted-host files.pythonhosted.org \
    --upgrade pip -r requirements.txt


##########################################
# Stage: build
##########################################
FROM dependencies AS build


# Setup dlite
RUN mkdir -p /home/user/sw/dlite
COPY bindings /home/user/sw/dlite/bindings
COPY  cmake /home/user/sw/dlite/cmake
COPY  doc /home/user/sw/dlite/doc
COPY  examples /home/user/sw/dlite/examples
COPY  src /home/user/sw/dlite/src
COPY  storages /home/user/sw/dlite/storages
COPY  tools /home/user/sw/dlite/tools
COPY  CMakeLists.txt LICENSE README.md /home/user/sw/dlite/
WORKDIR /home/user/sw/dlite

# Perform static code checking
# FIXME - test_tgen.c produce a lot of false positives
RUN cppcheck . \
    -I src \
    --language=c -q --force --error-exitcode=2 --inline-suppr -i build

# Build dlite
RUN mkdir build
WORKDIR /home/user/sw/dlite/build
RUN cmake .. -DFORCE_EXAMPLES=ON -DALLOW_WARNINGS=ON -DWITH_FORTRAN=ON \
        -DCMAKE_INSTALL_PREFIX=/tmp/dlite-install
RUN make

# Create distributable packages
RUN cpack
RUN cpack -G DEB
RUN cpack -G RPM

# Install
RUN make install

# Skip postgresql tests since we haven't set up the server and
# static-code-analysis since it is already done.
# TODO - set up postgresql server and run the postgresql tests...
RUN ctest -E "(postgresql|static-code-analysis)" || \
    ctest -E "(postgresql|static-code-analysis)" \
        --rerun-failed --output-on-failure -VV

# Set DLITE_USE_BUILD_ROOT in case we want to test dlite from the build dir
ENV DLITE_USE_BUILD_ROOT=YES


#########################################
# Stage: develop
#########################################
FROM build AS develop
ENV PATH=/tmp/dlite-install/bin:$PATH
ENV LD_LIBRARY_PATH=/tmp/dlite-install/lib:$LD_LIBRARY_PATH
ENV DLITE_ROOT=/tmp/dlite-install
ENV PYTHONPATH=/tmp/dlite-install/lib/python3.8/site-packages:$PYTHONPATH


##########################################
# Stage: final slim image
##########################################
FROM ubuntu:21.04 AS production
#FROM python:3.9.6-slim-buster

RUN apt -qq update
RUN DEBIAN_FRONTEND="noninteractive" apt-get install -qq -y --fix-missing librdf0 python3-dev python3-pip \
  && rm -rf /var/lib/apt/lists/*
# Copy needed dlite files and libraries to slim image
COPY --from=build /tmp/dlite-install /usr/local
COPY --from=build /usr/lib/x86_64-linux-gnu/libhdf5*.so* /usr/local/lib/
COPY --from=build /usr/lib/x86_64-linux-gnu/libsz.so* /usr/local/lib/
COPY --from=build /usr/lib/x86_64-linux-gnu/libaec.so* /usr/local/lib/
COPY --from=build /usr/lib/x86_64-linux-gnu/libm.so* /usr/local/lib/
RUN pip install --upgrade pip \
    --trusted-host files.pythonhosted.org \
    numpy \
    PyYAML \
    psycopg2-binary

WORKDIR /home/user
ENV LD_LIBRARY_PATH=/usr/local/lib
ENV DLITE_ROOT=/usr/local
ENV PYTHONPATH=/usr/local/lib/python3.9/site-packages

# Default command
CMD ["/bin/bash"]
