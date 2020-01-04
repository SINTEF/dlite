#
# Build this docker with
#
#     docker build -t dlite .
#
# Run this docker
#
#     docker run -i -t dlite
#


#from continuumio/miniconda3
FROM ubuntu:18.04

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
    g++ \
    libhdf5-dev \
    libjansson-dev \
    make \
    python3 \
    python3-dev \
    python3-numpy \
    python3-psycopg2 \
    python3-yaml \
    swig3.0

# Install IPython
#RUN apt-get install -y ipython3
RUN apt-get install -y python3-pip
RUN pip3 install ipython


#RUN git clone https://github.com/SINTEF/dlite.git
#cd dlite
#RUN git submodule init
#RUN git submodule update

RUN useradd -ms /bin/bash user
COPY . /home/user/sw/dlite
RUN chown user:user -R /home/user/sw/dlite
USER user
WORKDIR /home/user/
ENV PYTHONPATH "/home/user/EMMO-python/:${PYTHONPATH}"

RUN mkdir /home/user/sw/dlite/build
RUN cd /home/user/sw/dlite/build && cmake ..
RUN cd /home/user/sw/dlite/build && make
RUN cd /home/user/sw/dlite/build && make install
RUN cd /home/user/sw/dlite/build && make test


ENTRYPOINT ipython3 \
    --colors=LightBG \
    --autocall=1 \
    --no-confirm-exit \
    --TerminalInteractiveShell.display_completions=readlinelike \
    -i \
    -c 'import dlite' \
    --
