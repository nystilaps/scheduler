# Create a virtual environment with all tools installed
# ref: https://hub.docker.com/_/ubuntu
FROM ubuntu:rolling AS env
# Install system build dependencies
# note: here we use the CMake package provided by Ubuntu
# see: https://repology.org/project/cmake/versions
ENV PATH=/usr/local/bin:$PATH
RUN apt-get update -q && \
apt-get install -yq git build-essential cmake && \
apt-get clean && \
rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

CMD [ "/bin/sh" ]

# Add the library src to our build env
FROM env AS devel
# Create lib directory
WORKDIR /home/lib
# Bundle lib source
COPY . .

# Build in an other stage
FROM devel AS build
# CMake configure
RUN cmake -H. -Bbuild
# CMake build
RUN cmake --build build --target all
# CMake install
#RUN cmake --build build --target install

# Create an install image to check cmake install config
FROM env AS install
# Copy lib from build to install
COPY --from=build /usr/local /usr/local/
# Copy  sample
WORKDIR /home/sample
COPY ci/sample .
