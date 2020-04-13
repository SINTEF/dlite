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
    swig3.0 \
    cppcheck \
    gfortran

# Install IPython
#RUN apt-get install -y ipython3
RUN apt-get install -y python3-pip
RUN pip3 install ipython

# The following section performs the build
FROM dependencies AS build

# Create and become a normal user
RUN useradd -ms /bin/bash user
USER user
ENV PYTHONPATH "/home/user/EMMO-python/:${PYTHONPATH}"

RUN mkdir /home/user/sw

WORKDIR /home/user/sw
RUN git clone https://github.com/SINTEF/dlite.git

WORKDIR /home/user/sw/dlite
RUN git submodule update --init
RUN mkdir build

RUN git fetch origin
#RUN git checkout fixed-several-additional-bugs
RUN git checkout debug

RUN cppcheck . \
    --language=c -q --force --error-exitcode=2 --inline-suppr -i build

WORKDIR /home/user/sw/dlite/build
RUN cmake .. -DFORCE_EXAMPLES=ON
RUN make
RUN make install
RUN make test

ENTRYPOINT ipython3 \
    --colors=LightBG \
    --autocall=1 \
    --no-confirm-exit \
    --TerminalInteractiveShell.display_completions=readlinelike \
    -i \
    -c 'import dlite' \
    --
