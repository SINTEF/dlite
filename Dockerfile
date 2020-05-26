#
# Build this docker with
#
#     docker build -t dlite .
#
# Run this docker
#
#     docker run -i -t dlite
#

FROM ubuntu:18.04 AS dependencies

RUN apt-get update

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
RUN apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
RUN apt update

# Install dependencies
RUN apt-get install -y \
    cmake \
    doxygen \
    gcc \
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
    swig3.0 \
    cppcheck \
    gfortran

# Install Python packages
RUN pip3 install ipython

# The following section performs the build
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
RUN cppcheck . \
    --language=c -q --force --error-exitcode=2 --inline-suppr -i build

# Build dlite
RUN mkdir build
WORKDIR /home/user/sw/dlite/build
RUN cmake ..
RUN make
RUN make install
RUN ctest -E postgresql  # skip postgresql since we haven't set up the server

ENTRYPOINT ipython3 \
    --colors=LightBG \
    --autocall=1 \
    --no-confirm-exit \
    --TerminalInteractiveShell.display_completions=readlinelike \
    -i \
    -c 'import dlite' \
    --
